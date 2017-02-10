#include "tmdl.h"
#include "chaptersToDownload.h"
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include "generalMethods.h"
#include <stdio.h>
#include <string.h>

void unzip_all_comic_book_archives() {
    char *folderToSend = make_bash_ready(get_series_folder());
    size_t folderLength = strlen(folderToSend);
    pid_t pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        close(1), close(2);
        char *commandToRun = (char *) malloc(sizeof(char) * 
                (folderLength + 98));
        if (commandToRun == NULL) {
            exit(21);
        }
        sprintf(commandToRun, "' ( cd %s && find -name '\\''*.cbz'\\'' "
                "-exec sh -c '\\'' unzip -qqo -d \"${1%%.cbz}\" \"$1\"'\\'' _ {} \\;)'"
                , folderToSend);
        execlp("sh", "sh", "-c", commandToRun, NULL);
        exit(24);
    }
    //parent
    int status;
    if ((waitpid(pid, &status, 0) == -1) || (WIFEXITED(status) == 0)) {
        exit(21);
    }
    if (WEXITSTATUS(status) != 0) {
        //dunno what happned... exec probs failed
        exit(21);
    }
    pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        close(1), close(2);
        char *filesToDelete = concat(folderToSend, "*.cbz");
        execlp("rm", "rm", "-f", filesToDelete, NULL);
        exit(24);
    }
    if ((waitpid(pid, &status, 0) == -1) || (WIFEXITED(status) == 0)) {
        exit(21);
    }
    if (WEXITSTATUS(status) != 0) {
        //dunno what happned... exec probs failed
        exit(21);
    }
    free(folderToSend);
}

void delete_similar_images() {
    pid_t pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        close(1), close(2);
        char *folderToSend = make_bash_ready(get_series_folder());
        char *commandToRun = (char *) malloc(sizeof(char) * 
                (strlen(folderToSend) * 2 + 3 + 103));
        //+3 is for percentage match - 100 is max
        sprintf(commandToRun, "%s.fingerprints.fid -i 'VIEW(){ for f in "
                "\"$@\";do echo \\\"$f\\\" | xargs rm -f ;done }' "
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
    pid_t pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        char *folderToSend = make_bash_ready(get_series_folder());
        char *commandToRun = (char *) malloc(sizeof(char) * 
                (strlen(folderToSend) + 74));
        sprintf(commandToRun, "' ( cd %s && for i in */; do zip -r \"${i%%/}.zip\" "
                "\"$i\" | rm -rf \"$i\"; done)'", folderToSend);
        execlp("sh", "sh", "-c", commandToRun, NULL);
        exit(24); 
    }
    //parent
    int status;
    if ((waitpid(pid, &status, 0) == -1) || (WIFEXITED(status) == 0)) {
        exit(21);
    }
    if (WEXITSTATUS(status) != 0) {
        //dunno what happned... exec probs failed
        exit(21);
    }
}

void experimental_find_dupes() {
    if (!get_to_find_dupes()) {
        if (get_zip_approval()) {
            if (get_verbose()) {
                puts("Zipping any folders");
            }
            rezip_all_folders();
        } else {
            if (get_verbose()) {
                puts("Unzipping any comic book archives");
            }
            unzip_all_comic_book_archives();
        }
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
