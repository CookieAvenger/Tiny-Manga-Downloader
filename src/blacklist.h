#ifndef BLACKLIST
#define BLACKLIST

#include <stdbool.h>

//Contains all information to uniquely identify a file
typedef struct blacklistentry {
    char *hashValue;
    char *chapterName;
    char *fileName;
} blacklistEntry;

void threaded_load_blacklist();

void threaded_save_blacklist(bool toFree, bool toSave);

void join_threaded_blacklist();

void blacklist_handle_file (char *filePath, char *chapter, char *file);

#endif
