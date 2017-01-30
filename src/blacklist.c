#include "avlTree.h"
#include "blacklist.h"
#include "generalMethods.h"
#include <stdlib>
#include <string.h>
#include "tmdl.h"

avlTree *blacklist;
char *blacklistLocation;
bool hashFail = false;

void load_blacklist() {
    if (!get_delete()) {
        return;
    }
    blacklistLocation = concat(get_series_folder, ".blacklist.sha256");
    FILE *blacklist = fopen(blacklistLocation, "r");
    if (blacklist == NULL) {
        blacklist = (avlTree *) malloc(sizeof(avlTree));
        blacklist->size = 0;
        blacklist->comparator = strcmp;
        if (verbose) {
            
        }
        if (get_verbose()) {
            puts("New blacklist being created");
        }
        //if verbose say new blacklist created
    } else {
        char **seperatedFile = read_entire_file(source, seperator, true);
        blacklist = sorted_construction(seperatedFile, strcmp);
        free(seperatedFile);
        if (get_verbose()) {
            puts("loaded saved blacklist");
        }
    }
    fclose(blacklist);
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
        execlp("sha256sum", "sha256", filePath, NULL);
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

//IMPLIMENT LATER TO CHANGE WHAT THE BLACKLIST HOLDS AND UNZIP AND DELETE FIRST DUPLICATE
bool blacklist_handle_file(char *filePath) {
    if (!get_delete()) {
        return false;
    }
    char *hashSum = calculate_hash(filePath);
    if (hashSum == NULL) {
        return false;
    }
    if (!insert_node(blacklist, hashSum)) {
        delete_file(filePath);
        return true;
    }
    return false;
}

void save_blacklist() {
    if (!get_delete()) {
        return;
    }
    char **blacklistToSave = get_array(blacklist);
    //write to blacklistLocation
    free(blacklistToSave);
    free_tree(blacklist, true);
}
