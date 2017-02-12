#include "tmdl.h"
#include "chaptersToDownload.h"
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include "generalMethods.h"
#include <stdio.h>
#include <string.h>

void* write_script(char *name, char *script) {
    char *scriptLocation = concat(get_series_folder(), name);
    FILE *newScript = fopen(scriptLocation, "w");
    if (newScript == NULL) {
        exit(3);
    }
    fprintf(newScript, "%s", script);
    fclose(newScript);
    return scriptLocation;
}

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
    char *scriptLocation = write_script(".unzip.sh", scriptToRun);
    free(folderToSend);
    free(scriptToRun);
    pid_t pid = fork();
    if (pid == -1) {
        remove(scriptLocation);
        exit(21);
    } else if (pid == 0) {
        //child
        close(1), close(2);
        execlp("sh", "sh", scriptLocation, NULL);
        exit(24);
    }
    //parent
    int status;
    if ((waitpid(pid, &status, 0) == -1) || (WIFEXITED(status) == 0)) {
        remove(scriptLocation);
        exit(21);
    }
    if (WEXITSTATUS(status) != 0) {
        //dunno what happned... exec probs failed
        remove(scriptLocation);
        exit(21);
    }
    remove(scriptLocation);
    free(scriptLocation);
}

void delete_similar_images() {
    pid_t pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        //close(1), close(2);
        char *folderToSend = make_bash_ready(get_series_folder());
        char *commandToRun = (char *) malloc(sizeof(char) * 
                (strlen(folderToSend) * 2 + 3 + 103));
        //+3 is for percentage match - 100 is max
        sprintf(commandToRun, "%s.fingerprints.fid -i 'VIEW(){ for f in "
                "\"$@\";do rm -f \\\"$f\\\" ;done }' "
                "-t %d%% -q -q -R %s", folderToSend,
                get_similarity_percentage(), folderToSend);
        execlp("findimagedupes", "findimagedupes", commandToRun, NULL);
        exit(24);
    }
    //parent
    int status;
    if ((waitpid(pid, &status, 0) == -1) || (WIFEXITED(status) == 0)) {
        exit(21);
    }
    if (WEXITSTATUS(status) != 0) {
        fputs("Image similary program failed, ensure "
                "that \"findimagedupes\" is installed on your system.", stderr); 
    }
}

void rezip_all_folders() {
    char *folderToSend = make_bash_ready(get_series_folder());
    char *scriptToRun = (char *) malloc(sizeof(char) *
            (strlen(folderToSend) + 102));
    sprintf(scriptToRun, "(cd %s && find . ! -path . -type d -prune -exec sh "
            "-c 'zip -jXrq \"$1.cbz\" \"$1\" && rm -rf \"$1\"' _ {} \\;)",
             folderToSend);
    char *scriptLocation = write_script(".zip.sh", scriptToRun);
    free(folderToSend);
    free(scriptToRun);
    pid_t pid = fork();
    if (pid == -1) {
        remove(scriptLocation);
        exit(21);
    } else if (pid == 0) {
        //child
        close(1), close(2);
        execlp("sh", "sh", scriptLocation, NULL);
        exit(24); 
    }
    //parent
    int status;
    if ((waitpid(pid, &status, 0) == -1) || (WIFEXITED(status) == 0)) {
        remove(scriptLocation);
        exit(21);
    }
    if (WEXITSTATUS(status) != 0) {
        //dunno what happned... exec probs failed
        remove(scriptLocation);
        exit(21);
    }
    remove(scriptLocation);
    free(scriptLocation);
}

void experimental_find_dupes() {
    if (!get_to_find_dupes()) {
        /*if (get_zip_approval()) {
            if (get_verbose()) {
                puts("Zipping any folders");
            }
            rezip_all_folders();
        } else {
            if (get_verbose()) {
                puts("Unzipping any comic book archives");
            }
            unzip_all_comic_book_archives();
        }*/
        return;
    }
    if (get_verbose()) {
        puts("Unzipping any comic book archives");
    }
    unzip_all_comic_book_archives();
    if (get_verbose()) {
        puts("Attemping to delete similar images, please be patient...");
    }
    delete_similar_images();
    if (get_zip_approval()) {
        if (get_verbose()) {
            puts("Rezipping all folders");
        }
        rezip_all_folders();
    }
}
