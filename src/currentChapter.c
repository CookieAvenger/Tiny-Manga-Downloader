#include "kissMangaDownload.h"
#include "mangaSeeSupport.h"
#include "tmdl.h"
#include "generalMethods.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "networking.h"
#include <unistd.h>
#include <sys/wait.h>
#include "blacklist.h"
#include "experimental.h"

//Weather incompleteChapterFolder is set or not
bool folderSet = false;
//Folder current chapter images are being downloaded into
char *incompleteChapterFolder = NULL;

//Return last folder being downloaded into
char *get_incomplete_chapter_folder() {
    return incompleteChapterFolder;
}

//Set incompleteChapterFolder
void setup_incomplete_chapter_folder(char *folderName) {
    if (folderSet) {
        free(incompleteChapterFolder);
    } else {
        folderSet = true;
    }
    if (folderName == NULL) {
        incompleteChapterFolder = NULL;
        folderSet = false;
        return;
    }
    char *veryTemporary = concat(folderName, "/");
    incompleteChapterFolder = concat(get_series_folder(), veryTemporary);
    free(veryTemporary);
}

//Work out file extension from url name
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

//Figure out the image extenstion from parseing header
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

//Figures out file extension and renames the file
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
        rename(filePath, finalPath);
        free(finalPath);
    }
    char *finalName = concat(fileName, extension);
    free(extension);
    return finalName;
}

//Download every url in order and name it in that order
void process_and_download_urls(char **pictureUrls, Chapter *current) {
    size_t numberOfUrls = get_pointer_array_length((void **)pictureUrls);
    for (size_t i = 0; i < numberOfUrls; i++) {
        if (get_verbose()) {
            printf("\rDownloading Chapter %zu/%zu, page %zu/%zu",
                    get_current_download_chapter(), get_download_length(),
                    i+1, numberOfUrls);
            fflush(stdout);
        }
        char *fileNumber = size_to_string(i+1);
        char *numberFilePath = concat(incompleteChapterFolder, fileNumber);
        int curlSuccess = download_file(pictureUrls[i], numberFilePath);
        if (curlSuccess != 0) {
            free(fileNumber), free(numberFilePath), free(pictureUrls[i]);
            //Make this better at some point
            fprintf(stderr, "\nError downloading page %zu from %s\n", 
                    i+1, current->name);
            continue;
        }
        char *finalFileName = sort_out_file_extension(numberFilePath, 
                fileNumber, pictureUrls[i]);
        free(fileNumber), free(numberFilePath), free(pictureUrls[i]);
        char *finalFilePath = concat(incompleteChapterFolder, finalFileName);
        blacklist_handle_file(finalFilePath, current->name, 
                finalFileName);
        free(finalFileName), free(finalFilePath);
    }
    puts("");
    fflush(stdout);
    free(pictureUrls);
}

//Zip folder to comic book archive, or unzip comic book archive to folder
void zip_contents(char *name, bool unzip) {
    if (!unzip && is_directory_empty(incompleteChapterFolder)) {
        return;
    }
    if (get_verbose()) {
        char *action = "Zipping";
        if (unzip) {
            action = "Unzipping";
        }
        printf("%s chapter %s\n", action, name);
        fflush(stdout);
    } 
    char *zipConstructor = concat(get_series_folder(), name);
    char *tempZip = concat(zipConstructor, ".cbz");
    pid_t pid = fork();
    if (pid == -1) {
        exit(22);
    } else if (pid == 0) {
        //child
        close(0), close(1), close(2);
        //No point freeing anything, all gonna vanish at exec or exit anyway
        char *zipName = make_bash_ready(tempZip);
        char *folderName = make_bash_ready(incompleteChapterFolder);
        char *zipCommand = "zip -jXrq";
        char *unzipCommand = "unzip -qqo";
        char *optionalCommand = " ";
        if (unzip) {
            optionalCommand = " -d ";
        }
        char *commandToUse = zipCommand;
        if (unzip) {
            commandToUse = unzipCommand;
        }
        char *commandToRun = (char *) malloc(sizeof(char) * 
                (strlen(zipName) + strlen(folderName) + 
                strlen(commandToUse) + strlen(optionalCommand) + 2));
        if (commandToRun == NULL) {
            exit(21);
        }
        sprintf(commandToRun, "%s %s%s%s",
                commandToUse, zipName, optionalCommand, folderName);
        execlp("bash", "bash", "-c", commandToRun, NULL);
        exit(24);
    }
    //parent
    free(zipConstructor);
    int status;
    if ((waitpid(pid, &status, 0) == -1) || (WIFEXITED(status) == 0)) {
        exit(21);
    }
    if (WEXITSTATUS(status) != 0) {
        if (unzip) {
            exit(28);
        }
        exit(27);
    }
    if (!unzip) {
        delete_folder(incompleteChapterFolder, 1);
    } else {
        remove(tempZip);
    }
    free(tempZip);
}

//Check if a chapter exists and zip or unzip if necassary
bool chapterExists(char *toCheck) {
    char *downloadedFileName = concat(toCheck, ".cbz");
    char *fullPath = concat(get_series_folder(), downloadedFileName);
    char *alternativePath = concat(get_series_folder(), toCheck);
    free(downloadedFileName);
    bool existance = false;
    if (access(alternativePath, F_OK) != -1) {
        existance = true;
        if (access(fullPath, F_OK) != -1) {
            remove(fullPath);
        }
        if (get_zip_approval()) {
            zip_contents(toCheck, false);
        }
    } else if (access(fullPath, F_OK) != -1) {
        existance = true;
        if (!get_zip_approval()) {
            zip_contents(toCheck, true);
        }
    }
    free(fullPath), free(alternativePath);
    return existance;
}

//Start downloading a chapter
void download_chapter(Chapter *current, Site source) {
    setup_incomplete_chapter_folder(current->name);
    if (current->doneWith || chapterExists(current->name)) {
        if (get_verbose()) {
            printf("Skipping %s - already downloaded\n", current->name);
            fflush(stdout);
        }
        setup_incomplete_chapter_folder(NULL);
        return;
    }
    char **pictureUrls = NULL;
    if (source == kissmanga) {
        pictureUrls = setup_kissmanga_chapter(current);
    } else if (source == mangasee) {
        pictureUrls = setup_mangasee_chapter(current);
    }
    if ((pictureUrls == NULL) || (pictureUrls[0] == NULL)) {
        return;
    }
    create_folder(incompleteChapterFolder);
    //download into a folder
    process_and_download_urls(pictureUrls, current);
    //start zip process here
    if (get_zip_approval()) {
        zip_contents(current->name, false);
    }
    setup_incomplete_chapter_folder(NULL);
    //start saving blacklist
    //This is so no progress after a chapter is lost - but it's a bit overkill :/
    bool toFree = false;
    if (get_current_download_chapter() >= get_download_length()) {
        toFree = true;
    }
    threaded_save_blacklist(toFree, true);
    if (!get_verbose()) {
        printf("Downloaded %s: %s", get_manga_name(), current->name);
        //expect to write to file, so don't flush let system handle that
    }
    set_files_changed();
}
