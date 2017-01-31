#ifndef BLACKLIST
#define BLACKLIST

typedef struct blacklistentry {
    char *hashValue;
    char *chapterName;
    char *fileName;
} blacklistEntry;

void threadedLoad();

void joinThreadedLoad();

//read blacklist from file or initialised a new one
//needs to have appropriate read write errors
//run this as a thread after the folder location is set and have it join before
//download queue starts
//if verbose say it's loaded
void load_blacklist();

//add to blacklist if not already there, otherwise delete
//returns true if file deleted, false if not - this changes what the next one
//needs to be named (cuz they are named iteratively)
void blacklist_handle_file (char *filePath);

//have this happen on exit, also when all chapters finish downloading
//needs to have appropriate read write errors
//what if tree returns null
void save_blacklist();

#endif
