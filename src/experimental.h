#include <stdbool.h>

void experimental_find_dupes();
void set_files_changed();
void unzip_all_comic_book_archives();
void rezip_all_folders();
bool get_dupe_started();
char *get_bash_script_location();
void* write_script(char *name, char *script, bool fullNameGiven);
char *execute_script(char *scriptFile, int error, bool toPipe,
        char endRead, bool keepOutput);
