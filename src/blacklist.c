#include "avlTree.h"
#include "blacklist.h"
#include "generalMethods.h"
#include <stdlib>
#include <string.h>
#include "tmdl.h"

avlTree *blacklist;
char *blacklistLocation;
bool hashFail = false;

int blacklistComparator (const blacklistEntry *alpha, 
        const blacklistEntry *beta) {
    return strcmp(alpha->hashValue, beta->hashValue);
}

//One blacklist entry is 3 lines, 1 value line, 1 chapter line and one file name line
//each entry can be new line seperated but in between lines it cannot
//Should we keep exact file name? or just the number and rm with [number].*
blacklistEntry *read_single_entry(FILE *blacklistFile) {
    char *readHashValue = read_from_file(blacklistFile, '\n', true);
    if (readHashValue == NULL) {
        return NULL;
    } else if (readHashValue[0] == '\0') {
        free(readHashValue);
        return read_single_entry(blacklistFile);
    }
    char *readChapterName = read_from_file(blacklistFile, '\n', true);
    if (readChapterName == NULL || readChapterName[0] == '\0') {
        free(readHashValue);
        free(readChapterName);
        return NULL;
    }
    char *readFileName = read_from_file(blacklistFile, '\n', true);
    if (readFileName == NULL || readFileName[0] == '\0') {
        free(readHashValue);
        free(readChapterName);
        free(readFileName);
        return NULL;
    }
    blacklistEntry *newEntry = (blacklistEntry *) malloc(sizeof(blacklistEntry)); 
    if (newEntry == NULL) {
        exit(21);
    }
    newEntry->hashValue = readHashValue;
    newEntry->chapterName = readChapterName;
    newEntry->fileName = readFileName;
    return newEntry;
}

blacklistEntry **read_blacklist(FILE *blacklistFile) {
    blacklistEntry *next;
    unsigned long long dynamic = 4, count = 0;
    blacklistEntry **loadedList = (blacklistEntry **) 
            malloc(sizeof(blacklistEntry *) * dynamic);
    if (loadedList == NULL) {
        exit(21);
    }
    while(next = read_single_entry(blacklistFile), (next != NULL)) {
        count++;
        if (count == dynamic) {
            dynamic *= 2;
            loadedList = (blacklistEntry **) realloc(loadedList, 
                    sizeof(blacklistEntry *) * dynamic);
            if (loadedList == NULL) {
                exit(21);
            }
        }
        loadedList[count - 1] = next;
    }
    loadedList[count] = NULL;
}

void load_blacklist() {
    if (!get_delete()) {
        return;
    }
    blacklistLocation = concat(get_series_folder, ".blacklist.sha256");
    FILE *blacklistFile = fopen(blacklistLocation, "r");
    if (blacklistFile == NULL) {
        blacklist = (avlTree *) malloc(sizeof(avlTree));
        blacklist->size = 0;
        blacklist->comparator = strcmp;
        if (get_verbose()) {
            puts("New blacklist being created");
        }
    } else {
        blacklistEntry **loadedList = read_blacklist(blacklistFile);
        blacklist = sorted_construction(loadedList, blacklistComparator);
        free(loadedList);
        if (get_verbose()) {
            puts("Loaded saved blacklist");
        }
    }
    fclose(blacklistLocation);
}

//if fails say blacklist not being used once and then move on
char *calculate_hash(char *filePath) {
    int fds[2];
    pipe(fds);
    int pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        dup2(fds[1], 1);
        close(fd[0]), close(2);
        execlp("shasum", "shasum", "-U" , "-a", "256", filePath, NULL);
        exit(24);
    }
    //parent
    close(fds[1]);
    bool errorOccured = false;
    int wait;
    if ((wait(&status) == -1) || (WIFEXITED(status) == 0)) {
        errorOccured = true;
    }
    if (WEXITSTATUS(status) != 0) {
        errorOccured = true;
    }
    if (errorOccured) {
        if (hashFail == false) {
            if (get_verbose()) {
                fputs("Hashing is not currently being used - please ensure "
                        "sha256sum is installed\n", stderr);
            }
            hashFail = true;
        }
        return NULL;
    } else {
        FILE *hashSumStream = fopen(fds[0]);
        if (hashSumStream == NULL) {
            exit(21);
        }
        char *fileHash = read_from_file(hashSumStream, ' ', true);
        fclose(hashSumStream);
        close(fds[0]); 
        return fileHash;
    }
}

bool chapter_is_zip(char *chapterLocation) {
    char *downloadedFileName = concat(chpaterLocation, ".cbz");              
    char *fullPath = concat(get_series_folder(), downloadedFileName);
    //need some kind of check this isn't a directory
    bool isZip = ((access(fullPath, F_OK) != -1) && is_file(fullPath));
    free(fullPath);
    free(downloadedFileName);
    return isZip;
}

void delete_file_in_zip() {
    
}

void delete_blacklisted_file(blacklistEntry *toDelete) {
    if ((strcmp(toDelete->chapterName, "Deleted") == 0) && 
            (strcmp(toDelete->fileName))) {
        return;
    }
    char *chapterPath = concat(get_series_folder(), toDelete->chapter); 
    if (chapter_is_zip(chapterPath)) {
        char *chapterZipLocation = concat(chpaterLocation, ".cbz");              
        //if zip do appropriate zip thing with correct error message no exit here
        delete_file_in_zip(chapterZipLocation, toDelete->file);
        free(chapterZipLocation);
    } else {
        char *tempPath = concat(chapterPath, "/");
        char *fullFilePath = concat(tempPath, toDelete->file);
        delete_file(fullFilePath);
        free(fullFilePath);
        free(tempPath);
    }
    free(chapterPath);
    free(toDelete->chapter);
    free(toDelete->file);
    toDelete->chapterName = toDelete->fileName = "Deleted";
}

//IMPLIMENT LATER TO CHANGE WHAT THE BLACKLIST HOLDS AND UNZIP AND DELETE FIRST DUPLICATE
void blacklist_handle_file(char *filePath, char *chapter, char *file) {
    enter_critical_code();
    if (!get_delete()) {
        return;
    }
    char *hashSum = calculate_hash(filePath);
    if ((hashSum == NULL) || (hashSum[0] == '\0')) {
        return;
    }
    blacklistEntry *newEntry = (blacklistEntry *) malloc(sizeof(blacklistEntry));
    if (newEntry == NULL) {
        exit(21);
    }
    newEntry->hashValue = hashSum;
    newEntry->chapterName = make_permenent_string(chapter);
    newEntry->fileName = make_permenent_string(file);
    blacklistEntry *foundEntry = dictionary_lookup(blacklist, newEntry);
    if (foundEntry != NULL) {
        delete_blacklisted_file(foundEntry);
        delete_file(filePath);
        free(newEntry->hashValue);
        free(newEntry->chapterName);
        free(newEntry->fileName);
        free(newEntry);
    } else {
        insert_node(blacklist, newEntry);
    }
    exit_critical_code();
}

void save_blacklist() {
    if (!get_delete()) {
        return;
    }
    char **blacklistToSave = get_array(blacklist);
    //write to blacklistLocation
    //loop through freeing everything in structs and struct too
    free(blacklistToSave);
    free_tree(blacklist, false);
}
