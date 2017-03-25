#include <stdbool.h>
#include "chaptersToDownload.h"

bool process_flag (char *flag);
bool get_using_settings();
char *get_current_url();
bool get_verbose();
bool get_delete();
char *get_save_directory();
char *get_domain();
char *get_series_path();
bool get_zip_approval();
bool get_to_find_dupes();
int get_similarity_percentage();
void set_series_path(char *newPath);
Site process_first_url(char *url);
bool get_decrypt_first();
