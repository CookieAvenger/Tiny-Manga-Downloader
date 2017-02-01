#include <stdlib.h>
#include "tmdl.h"
#include "generalMethods.h"
#include "currentChapter.h"
#include <string.h>
#include "blacklist.h"

Site source;
char *seriesFolder = NULL;
ChapterQueue *head = NULL;
ChapterQueue *tail = NULL;
unsigned long fullLength = 0;
unsigned long overallPointer = 0;

char *get_series_folder() {
    return seriesFolder;
}

void set_series_folder(char *folder) {
    if (seriesFolder != NULL) {
        return;
    }
    if (folder == NULL) {
        char *path = concat(get_save_directory(), "/");
        seriesFolder = path;
    } else {
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
    if (!get_using_settings()) {
        threaded_load_blacklist();
    }
}

void set_source(Site domainUsed) {
    source = domainUsed;
}

unsigned long get_download_length() {
    return fullLength;
}

unsigned long get_current_download_chapter() {
    return overallPointer;
}

void free_chapter(Chapter *toFree) {
    free(toFree->name);
    free(toFree->link);
    free(toFree);
}

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

void download_entire_queue() {
    if (fullLength == 0) {
        return;
    }
    setup_temporary_folder();
    Chapter *toDownload;
    while (toDownload = pop_from_download(), toDownload != NULL) {
        download_chapter(toDownload, source);
        free_chapter(toDownload);
    }
    fullLength = overallPointer = 0;
    head = tail = NULL;
}
