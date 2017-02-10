#include "hashMap.h"
#include "blacklist.h"
#include "generalMethods.h"
#include <stdlib.h>
#include <string.h>
#include "tmdl.h"
#include <unistd.h>
#include <pthread.h>
#include "chaptersToDownload.h"
#include <sys/wait.h>
#include "customParser.h"

#include <stdio.h>

hashMap *blacklist;
char *blacklistLocation;
bool hashFail = false;
pthread_t threadId;
bool threadOn = false;

int blacklist_comparator (const void *alpha, 
        const void *beta) {
    blacklistEntry *a = (blacklistEntry *) alpha;
    blacklistEntry *b = (blacklistEntry *) beta;
    int temp =  strcmp(a->hashValue, b->hashValue);
    return temp;
}

//make for a hexidecimal hash 64 bits and above
long blacklist_get_key(const void *alpha) {
    blacklistEntry *a = (blacklistEntry *) alpha;
    //make a key!
    //would be nice to process all the charecters, but honestly :/ no point
    //see we only have 64 bit memory any way, getting a key 256 bits accurate
    //is pointless
    long temp = parse_hexadecimal_to_one_long(a->hashValue);    
    return temp;
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
    size_t dynamic = 4, count = 0;
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
    return loadedList;
}

void load_blacklist() {
    if (!get_delete()) {
        return;
    }
    blacklistLocation = concat(get_series_folder(), ".blacklist.sha256");
    bool existance = (access(blacklistLocation, F_OK) != -1) 
            && is_file(blacklistLocation);
    if (!existance) {
        blacklist = new_hash_map(blacklist_comparator, blacklist_get_key);
        if (get_verbose()) {
            puts("New blacklist being created");
        }
    } else {
        FILE *blacklistFile = fopen(blacklistLocation, "r");
        if (blacklistFile == NULL) {
            exit(3);
        }
        blacklistEntry **loadedList = read_blacklist(blacklistFile);
        blacklist = hash_map_construction((void **) loadedList, 
                get_pointer_array_length((void **) loadedList), 
                blacklist_comparator, blacklist_get_key);
        free(loadedList);
        fclose(blacklistFile);
        if (get_verbose()) {
            puts("Loaded saved blacklist");
        }
    }
}

void *internal_load_blacklist(void *useless) {
    load_blacklist();
    return NULL;
}

void threaded_load_blacklist() {
    join_threaded_blacklist();
    if (!get_delete()) {
        return;
    }
    pthread_create(&threadId, NULL, internal_load_blacklist, NULL); 
    threadOn = true;
}

void join_threaded_blacklist() {
    if (!threadOn) {
        return;
    }
    pthread_join(threadId, NULL);
    threadOn = false;
}

//if fails say blacklist not being used once and then move on
char *calculate_hash(char *filePath) {
    int fds[2];
    pipe(fds);
    pid_t pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        dup2(fds[1], 1);
        close(fds[0]), close(2);
        execlp("shasum", "shasum", "-U" , "-a", "256", filePath, NULL);
        exit(24);
    }
    //parent
    close(fds[1]);
    bool errorOccured = false;
    int status;
    if ((waitpid(pid, &status, 0) == -1) || (WIFEXITED(status) == 0)) {
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
        FILE *hashSumStream = fdopen(fds[0], "r");
        if (hashSumStream == NULL) {
            exit(21);
        }
        char *fileHash = read_from_file(hashSumStream, ' ', true);
        fclose(hashSumStream);
        return fileHash;
    }
}

bool chapter_is_zip(char *chapterLocation) {
    char *downloadedFileName = concat(chapterLocation, ".cbz");              
    char *fullPath = concat(get_series_folder(), downloadedFileName);
    //need some kind of check this isn't a directory
    bool isZip = ((access(fullPath, F_OK) != -1) && is_file(fullPath));
    free(fullPath);
    free(downloadedFileName);
    return isZip;
}

void delete_file_in_zip(char *zipPath, char *fileName) {
    pid_t pid = fork();
    if (pid == -1) {
        exit(22);
    } else if (pid == 0) {
        //child
        close(1), close(2);
        execlp("zip", "zip", "-dq", zipPath, fileName, NULL);
        exit(24);
    }
    //parent
    int status;
    if ((waitpid(pid, &status, 0) == -1) || (WIFEXITED(status) == 0)) {
        exit(21);
    }
    if (WEXITSTATUS(status) != 0) {
        if (get_verbose()) {
            fprintf(stderr, "Failed to delete %s from %s\n", fileName, zipPath);
        }
    }
}

void delete_blacklisted_file(blacklistEntry *toDelete) {
    if ((strcmp(toDelete->chapterName, "Deleted") == 0) || 
            (strcmp(toDelete->fileName, "Deleted") == 0)) {
        return;
    }
    char *chapterPath = concat(get_series_folder(), toDelete->chapterName); 
    if (chapter_is_zip(chapterPath)) {
        char *chapterZipLocation = concat(chapterPath, ".cbz");              
        delete_file_in_zip(chapterZipLocation, toDelete->fileName);
        free(chapterZipLocation);
    } else {
        char *tempPath = concat(chapterPath, "/");
        char *fullFilePath = concat(tempPath, toDelete->fileName);
        delete_file(fullFilePath);
        free(fullFilePath);
        free(tempPath);
    }
    free(chapterPath);
    free(toDelete->chapterName);
    free(toDelete->fileName);
    toDelete->chapterName = make_permenent_string("Deleted");
    toDelete->fileName = make_permenent_string("Deleted");
}

//needs fixing
void blacklist_handle_file(char *filePath, char *chapter, char *file) {
    join_threaded_blacklist();
    if (!get_delete()) {
        return;
    }
    //critical mode entering doesn't really matter any more tbh :/
    //cuz I save after each chapter, so I'm safe anyway, but meh...
    enter_critical_code();
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
    blacklistEntry *foundEntry = insert_item_into_map(blacklist, (void *)newEntry);
    if (foundEntry != NULL) {
        delete_blacklisted_file(foundEntry);
        delete_file(filePath);
        free(newEntry->hashValue);
        free(newEntry->chapterName);
        free(newEntry->fileName);
        free(newEntry);
    }
    exit_critical_code();
}
/*
void *internal_blacklist_handle_file(void *sentInfo) {
    char **fileInfo = (char **) sentInfo;
    blacklist_handle_file(fileInfo[0], fileInfo[1], fileInfo[2]);
    free(sentInfo);
    return NULL; 
}

void threaded_blacklist_handle_file(char *filePath, char *chapter, char *file) {
    join_threaded_blacklist();
    if (!get_delete()) {
        return;
    }
    char **toSend = (char **) malloc(sizeof(char *) * 3);
    if (toSend == NULL) {
        exit(21);
    }
    toSend[0] = filePath, toSend[1] = chapter, toSend[2] = file;
    pthread_create(&threadId, NULL, internal_blacklist_handle_file, 
            (void *) toSend); 
    threadOn = true;
}
*/
void save_blacklist(bool toFree) {
    if (!get_delete()) {
        return;
    }
    FILE *saveFile = fopen(blacklistLocation, "w");
    if (saveFile == NULL) {
        exit(3); 
    }
    blacklistEntry **blacklistToSave = (blacklistEntry **) 
            turn_map_into_array(blacklist);
    if (blacklistToSave == NULL || blacklistToSave[0] == NULL) {
        return;
    }
    int i = 0;
    blacklistEntry *currentEntry;
    while(currentEntry = blacklistToSave[i++], currentEntry != NULL) {
        fprintf(saveFile, "%s\n", currentEntry->hashValue);    
        fprintf(saveFile, "%s\n", currentEntry->chapterName);    
        fprintf(saveFile, "%s\n", currentEntry->fileName);    
        if (toFree) {
            free(currentEntry->hashValue);
            free(currentEntry->chapterName);
            free(currentEntry->fileName);
            free(currentEntry);
        }
    }
    free(blacklistToSave);
    fclose(saveFile);
    if (toFree) {
        free_map(blacklist);
    }
}

void *internal_save_blacklist(void *toFree) {
    bool toSend = *(bool *) toFree;
    save_blacklist(toSend);
    free(toFree);
    return NULL;
}

void threaded_save_blacklist(bool toFree) {
    join_threaded_blacklist();
    if (!get_delete()) {
        return;
    }
    bool *toSend = (bool *) malloc(sizeof(bool));
    if (toSend == NULL) {
        exit(21);
    }
    *toSend = toFree;
    pthread_create(&threadId, NULL, internal_save_blacklist, (void *) toSend); 
    threadOn = true;
}
