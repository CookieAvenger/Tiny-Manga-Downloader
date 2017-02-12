#ifndef BLACKLIST
#define BLACKLIST
#include <stdbool.h>

//supports threads but not actually all that thread safe, just start a thread,
//do other stuff and join thread!! don't start multiple threads while touching
//the blacklist

typedef struct blacklistentry {
    char *hashValue;
    char *chapterName;
    char *fileName;
} blacklistEntry;

void threaded_load_blacklist();

void threaded_save_blacklist(bool toFree);

void join_threaded_blacklist();

//read blacklist from file or initialised a new one
//needs to have appropriate read write errors
//run this as a thread after the folder location is set and have it join before
//download queue starts
//if verbose say it's loaded
//void load_blacklist();

//add to blacklist if not already there, otherwise delete
//returns true if file deleted, false if not - this changes what the next one
//needs to be named (cuz they are named iteratively)
void blacklist_handle_file (char *filePath, char *chapter, char *file);

//have this happen on exit, also when all chapters finish downloading
//needs to have appropriate read write errors
//what if tree returns null
//void save_blacklist(bool toFree);

#endif
