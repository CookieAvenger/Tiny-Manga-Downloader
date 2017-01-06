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

char *get_file_name(char *url) {
    char *fileName = rstrstr(url, "/");
    if (fileName == NULL) {
        //Pretty much immpossible, will deal with later
        fileName = "Unknown";
    }
    char *fullPathName = concat(temporaryFolder, fileName);
    return fullPathName;
}

void process_and_download_urls(char **pictureUrls) {
    int numberOfUrls = get_string_array_length(pictureUrls);
    for (int i = 0; i < numberOfUrls; i++) {
        download_file(pictureUrls[i], get_file_name(pictureUrls[i]));
    }
    exit(0);
    //get file name and content null - say failed to get here
    //work out what failed is in curl
    //free while doing
    //also if verbose put progress here
    //getting i pick out of number in chapter # out of this many chapters for this manga
    //Manually working out file name isn't working out - see if curl can help
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
