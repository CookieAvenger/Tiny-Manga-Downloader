#include "kissMangaDownload.h"
#include "tmdl.h"
#include <stdio.h>
#include "generalMethods.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "curl.h"

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
    char *downloadedFileName = concat(toCheck, ".cbr");
    char *fullPath = concat(get_series_folder(), downloadedFileName);
    free(downloadedFileName);
    bool existance = (access(fullPath, F_OK) != -1);
    free(fullPath);
    return existance;
}

void process_and_download_urls(char **pictureUrls) {
    download_a_file(pictureUrls[0]);
    exit(0);
    //get file name and content null - say failed to get here
    //free while doing
    //also if verbose put progress here
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
    //download into folder
    process_and_download_urls(pictureUrls);
    //start zip process
    string_array_free(pictureUrls);
    //wait for zip to end
    delete_folder(temporaryFolder);
}
