#include "chaptersToDownload.h"

void download_chapter(Chapter *current, Site source);
void delete_temporary_folder();
char *sort_out_file_extension(char *filePath, char *fileName, char *url);
char *get_incomplete_chapter_folder();
