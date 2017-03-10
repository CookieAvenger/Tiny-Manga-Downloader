#include "tmdl.h"
#include "chaptersToDownload.h"
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include "generalMethods.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

//Weather or not any files were actually changed this run
bool filesChanged = false;
//Weather or not duplication checking started
bool processStarted = false;
//Location of last used script
char *scriptLocation = NULL;

//Get last script location
char *get_bash_script_location() {
    return scriptLocation;
}

//Return if duplication has started or not
bool get_dupe_started() {
    return processStarted;
}

//Allow dupe check to occur
void set_files_changed() {
    filesChanged = true;
}

//write a bash script to file
void* write_script(char *name, char *script, bool fullNameGiven) {
    char *writingScriptLocation = name;
    if (!fullNameGiven) {
        writingScriptLocation = concat(get_series_folder(), name);
    }
    FILE *newScript = fopen(writingScriptLocation, "w");
    if (newScript == NULL) {
        exit(3);
    }
    fprintf(newScript, "%s\n", script);
    fflush(newScript);
    fclose(newScript);
    return writingScriptLocation;
}

char *execute_script(char *scriptFile, int error, bool toPipe, int endRead, bool keepOutput) {
    scriptLocation = scriptFile;
    pid_t pid = fork();
    int fds[2];
    if (toPipe) {
        if (pipe(fds) == -1) {
            exit(21);
        }
    }
    if (pid == -1) {
        if (error != -1) {
            exit(error);
        }
    } else if (pid == 0) {
        //child
        if (toPipe) {
            dup2(fds[1], 1);
            close(fds[0]);
        } else {
            if (!keepOutput) {
                close(1);
            }
        }
        if (!keepOutput) {
            close(2);
        }
        close(0);
        execlp("sh", "sh", scriptLocation, NULL);
        exit(24);
    }
    //parent
    int status;
    if ((waitpid(pid, &status, 0) == -1) || (WIFEXITED(status) == 0)) {
        if (error != -1) {
            exit(error);
        }
    }
    char *toReturn = NULL;
    if (toPipe) {
        close(fds[1]);
        FILE *toRead = fdopen(fds[0], "r");
        if (toRead == NULL) {
            exit(21);
        }
        if (endRead < -1) {
            endRead = EOF;
        }
        toReturn = read_from_file(toRead, endRead, false);
        if (keepOutput && toReturn != NULL) {
            printf("%s", toReturn);
            if (endRead != EOF) {
                char toOut; 
                while (toOut = fgetc(toRead), toOut != EOF) {
                    printf("%c", toOut);
                }
            }
        }
        fclose(toRead);
    }
    if (WEXITSTATUS(status) != 0) {
        //dunno what happened... exec probs failed
        if (error != -1) {
            exit(error);
        }
    }
    remove(scriptLocation);
    char *toFree = scriptLocation;
    scriptLocation = NULL;
    free(toFree);
    return toReturn;
}

//Unzip all comic book archives
void unzip_all_comic_book_archives() {
    char *folderToSend = make_bash_ready(get_series_folder());
    size_t folderLength = strlen(folderToSend);
    char *scriptToRun = (char *) malloc(sizeof(char) *
            (folderLength + 95));
    if (scriptToRun == NULL) {
        exit(21);
    }
    sprintf(scriptToRun, "(cd %s && find -name '*.cbz' -exec sh -c 'unzip "
            "-qqo -d \"${1%%.cbz}\" \"$1\" && rm -f \"$1\"' _ {} \\;)", folderToSend);
    char *scriptFile = write_script(".unzip.sh", scriptToRun, false);
    free(folderToSend);
    free(scriptToRun);
    (void) execute_script(scriptFile, 21, false, EOF, false); 
}

//Find and delete similar images
void delete_similar_images() {
    pid_t pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        close(1), close(2);
        char *folderToSend = make_bash_ready(get_series_folder());
        char *commandToRun = (char *) malloc(sizeof(char) * 
                (strlen(folderToSend) * 2 + 3 + 115));
        //+3 is for percentage match - 100 is max
        sprintf(commandToRun, "findimagedupes -f %s.fingerprints.fid -i 'VIEW(){ for f in "
                "\"$@\";do echo \\\"$f\\\" | xargs rm -f ;done }' "
                "-t %d%% -q -q -R %s", folderToSend,
                get_similarity_percentage(), folderToSend);
        execlp("bash", "bash", "-c", commandToRun, NULL);
        exit(24);
    }
    //parent
    processStarted = true;
    int status;
    if ((waitpid(pid, &status, 0) == -1) || (WIFEXITED(status) == 0)) {
        exit(21);
    }
    if (WEXITSTATUS(status) != 0) {
        fputs("Image similary program failed, ensure that "
                "\"findimagedupes\" is installed on your system.\n", stderr); 
    }
}

//Zip all non empty folders
void rezip_all_folders() {
    char *folderToSend = make_bash_ready(get_series_folder());
    char *scriptToRun = (char *) malloc(sizeof(char) *
            (strlen(folderToSend) + 114));
    sprintf(scriptToRun, "(cd %s && find . ! -path . -type d -not -empty -prune"
            " -exec sh -c 'zip -jXrq \"$1.cbz\" \"$1\" && "
            "rm -rf \"$1\"' _ {} \\;)", folderToSend);
    char *scriptFile = write_script(".zip.sh", scriptToRun, false);
    free(folderToSend);
    free(scriptToRun);
    (void) execute_script(scriptFile, 24, false, EOF, false);
    processStarted = false;
}

//Run the duplication check program if allowed
void experimental_find_dupes() {
    if (!get_to_find_dupes() || !filesChanged || get_series_folder() == NULL) {
        return;
    }
    if (get_verbose()) {
        puts("Unzipping any comic book archives");
        fflush(stdout);
    }
    unzip_all_comic_book_archives();
    if (get_verbose()) {
        puts("Attemping to delete similar images, please be patient...");
        fflush(stdout);
    }
    delete_similar_images();
    if (get_zip_approval()) {
        if (get_verbose()) {
            puts("Rezipping all folders");
            fflush(stdout);
        }
        rezip_all_folders();
    }
}
