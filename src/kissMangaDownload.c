#include "kissMangaRead.h"
#include "generalMethods.h"
#include "chaptersToDownload.h"
#include "tmdl.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char **setup_kissmanga_chapter(Chapter *current) {
    char *page = get_kissmanga_chapter(current->link);
    if (page == NULL) {
        fprintf(stderr, "Failed to access: %s\n", current->name);
        return NULL;
    }
    char *unparsedImageList = get_substring(page, "lstImage", "currImage", -1);
    free(page);
    if (unparsedImageList == NULL) {
        fprintf(stderr, "Couldn't parse chapter: %s\n", current->name);
        return NULL;
    }
    return continuous_substring(unparsedImageList, "\"", "\"");
}

//to get link just substring " and ", for chapter name > and \n
void fill_up_queue(char **unparsedChapters) {
    int chaptersNumber = get_string_array_length(unparsedChapters);
    for (int i = 0; i < chaptersNumber; i++) {
        Chapter *toAdd = (Chapter *) malloc(sizeof(Chapter));
        char *linkToAdd = get_substring(unparsedChapters[i], "\"", "\"", 26);
        char *nameToAdd = get_substring(unparsedChapters[i], "\n", "\n", 26);
        toAdd->name = nameToAdd;
        toAdd->link = linkToAdd;
        add_to_download_list(toAdd);
        free(unparsedChapters[i]);
    } 
    free(unparsedChapters);
}

void download_kissmanga_series(char *randomChapterLink) {
    char *initialPage = get_kissmanga_chapter(randomChapterLink);
    if (initialPage == NULL) {
        exit(26);
    }
    parse_and_set_series_folder(initialPage); 
    char *skipFirst = strstr(initialPage, "</select>") + 9;
    char * chaptersSection = get_substring(skipFirst, "</select>", 
            "</select>", 26);
    free(initialPage);
    char **chaptersUnparsed = continuous_substring(chaptersSection, 
            "<option value=", "</option>");
    free(chaptersSection);
    fill_up_queue(chaptersUnparsed);
}

void setup_kissmanga_download() {
    bypass_DDOS_protection();
    char *testType = get_kissmanga_page(get_series_path());
    if (testType == NULL) {
        fprintf(stderr, "This url: %s, is an invalid series location, skipping\n"
                , get_current_url());
        return;
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
        char *chapterLink = get_substring(testType, testString, "\"", 26);
        free(testString);
        free(testType);
        download_kissmanga_series(chapterLink);
    } else {
        //Chapter Page
        //To do
        printf("%s\n", testType);
        free(testString);
        free(testType);
    }
}
