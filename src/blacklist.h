#include <stdbool.h>

//read blacklist from file or initialised a new one
//needs to have appropriate read write errors
//run this as a thread after the folder location is set and have it join before
//download queue starts
//if verbose say it's loaded
void load_blacklist();

//add to blacklist if not already there, otherwise delete
//returns true if file deleted, false if not - this changes what the next one
//needs to be named (cuz they are named iteratively)
bool blacklist_handle_file (char *filePath);

//have this happen on exit, also when all chapters finish downloading
//needs to have appropriate read write errors
void save_blacklist();
