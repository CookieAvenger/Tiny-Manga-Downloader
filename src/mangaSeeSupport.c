#include "networking.h"
#include "generalMethods.h"
#include "tmdl.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "chaptersToDownload.h"
#include "customParser.h"

char *get_mangasee_page(char *file);

char *handle_error_codes(char *page) {
    if (strncmp(page + 9, "3", 1) == 0) {
        char *redirectTo = get_substring(page, "Location: ", "\n", 6);
        free(page);
        page = get_mangasee_page(redirectTo);
        free(redirectTo);
        return page;
    } else if (strncmp(page + 9, "4", 1) == 0) {
        free(page);
        return NULL;
    }
    return page;
}

char *get_mangasee_page(char *file) {
    char *page = NULL;
    int fd = send_HTTP_request(get_domain(), file, NULL, NULL);
    page = read_all_from_fd(fd, false);
    page = handle_error_codes(page);
    return page;
}

char **setup_mangasee_chapter(Chapter *current) {
    char *page = get_mangasee_page(current->link);
    if (page == NULL) {
        fprintf(stderr, "Failed to access: %s\n", current->name);
        return NULL;
    }
    char *imagesPart = get_substring(page, "PageArr={", "};", -1);
    free(page);
    if (imagesPart == NULL) {
        fprintf(stderr, "Couldn't parse chapter: %s\n", current->name);
        return NULL;
    }
    char *skipFirst = strstr(imagesPart, ":\"");
    if (skipFirst == NULL) {
        free(imagesPart);
        exit(26);
    }
    skipFirst = skipFirst + 2;
    char **toReturn = continuous_substring(skipFirst, ":\"", "\"");
    free(imagesPart);
    //do the parsing
    char *toReplace;
    size_t count = 0;
    while(toReplace = toReturn[count], toReplace != NULL) {
        toReturn[count] = str_replace(toReplace, "\\", NULL);
        free(toReplace), count++;
    }
    return toReturn;
}

void parse_and_set_mangasee_series_folder(char *page, bool chapterPage) {
    char *seriesLocation = get_substring(page, "\"SeriesName\"",
             "\n", 26);
    char *seriesName;
    if (chapterPage) {
        seriesName = get_substring(seriesLocation, "value=\"", "\"", 26);
    } else {
        seriesName = get_substring(seriesLocation, ">", "<", 26);
    }
    decode_html_entities_utf8(seriesName, NULL);
    set_series_folder(seriesName);
    free(seriesLocation);
}

void setup_mangasee_chapters_download(char *seriesPage) {
    char *chaptersPart = get_substring(seriesPage, "list chapter", "</div>", 26);
    char **chaptersUnparse = continuous_substring(chaptersPart, "Chapter=",
            "/span>");
    free(chaptersPart);
    size_t numberOfChapters = get_pointer_array_length((void **) chaptersUnparse);
        for (ssize_t i = numberOfChapters - 1; i >= 0; i--) {
        Chapter *toAdd = (Chapter *) malloc(sizeof(Chapter));
        if (toAdd == NULL) {
            exit(21);
        }
        char *linkToAdd = get_substring(chaptersUnparse[i], "href=\"", "\"", 26);
        char *nameToAdd = get_substring(chaptersUnparse[i],
                "chapterLabel\">", "<", 26);
        decode_html_entities_utf8(nameToAdd, NULL);
        toAdd->name = nameToAdd;
        toAdd->link = linkToAdd;
        add_to_download_list(toAdd);
        free(chaptersUnparse[i]);
    }
    free(chaptersUnparse);
}

void setup_mangasee_download() {
    char *seriesPage = get_mangasee_page(get_series_path());
    if (seriesPage == NULL) {
        fprintf(stderr, "This url: %s, is an invalid series location, skipping\n"
                , get_current_url());
        return;
    }
    char *isSeriesPage = get_substring(seriesPage, "startReading\">",
             "</div>", -1);
    if (isSeriesPage != NULL) {
        //SeriesPage
        parse_and_set_mangasee_series_folder(seriesPage, false);
        setup_mangasee_chapters_download(seriesPage);
        //download thumbail and information here!!
        free(seriesPage);
    } else {
        //Chapter Page
        //To do
        printf("%s\n", seriesPage);
        fflush(stdout);
        free(seriesPage);
        exit(100);
    }
}
