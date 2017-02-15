#include <stdio.h>

void mangasee_info_search_and_write(char *infoToParse, char *substringStart,
        char *substringEnd, char *single, char *plural, FILE *saveTo);
void setup_mangasee_download();
char **setup_mangasee_chapter(Chapter *current);
