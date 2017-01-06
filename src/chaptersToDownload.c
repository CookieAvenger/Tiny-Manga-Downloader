#include "chaptersToDownload.h"
#include <stdlib.h>
#include "tmdl.h"
#include "generalMethods.h"

//Turn into a real queue later to save space

Site source;
char *seriesFolder;
Chapter **downloadArray;
//Malloced size of array
int dynamic = 0;
//Size of array used
int length = 0;
//Current point up to which we are up to
int pointer = 0;

void set_series_folder(char *folder) {
    char *path = concat(get_save_directory(), "/");
    seriesFolder = concat(path, folder);
    free(path);
}

void set_source(Site domainUsed) {
    source = domainUsed;
}

int get_download_length() {
    return length;
}

int get_current_download_number() {
    return pointer;
}

//After each pop free a chapter then
void free_download_array() {
    //this loop should never occure, but just in case
    for (int i = pointer; i < length; i++) {
       free(downloadArray[i]); 
    }
    free(downloadArray);
    pointer = length = dynamic = 0;
}

void add_to_download_list(Chapter *toAdd) {
    if (dynamic == 0) {
        dynamic = 4;
        downloadArray = (Chapter **) malloc(sizeof(Chapter *) * dynamic);
        if (downloadArray == NULL) {
            exit(21);
        }
    }
    length++;
    if (length == dynamic) {
        dynamic *= 2;
        downloadArray = (Chapter **) realloc(downloadArray, 
                sizeof(Chapter *) * dynamic);
        if (downloadArray == NULL) {
            exit(21);
        }
    }
    downloadArray[length - 1] = toAdd;
}

Chapter *pop_from_download() {
    if (pointer++ > length) {
        return NULL;
    } else {
        return downloadArray[pointer - 1];
    }
}

void free_chapter(Chapter *toFree) {
    free(toFree->name);
    free(toFree->link);
    free(toFree);
}
