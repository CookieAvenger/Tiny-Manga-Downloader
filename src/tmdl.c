#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "tmdl.h"
#include "kissMangaDownload.h"
#include "generalMethods.h"
#include "chaptersToDownload.h"
#include "currentChapter.h"
#include <sys/wait.h>

bool verbose = false;
bool zip = false;
char *saveDirectory = NULL;
char *domain;
char *seriesPath;

bool get_zip_approval() {
    return zip;
}

char *get_series_path() {
    return seriesPath;
}

bool get_verbose() {
    return verbose;
}

char *get_save_directory() {
    return saveDirectory;
}

char *get_domain() {
    return domain;
}

void terminate_handler(int signal) {
    exit(9);
}

//Prints appropriate error to stderr before exit
void print_error(int err, void *notUsing) {
    //don't care if wait fails, should fail most of the time in fact
    int status;
    wait(&status);
    delete_folder(get_temporary_folder(), -1);
    switch(err) {
        case 1:
            fputs("Usage: tmdl url [savelocation] [-v]\n", stderr);
            break;
        case 2:
            fputs("Directory does not exist", stderr);
            break;
        case 3:
            fputs("Do not have nessacary read and write permissions ", stderr);
            fputs("in save location\n", stderr);
            break;
        case 4:
            fputs("Do not have nessacary read and write permissions ", stderr);
            fputs("in current directory\n", stderr);
            break;
        case 5:
            fprintf(stderr, "%s is an invalid domain\n", domain);
            break;
        case 6:
            fputs("Invalid series location\n", stderr);
            break;
        case 9:
            fputs("\nStopping prematurely\n", stderr);
            break;
        case 21:
            fputs("System error\n", stderr);
            break;
        case 22:
            fputs("Network error\n", stderr);
            break;
        case 23:
            fputs("Unkown error parsing cookie information\n", stderr);
            break;
        case 24:
            //Not handled at all - exec failed
            break;
        case 25:
            //Handled elsewhere - cookie script failed
            break;
        case 26:
            fputs("Webpage parsing error\n", stderr);
            break;
        case 27:
            fputs("Zipping failed, try with storing in folders instead\n",
                    stderr);
            break;
        case 28:
            fputs("Failed to copy to new folder\n", stderr);
            break;
    }
}

//Checks if arguments are correct and exits if otherwise
Site argument_check(int argc, char** argv) {
    if (argc < 2 || argc > 4) {
        exit(1);
    }
    Site domainUsed = other; 
    //run a check on arg[1] to connect to the site only supporting kiss rn 
    char *domainCheck = strstr(argv[1], "kissmanga");
    if (domainCheck != NULL) {
        //We are using going with kissmanga
        domainUsed = kissmanga;
        seriesPath = strstr(domainCheck, "/");
        if (seriesPath == NULL) {
            exit(6);
        }
        size_t charectersInDomain = seriesPath - domainCheck;
        domain = (char *) malloc(sizeof(char) * (charectersInDomain + 1));
        if (domain == NULL) {
            exit(21);
        }
        strncpy(domain, domainCheck, charectersInDomain);
        domain[charectersInDomain] = '\0';
    } else {
        //put other domain checks here
        exit(5);
    }
    if (argc >= 3) {
        for (int i = 3; i <= argc; i++) {
            if (!verbose && ((strcmp(argv[i-1], "-v") == 0) ||
                    (strcmp(argv[i-1], "-V") == 0))) {
                //if = -v set verbose
                verbose = true;
            } else if (saveDirectory == NULL){
                //other wise check location exists
                //if not exit
                if (access(argv[i-1], F_OK) != -1) {
                    if (access(argv[i-1], W_OK|R_OK) != -1) {
                        saveDirectory = realpath(argv[i-1], NULL);
                    } else {
                        exit(3);
                    }
                } else {
                    exit(2);
                }
            } else {
                exit(1);
            }
        }
    }
    if (saveDirectory == NULL) {
        //reimpliment getcwd so you don't have to use a buffer
        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            saveDirectory = make_permenent_string(cwd);
            if (access(saveDirectory, R_OK|W_OK) == -1) {
                exit(4);
            }
        } else {
           exit(21); 
        }
    }
    return domainUsed;
}

int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, terminate_handler);
    on_exit(print_error, NULL);
    Site domainUsed = argument_check(argc, argv);
    set_source(domainUsed);
    if (domainUsed == kissmanga) {
        setup_kissmanga_download();
    } 
    download_entire_queue();
    return 0;
}
