#include <stdlib.h>
#include "tmdl.h"
#include "generalMethods.h"
#include "currentChapter.h"
#include <string.h>
#include "blacklist.h"

//site being used in this instance of tmdl
Site source;
//Location of where to save manga
char *seriesFolder = NULL;
//Name of manga being downloaded in this instance of tmdl
char *mangaName = NULL;
//Start of chapter queue
ChapterQueue *head = NULL;
//End of chapter queue
ChapterQueue *tail = NULL;
//Counter of number of items pushed to queue
size_t fullLength = 0;
//Counter of number of items popped from queue
size_t overallPointer = 0;

//Return name of manga being downloaded
char *get_manga_name() {
    return mangaName;
}

//Return manga save location
char *get_series_folder() {
    return seriesFolder;
}

//Set manga save location
void set_series_folder(char *folder) {
    char *folderRevised = str_replace(folder, "\\", "|");
    mangaName = folderRevised;
    if (seriesFolder != NULL) {
        return;
    }
    if (folder == NULL) {
        char *path = concat(get_save_directory(), "/");
        seriesFolder = path;
    } else {
        char *path = concat(get_save_directory(), "/");
        if (strcmp(rstrstr(get_save_directory(), "/")+1,
                folderRevised) == 0) {
            seriesFolder = path;
        } else {
            char *fullFolder = concat(path, folderRevised);
            free(path);
            seriesFolder = concat(fullFolder, "/");
            free(fullFolder);
        }
    }
    create_folder(seriesFolder);
    //Start loading blacklist as soon as possible
    if (!get_using_settings()) {
        threaded_load_blacklist();
    }
}

//Set source we are downloading from
void set_source(Site domainUsed) {
    source = domainUsed;
}

//Get site currently using
Site get_source() {
    return source;
}

//Get number of items pushed onto queue
size_t get_download_length() {
    return fullLength;
}

//Get number of items popped from queue
size_t get_current_download_chapter() {
    return overallPointer;
}

//Add a chapter to the queue
void add_to_download_list(Chapter *toAdd) {
    ChapterQueue *new = (ChapterQueue *) malloc(sizeof(ChapterQueue));
    new->current = toAdd;
    new->next = NULL;
    if (head == NULL) {
        head = tail = new;
    } else {
        tail->next = new;
        tail = new;
    }
    fullLength++;
}

//Extract a chapter from the queue
Chapter *pop_from_download() {
    if (head == NULL) {
        return NULL;
    }
    Chapter *toDownload = head->current;
    ChapterQueue *newHead = head->next;
    free(head);
    head = newHead;
    if (head == NULL) {
        tail = NULL;
    }
    overallPointer++;
    return toDownload;
}

//Go through the queue and download every chapter
void download_entire_queue() {
    if (fullLength == 0) {
        threaded_save_blacklist(true, false);
        return;
    }
    Chapter *toDownload;
    while (toDownload = pop_from_download(), toDownload != NULL) {
        download_chapter(toDownload, source);
        free(toDownload->name);
        free(toDownload->link);
        free(toDownload);
    }
    fullLength = overallPointer = 0;
    head = tail = NULL;
}
