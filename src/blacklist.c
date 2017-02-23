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
#include <limits.h>
#include <stdio.h>

//Data structure containing all blacklisted files
hashMap *blacklist = NULL;
//File path of saved blacklist
char *blacklistLocation;
//Weather attempt at hashing has failed before
bool hashFail = false;
//Currently running blacklist thread id
pthread_t threadId;
//Weather a thread is currently running at all
bool threadOn = false;

//Returns comparasion of hash values stored in blacklist structs
int blacklist_comparator (const void *alpha, 
        const void *beta) {
    blacklistEntry *a = (blacklistEntry *) alpha;
    blacklistEntry *b = (blacklistEntry *) beta;
    int temp =  strcmp(a->hashValue, b->hashValue);
    return temp;
}

//Returns a key for a blacklist based on the first 64 bits of the hash value
long blacklist_get_key(const void *alpha) {
    blacklistEntry *a = (blacklistEntry *) alpha;
    unsigned long unsignedKey = parse_hexadecimal_to_one_long(a->hashValue);
    signed long signedKey = unsignedKey - LONG_MAX;
    return signedKey;
}

//Parse a single blacklist entry from file
blacklistEntry *read_single_entry(FILE *blacklistFile) {
    char *readHashValue = read_from_file(blacklistFile, '\n', true);
    if (readHashValue == NULL) {
        return NULL;
    } else if (readHashValue[0] == '#' || readHashValue[0] == '\0') {
        //Skips blank lines and comment lines after a single entry
        free(readHashValue);
        return read_single_entry(blacklistFile);
    }
    char *readChapterName = read_from_file(blacklistFile, '\n', true);
    //Can get readChapterName[0] == '\0', means file has been deleted
    char *readFileName = read_from_file(blacklistFile, '\n', true);
    blacklistEntry *newEntry = (blacklistEntry *) malloc(sizeof(blacklistEntry)); 
    if (newEntry == NULL) {
        exit(21);
    }
    newEntry->hashValue = readHashValue;
    newEntry->chapterName = readChapterName;
    newEntry->fileName = readFileName;
    return newEntry;
}

//Parses entire blacklist file
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

//Attemp to load a blacklist from file
void load_blacklist() {
    if (!get_delete()) {
        return;
    }
    blacklistLocation = concat(get_series_folder(), ".blacklist.sha256");
    bool existance = (access(blacklistLocation, F_OK) != -1) 
            && is_file(blacklistLocation);
    if (existance) {
        //Loading blacklist
        FILE *blacklistFile = fopen(blacklistLocation, "r");
        if (blacklistFile == NULL) {
            exit(3);
        }
        blacklistEntry **loadedList = read_blacklist(blacklistFile);
        //loadedList can never == NULL
        if (loadedList[0] != NULL) {
            blacklist = hash_map_construction((void **) loadedList, 
                    get_pointer_array_length((void **) loadedList), 
                    blacklist_comparator, blacklist_get_key);
            free(loadedList);
            fclose(blacklistFile);
            if (get_verbose()) {
                puts("Loaded saved blacklist");
                fflush(stdout);
            }
            return;
        } else {
            free(loadedList);
        }
    }
    //No blacklist available
    blacklist = new_hash_map(blacklist_comparator, blacklist_get_key);
    if (get_verbose()) {
        puts("New blacklist being created");
        fflush(stdout);
    }
}

//Wrapper on load_blacklist() for thread creation
void *internal_load_blacklist(void *useless) {
    load_blacklist();
    return NULL;
}

//Start a thread to load the blacklist
void threaded_load_blacklist() {
    join_threaded_blacklist();
    if (!get_delete()) {
        return;
    }
    pthread_create(&threadId, NULL, internal_load_blacklist, NULL); 
    threadOn = true;
}

//Join any running blacklist thread
void join_threaded_blacklist() {
    if (!threadOn) {
        return;
    }
    pthread_join(threadId, NULL);
    threadOn = false;
}

//Also tried it with popen and pclose, but will use that for something
//that actually invokes sh -c '%s'
//Other problem with that is I can't close(2);
//Calculate sha256 hash of a particular file
char *calculate_hash(char *filePath) {
    int fds[2];
    int errCheck = pipe(fds);
    if (errCheck == -1) {
        exit(21);
    }
    pid_t pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        dup2(fds[1], 1);
        close(0), close(fds[0]), close(2);
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
    if (!errorOccured && WEXITSTATUS(status) != 0) {
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

//Check if file path is a comic book archive
bool chapter_is_zip(char *chapterLocation) {
    char *downloadedFileName = concat(chapterLocation, ".cbz");              
    char *fullPath = concat(get_series_folder(), downloadedFileName);
    bool isZip = ((access(fullPath, F_OK) != -1) && is_file(fullPath));
    free(fullPath);
    free(downloadedFileName);
    return isZip;
}

//Tries to delete a file from a zip
void remove_in_zip(char *zipPath, char *fileName) {
    pid_t pid = fork();
    if (pid == -1) {
        exit(22);
    } else if (pid == 0) {
        //child
        close(0), close(1), close(2);
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

//Deletes a file described in a blacklist entry struct
void delete_blacklisted_file(blacklistEntry *toDelete) {
    //Any of these conditions can mean deleted
    if ((toDelete->chapterName[0] == '\0') || (toDelete->fileName[0] == '\0')
        || (strncmp(toDelete->fileName, "Deleted", 7) == 0)
        || (strncmp(toDelete->chapterName, "Deleted", 7) == 0)) {
        return;
    }
    char *chapterPath = concat(get_series_folder(), toDelete->chapterName); 
    if (chapter_is_zip(chapterPath)) {
        char *chapterZipLocation = concat(chapterPath, ".cbz");              
        remove_in_zip(chapterZipLocation, toDelete->fileName);
        free(chapterZipLocation);
    } else {
        char *tempPath = concat(chapterPath, "/");
        char *fullFilePath = concat(tempPath, toDelete->fileName);
        remove(fullFilePath);
        free(fullFilePath);
        free(tempPath);
    }
    free(chapterPath); 
    if (get_verbose()) {
        char *deletedFileName = (char *) malloc(sizeof(char) *
                (9 + strlen(toDelete->fileName)));
        sprintf(deletedFileName, "Deleted %s", toDelete->fileName);
        free(toDelete->fileName);
        toDelete->fileName = deletedFileName;
    } else {
        free(toDelete->fileName), free(toDelete->chapterName);
        toDelete->chapterName = make_permenent_string("");
        toDelete->fileName = make_permenent_string("");
    }
}

//Turn file information into a blacklist entry and delete if duplicate
void blacklist_handle_file(char *filePath, char *chapter, char *file) {
    join_threaded_blacklist();
    if (!get_delete() || blacklist == NULL) {
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
    blacklistEntry *foundEntry = insert_item_into_map(blacklist, (void *)newEntry);
    if (foundEntry != NULL) {
        delete_blacklisted_file(foundEntry);
        remove(filePath);
        free(newEntry->hashValue);
        free(newEntry->chapterName);
        free(newEntry->fileName);
        free(newEntry);
    }
}

//Saves backlist information to file and/or free blacklist data
void save_blacklist(bool toFree, bool toSave) {
    if (!get_delete() || blacklist == NULL) {
        return;
    }
    FILE *saveFile;
    if (toSave) {
        saveFile = fopen(blacklistLocation, "w");
        if (saveFile == NULL) {
            exit(3);
        }
        fputs("#DO NOT MANUALLY EDIT THIS FILE\n", saveFile);
    }
    blacklistEntry **blacklistToSave = (blacklistEntry **) 
            turn_map_into_array(blacklist);
    if (blacklistToSave == NULL || blacklistToSave[0] == NULL) {
        return;
    }
    int i = 0;
    blacklistEntry *currentEntry;
    while(currentEntry = blacklistToSave[i++], currentEntry != NULL) {
        if (toSave) {
            fprintf(saveFile, "%s\n", currentEntry->hashValue);
            fprintf(saveFile, "%s\n", currentEntry->chapterName);
            fprintf(saveFile, "%s\n", currentEntry->fileName);
        }
        if (toFree) {
            free(currentEntry->hashValue);
            free(currentEntry->chapterName);
            free(currentEntry->fileName);
            free(currentEntry);
        }
    }
    free(blacklistToSave);
    if (toSave) {
        fflush(saveFile);
        fclose(saveFile);
    }
    if (toFree) {
        free_map(blacklist);
    }
}

//Wrapper for save_blacklist for thread creation
void *internal_save_blacklist(void *toSend) {
    bool *infoSent = (bool *) toSend;
    bool toFree = infoSent[0], toSave = infoSent[1];
    free(toSend);
    save_blacklist(toFree, toSave);
    return NULL;
}

//Create a threaded blacklist
void threaded_save_blacklist(bool toFree, bool toSave) {
    join_threaded_blacklist();
    if (!get_delete() || blacklist == NULL) {
        return;
    }
    bool *toSend = (bool *) malloc(sizeof(bool) * 2);
    if (toSend == NULL) {
        exit(21);
    }
    toSend[0] = toFree, toSend[1] = toSave;
    pthread_create(&threadId, NULL, internal_save_blacklist, (void *) toSend); 
    threadOn = true;
}
