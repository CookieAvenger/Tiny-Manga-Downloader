#include "kissMangaRead.h"
#include "generalMethods.h"
#include "chaptersToDownload.h"
#include "tmdl.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void download_kissmanga_series(char *randomChapterLink) {
    char *initialPage = get_kissmanga_chapter(randomChapterLink);
    if (initialPage == NULL) {
        exit(26);
    }
    set_series_folder(initialPage); 
    char *skipFirst = strstr(initialPage, "</select>") + 9;
    char * chaptersSection = get_substring(skipFirst, "</select>", 
            "</select>", 26);
    free(initialPage);
    char **chaptersUnparsed = continuous_substring(chaptersSection, 
            "<option value=", "</option>");
    free(chaptersSection);
    //remeber to free chapters Unparsed with string array free
    //to get link just substring " and ", for chapter name > and \n
}

void start_kissmanga_download() {
    bypassDDOSprotection();
    char *testType = get_kissmanga_page(get_series_path());
    if (testType == NULL) {
        exit(6);
    }
    char *testString = (char *) malloc(sizeof(char) * 
            (strlen(get_series_path()) + 7));
    if (testString == NULL) {
        exit(21);
    }
    sprintf(testString, "href=\"%s/", get_series_path());
    char *result = strstr(testType, testString);
    //Need a better way of testing
    if (result != NULL) {
        //Series Page
        //Now we open any chapter
        char *chapterLink = get_substring(testType, testString, "\"", 26);
        free(testString);
        free(testType);
        download_kissmanga_series(chapterLink);
    } else {
        //Chapter Page
        printf("%s\n", testType);
        free(testString);
        free(testType);
    }
}
