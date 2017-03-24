#include <stdlib.h>
#include "tmdl.h"
#include "generalMethods.h"
#include "currentChapter.h"
#include <string.h>
#include "blacklist.h"
#include "kissMangaRead.h"
#include "kissMangaDownload.h"

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
                mangaName) == 0) {
            seriesFolder = path;
        } else {
            char *fullFolder = concat(path, mangaName);
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
    //if kissmanga, try to decrypt all the chapters first
    if (source == kissmanga) {
        if (get_verbose()) {
            puts("All kissmanga chapters have to be decrypted (sadly will be "
                    "resource intensive... no way to make it light for this atm) "
                    "before download can begin, please be patient");
            fflush(stdout);
        }
        ChapterQueue *pointer = head;
        size_t tempCurrent = 0;
        while (pointer != NULL) {
            ++tempCurrent;
            if (!chapterExists(pointer->current->name)) {
                if (get_verbose()) {
                    printf("Decrypting links for chapter %zu/%zu\n", tempCurrent, fullLength);
                    fflush(stdout);
                }
                pointer->current->customData =
                        setup_kissmanga_chapter(pointer->current);
            } else {
                pointer->current->doneWith = true;
            }
            pointer = pointer->next;
        }
        puts("Finished decryption relavent links");
        fflush(stdout);
        stop_decryption_program();
    }
    if (fullLength == 0) {
        threaded_save_blacklist(true, false);
        return;
    }
    Chapter *toDownload;
    while (toDownload = pop_from_download(), toDownload != NULL) {
        download_chapter(toDownload, source);
        toDownload->doneWith = true;
        free(toDownload->name);
        free(toDownload->link);
        free(toDownload);
    }
    //won't ever happen, if I just remove the if source == kissmanga line, then it would do the process
    //chapter by chapter instead of all chapters in a go, could provide option, but in my opinion, do first
    //is a far better strategy, anyway, it's gonna change at 0.2.0 with chapter selection
    stop_decryption_program();
    fullLength = overallPointer = 0;
    head = tail = NULL;
}
