#include <stdlib.h>
#include "tmdl.h"
#include "generalMethods.h"
#include "currentChapter.h"
#include <string.h>

#include <stdio.h>

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

int fullLength = 0;

int overallPointer = 0;

int time = 0;

char *get_series_folder() {
    return seriesFolder;
}

void set_series_folder(char *folder) {
    char *path = concat(get_save_directory(), "/");
    if (strcmp(rstrstr(get_save_directory(), "/")+1, folder) == 0) {
        seriesFolder = path;
    } else {
        char *fullFolder = concat(path, folder);
        free(path);
        seriesFolder = concat(fullFolder, "/");
        free(fullFolder);
    }
}

void set_source(Site domainUsed) {
    source = domainUsed;
}

int get_download_length() {
    return fullLength;
}

int get_current_download_chapter() {
    return overallPointer;
}

void free_chapter(Chapter *toFree) {
    free(toFree->name);
    free(toFree->link);
    free(toFree);
}

//After each pop free a chapter then
void free_download_array() {
    if (time++ == 1) {
        for (int i = 0; i < length; i++) {
           free_chapter(downloadArray[i]); 
        }
    }
    free(downloadArray);
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
    fullLength++;
}

Chapter *pop_from_download() {
    if (pointer++ >= length) {
        return NULL;
    } else {
        if ((length - pointer + 1) <= (dynamic / 4)) {
            dynamic -= (dynamic / 2);
            Chapter **newArray = (Chapter **) malloc (sizeof(Chapter *) 
                    * dynamic);
            memcpy(newArray, &downloadArray[pointer - 1], 
                    sizeof(Chapter **) * dynamic);
            free_download_array();
            //have to see if this length is right by adding stuff 
            // - will find out later
            length -= (pointer - 1);
            pointer = 1;
            downloadArray = newArray;
        }
        overallPointer++;
        return downloadArray[pointer - 1];
    }
}

void download_entire_queue() {
    if (length == 0) {
        return;
    }
    setup_temporary_folder();
    Chapter *current;
    while (current = pop_from_download(), current != NULL) {
        download_chapter(current, source);
    }
    free_download_array(); 
    pointer = length = dynamic = fullLength = overallPointer = time =  0;
}
