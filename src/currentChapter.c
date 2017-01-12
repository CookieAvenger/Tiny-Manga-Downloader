#include "kissMangaDownload.h"
#include "tmdl.h"
#include <stdio.h>
#include "generalMethods.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "networking.h"
#include <unistd.h>
#include <sys/wait.h>

char *temporaryFolder;

char *get_temporary_folder() {
    return temporaryFolder;
}

void setup_temporary_folder() {
    char *fullFolder = concat(get_series_folder(), "Downloading");
    temporaryFolder = concat(fullFolder, "/");
    free(fullFolder);
}

bool chapterExists(char *toCheck) {
    char *downloadedFileName = concat(toCheck, ".cbz");
    char *fullPath = concat(get_series_folder(), downloadedFileName);
    char *alternativePath = concat(get_series_folder(), toCheck);
    free(downloadedFileName);
    bool existance = ((access(fullPath, F_OK) != -1) || 
            (access(alternativePath, F_OK) != -1));
    free(fullPath), free(alternativePath);
    return existance;
}

char *get_file_name(char *url, int fileNameCounter) {
    fileNameCounter += 1;
    char *fileExtension = rstrstr(url, ".");
    if (fileExtension == NULL || fileExtension[1] == '\0' ||
            fileExtension[2] == '\0' || fileExtension[3] == '\0') {
        //workout later using file command and rename the downloaded file
        //for now do nothing
        fileExtension = "\0";
    }
    char fileExtensionBuffer[5];
    snprintf(fileExtensionBuffer, 5, "%s", fileExtension);
    //try not to use a buffer :/
    char fileName [16];
    sprintf(fileName, "%d%s", fileNameCounter, fileExtensionBuffer);
    char *fullPathName = concat(temporaryFolder, fileName);
    return fullPathName;
}

void process_and_download_urls(char **pictureUrls, Chapter *current) {
    int numberOfUrls = get_string_array_length(pictureUrls);
    for (int i = 0; i < numberOfUrls; i++) {
        if (get_verbose()) {
            printf("\rDownloading Chapter %d/%d, page %d/%d",
                    get_current_download_chapter(), get_download_length(),
                    i+1, numberOfUrls);
            fflush(stdout);
        }
        int curlSuccess = download_file(pictureUrls[i], get_file_name(pictureUrls[i], i));
        if (curlSuccess != 0) {
            //make this better at some point
            fprintf(stderr, "Error downloading page %d from %s", i+1, current->name);
        }
        free(pictureUrls[i]);
    }
    free(pictureUrls);
    //also if verbose put progress here
    //getting i pick out of number in chapter # out of this many chapters for this manga
    //Manually working out file name isn't working out - see if curl can help
}

void copy_contents(char *toMoveTo, char *contentsToMove) {
    int pid = fork();
    if (pid == -1) {
        exit(22);
    } else if (pid == 0) {
        //close(0), close(1), close(2);
        //child
        if (get_zip_approval()) {
            char *commandToRun = (char *) malloc(sizeof(char) * 
                    (strlen(toMoveTo) + strlen(contentsToMove) + 8));
            if (commandToRun == NULL) {
                exit(21);
            }
            sprintf(commandToRun, "\"zip %s %s\"", toMoveTo, contentsToMove);
            execlp("bash", "bash", "-c", commandToRun, NULL);
        } else {
            execlp("mv", "mv", contentsToMove, toMoveTo, NULL);
        }
        exit(24);
    }
    //parent
    int status;
    if ((wait(&status) == -1) || (WIFEXITED(status) == 0)) {
        exit(21);
    }
    if (WEXITSTATUS(status) != 0) {
        if (get_zip_approval()) {
            exit(27);
        } else {
            exit(28);
        }
    }
}

void move_downloaded(Chapter *current) {
    if (get_verbose()) {
        printf("\rMoving chapter %s", current->name);
        fflush(stdout);
    } 
    char *finalMoveTo, *contents;
    char *moveToConstructor = concat(get_series_folder(), current->name);
    if (get_zip_approval()) {
        //zip
        finalMoveTo = concat(moveToConstructor, ".cbz");
        contents = concat(get_temporary_folder(), "*");
    } else {
        //move
        finalMoveTo = concat(moveToConstructor, "/");
        contents = get_temporary_folder();
    }
    free(moveToConstructor);;
    copy_contents(finalMoveTo, contents);
    free(finalMoveTo);
    if (get_zip_approval()) {
        free(contents);
        delete_folder(temporaryFolder, 1);
    }
}

void download_chapter(Chapter *current, Site source) {
    if (chapterExists(current->name)) {
        if (get_verbose()) {
            printf("\rSkipping %s - already downloaded", current->name);
        }
        return;
    }
    char **pictureUrls;
    if (source == kissmanga) {
        pictureUrls = setup_kissmanga_chapter(current);
    }
    if ((pictureUrls == NULL) || (pictureUrls[0] == NULL)) {
        return;
    }
    create_folder(temporaryFolder);
    //download into a folder
    process_and_download_urls(pictureUrls, current);
    //start zip process here
    move_downloaded(current);
}
