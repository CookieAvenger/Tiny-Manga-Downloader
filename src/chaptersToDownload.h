#ifndef CHAPTER
#define CHAPTER

typedef enum site { kissmanga, other } Site;
                                            
typedef struct chapter {                    
    char *name;                             
    char *link;                             
} Chapter;                                  

struct chapterqueue {
    Chapter *current;
    struct chapterqueue *next;
};

typedef struct chapterqueue ChapterQueue;

void set_series_folder(char *folder);
void set_source(Site domainUsed);
int get_download_length();
int get_current_download_chapter();
void add_to_download_list(Chapter *toAdd);
void download_entire_queue();
char *get_series_folder();

#endif
