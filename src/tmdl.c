#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

bool verbose = false;
char* saveDirectory = NULL;

void terminate_handler(int signal) {
    //here we delete downloading directory - what if the zip is running?
    //gotta reap that child process first if it exists
    exit(9);
}

//Prints appropriate error to stderr before exit
void print_error(int err, void *notUsing) {
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
        case 9:
            fputs("Stopping prematurely\n", stderr);
            break;
        case 21:
            fputs("System error\n", stderr);
            break;
    }
}

bool getVerbose() {
    return verbose;
}

//Checks if arguments are correct and exits if otherwise
void argcheck(int argc, char** argv) {
    if (argc < 2 || argc > 4) {
        exit(1);
    }
    //run a check on arg[1] to connect to the site
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
                        saveDirectory = argv[i-1];
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
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            saveDirectory = cwd;
            if (access(saveDirectory, R_OK|W_OK) == -1) {
                exit(4);
            }
        } else {
           exit(21); 
        }
    }
}

int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);
    //ignore if execs fail - should change later
    signal(SIGCHLD, SIG_IGN);
    signal(SIGINT, terminate_handler);
    on_exit(print_error, NULL);
    argcheck(argc, argv);
}
