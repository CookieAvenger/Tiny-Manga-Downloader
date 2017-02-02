#include "kissMangaDownload.h"
#include "tmdl.h"
#include "generalMethods.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "networking.h"
#include <unistd.h>
#include <sys/wait.h>
#include "blacklist.h"

char *temporaryFolder;

char *get_temporary_folder() {
    return temporaryFolder;
}

void setup_temporary_folder() {
    temporaryFolder = concat(get_series_folder(), "Downloading/");
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

char *back_up_file_extension_finder(char *url) {
    char *fileExtension = rstrstr(url, ".");
    if (fileExtension == NULL || fileExtension[1] == '\0' ||
            fileExtension[2] == '\0' || fileExtension[3] == '\0') {
        fileExtension = "\0";
    }
    //largest image file extensions are only 4 charecters - so 5 should be g
    char fileExtensionBuffer[5];
    snprintf(fileExtensionBuffer, 5, "%s", fileExtension);
    return make_permenent_string(fileExtensionBuffer);
}

char *work_out_file_extension(unsigned char *header) {
    char *extension = NULL;
    //kudos to https://github.com/inorichi/tachiyomi for this method
    if (header[0] == 'G' && header[1] == 'I' && header[2] == 'F' 
            && header[3] == '8') {
        extension = ".gif";                                               
    } else if (header[0] == (unsigned char) 0x89 && header[1] == (unsigned char) 0x50 
            && header[2] == (unsigned char) 0x4E && header[3] == (unsigned char) 0x47 
            && header[4] == (unsigned char) 0x0D && header[5] == (unsigned char) 0x0A
            && header[6] == (unsigned char) 0x1A && header[7] == (unsigned char) 0x0A) {        
        extension = ".png";                                               
    } else if (header[0] == (unsigned char) 0xFF && header[1] == (unsigned char) 0xD8 
            && header[2] == (unsigned char) 0xFF) {
        if ((header[3] == (unsigned char) 0xE0) || (header[3] == (unsigned char) 0xE1 
                && header[6] == 'E' && header[7] == 'x' && header[8] == 'i'
                && header[9] == 'f' && header[10] == 0)) {                
            extension = ".jpeg";                                          
        } else if (header[3] == (unsigned char) 0xEE) {                           
            extension = ".jpg";                                           
        }                                                               
    }                                                                    
    if (extension != NULL) {
        extension = make_permenent_string(extension);
    }
    return extension;
}

char *sort_out_file_extension(char *filePath, char *fileName, char *url) {
    FILE *image = fopen(filePath, "r");
    if (image == NULL) {
        //man these error messages are crap need rehaul
        exit(3);
    }
    //this is not going to be null terminated
    unsigned char *fileHeader = (unsigned char *) malloc(sizeof(unsigned char) * 64);
    int readValue = fread(fileHeader, sizeof(unsigned char), 64, image);
    if (readValue == 0) {
        //we know the file exsists and have open it so wth
        exit(21);
    }
    char *extension = work_out_file_extension(fileHeader);
    free(fileHeader);
    fclose(image);
    if (extension == NULL) {
        extension = back_up_file_extension_finder(url); 
    }
    if (extension[0] != '\0') {
        char *finalPath = concat(filePath, extension);
        move_file(filePath, finalPath);
        free(finalPath);
    }
    char *finalName = concat(fileName, extension);
    free(extension);
    return finalName;
}

void process_and_download_urls(char **pictureUrls, Chapter *current) {
    unsigned long numberOfUrls = get_string_array_length(pictureUrls);
    for (unsigned long i = 0; i < numberOfUrls; i++) {
        if (get_verbose()) {
            printf("\rDownloading Chapter %lu/%lu, page %lu/%lu",
                    get_current_download_chapter(), get_download_length(),
                    i+1, numberOfUrls);
            fflush(stdout);
        }
        char *fileNumber = unsigned_long_to_string(i+1);
        char *numberFilePath = concat(temporaryFolder, fileNumber);
        int curlSuccess = download_file(pictureUrls[i], numberFilePath);
        //int curlSuccess = download_file(pictureUrls[i], get_file_name(pictureUrls[i], i));
        if (curlSuccess != 0) {
            //make this better at some point
            fprintf(stderr, "Error downloading page %lu from %s\n", i+1, current->name);
        }
        char *finalFileName = sort_out_file_extension(numberFilePath, fileNumber, pictureUrls[i]);
        free(fileNumber);
        free(numberFilePath);
        char *finalFilePath = concat(temporaryFolder, finalFileName);
        blacklist_handle_file(finalFilePath, current->name, finalFileName);
        free(finalFileName);
        free(finalFilePath);
        free(pictureUrls[i]);
    }
    puts("");
    free(pictureUrls);
}

void copy_contents(char *toMoveTo, char *contentsToMove) {
    int pid = fork();
    if (pid == -1) {
        exit(22);
    } else if (pid == 0) {
        close(1), close(2);
        //child
        if (get_zip_approval()) {
            char *commandToRun = (char *) malloc(sizeof(char) * 
                    (strlen(toMoveTo) + strlen(contentsToMove) + 11));
            if (commandToRun == NULL) {
                exit(21);
            }
            sprintf(commandToRun, "zip -jXq %s %s", toMoveTo, contentsToMove);
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
        printf("Moving chapter %s\n", current->name);
        fflush(stdout);
    } 
    char *finalMoveTo, *contents;
    char *moveToConstructor = concat(get_series_folder(), current->name);
    if (get_zip_approval()) {
        //zip
        char *tempFinalMoveTo = concat(moveToConstructor, ".cbz");
        char *tempContents = make_bash_ready(get_temporary_folder());
        finalMoveTo = make_bash_ready(tempFinalMoveTo);
        contents = concat(tempContents, "*");
        free(tempFinalMoveTo), free(tempContents);
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
            printf("Skipping %s - already downloaded\n", current->name);
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
    //start saving blacklist
    threaded_save_blacklist(false);
    //start zip process here
    move_downloaded(current);
    //join save thread here
    join_threaded_blacklist();
}
