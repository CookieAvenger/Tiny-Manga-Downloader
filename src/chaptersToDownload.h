#ifndef CHAPTER
#define CHAPTER

#include <stddef.h>

//Enum of what site is being used
typedef enum site { kissmanga, mangasee, other } Site;

//Struct containing relavent chapter information
typedef struct chapter {
    char *name;
    char *link;
} Chapter;

//Singly linked list made for Chapter Queue
struct chapterqueue {
    Chapter *current;
    struct chapterqueue *next;
};
typedef struct chapterqueue ChapterQueue;

void set_series_folder(char *folder);
void set_source(Site domainUsed);
size_t get_download_length();
size_t get_current_download_chapter();
void add_to_download_list(Chapter *toAdd);
void download_entire_queue();
char *get_series_folder();
char *get_manga_name();
Site get_source();

#endif
