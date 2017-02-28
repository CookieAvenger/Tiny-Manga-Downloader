#include "networking.h"
#include "generalMethods.h"
#include "tmdl.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "chaptersToDownload.h"
#include "customParser.h"
#include "currentChapter.h"
#include "customParser.h"

//Get all images from a mangasee chapter
char **setup_mangasee_chapter(Chapter *current) {
    char *page = get_standard_manga_page(current->link);
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

//set series folder from page depending on if its a chapter or series page
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
    if (seriesName != NULL) {
        free(seriesName);
    }
    free(seriesLocation);
}

//setup download of all chapters
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
        char *nameRevised = str_replace(nameToAdd, "\\", "|");
        free(nameToAdd);
        toAdd->name = nameRevised;
        toAdd->link = linkToAdd;
        add_to_download_list(toAdd);
        free(chaptersUnparse[i]);
    }
    free(chaptersUnparse);
}

//download series thumbnail
void download_mangasee_thumbnail(char *seriesPage) {
    char *skipOne = strstr(seriesPage, "img src=");
    if (skipOne == NULL) {
        return;
    }
    skipOne = skipOne + 8;
    char *imageLocation = get_substring(skipOne, "img src=\"", "\"", -1);
    if (imageLocation == NULL) {
        return;
    }
    char *fileName = "thumbnail";
    char *thumbnailPath = concat(get_series_folder(), fileName);
    int curlSuccess = download_file(imageLocation, thumbnailPath);
    if (curlSuccess != 0) {
        free(thumbnailPath), free(imageLocation);
        return;
    }
    char *useless = sort_out_file_extension(thumbnailPath, fileName, imageLocation);
    free(useless), free(thumbnailPath), free(imageLocation);
}

//search and save info
void mangasee_info_search_and_write(char *infoToParse, char *substringStart,
        char *substringEnd, char *single, char *plural, FILE *saveTo) {
    char **allInfo = continuous_substring(infoToParse,
            substringStart, substringEnd);
    size_t amountOfInfo = run_html_decode_on_strings(allInfo);
    char *initial = plural;
    if (amountOfInfo == 1) {
        initial = single;
    }
    if (amountOfInfo > 0) {
        write_string_array_to_file(initial, allInfo, ", ", "\n", saveTo);
    }
    pointer_array_free((void **) allInfo); 
}

//Get info
void download_mangasee_information(char *seriesPage) {
    char *fileName = "information.txt";
    char *filePath = concat(get_series_folder(), fileName);
    FILE *infoFile = fopen(filePath, "w");
    if (infoFile == NULL) {
        free(filePath);
        return;
    }
    fprintf(infoFile, "Name: %s\n", get_manga_name());

    //Could pin point in one substring, but it has spaces where they don't need
    //to be, so in case it changes, I'm making it a bit more complex
    char *alternativeNamesPart = get_substring(seriesPage,
            "Alternate Name", "</div>", -1);
    if (alternativeNamesPart != NULL) {
        char *alternativeNames = get_substring(alternativeNamesPart,
                "</b>", "\n", -1);
        free(alternativeNamesPart);
        if (alternativeNames != NULL) {
            decode_html_entities_utf8(alternativeNames, NULL);
            char *plural = "s";
            if (strchr(alternativeNames, ',') == NULL) {
                plural = "";
            }
            if (alternativeNames[0] != ' ') {
                char *toFree = alternativeNames;
                alternativeNames = concat(" ", alternativeNames);
                free(toFree);
            }
            fprintf(infoFile, "Alternative Name%s:%s\n",
                    plural, alternativeNames);
            free(alternativeNames);
        }
    }

    char *authorPart = get_substring(seriesPage, "Author", "</div>", -1);
    if (authorPart != NULL) {
        mangasee_info_search_and_write(authorPart, "'>", "<",
                "Author: ", "Authors: ", infoFile);
        free(authorPart);
    }

    char *genrePart = get_substring(seriesPage, "Genre", "</div>", -1);
    if (genrePart != NULL) {
        mangasee_info_search_and_write(genrePart, "'>", "<",
                "Genre: ", "Genres: ", infoFile);
        free(genrePart);
    }

    char *typePart = get_substring(seriesPage, "Type:", "</div>", -1);
    if (typePart != NULL) {
        char *comicType = get_substring(typePart, "\">:", "</a>", -1);
        free(typePart);
        if (comicType != NULL) {
            decode_html_entities_utf8(comicType, NULL);
            fprintf(infoFile, "Type: %s\n", comicType);
            free(comicType);
        }
    }

    char *releasePart = get_substring(seriesPage, "Released", "</div>", -1);
    if (releasePart != NULL) {
        char *yearReleased = get_substring(releasePart, "\">", "</a>", -1);
        free(releasePart);
        if (yearReleased != NULL) {
            decode_html_entities_utf8(yearReleased, NULL);
            fprintf(infoFile, "Released: %s\n", yearReleased);
            free(yearReleased);
        } 
    }

    char *statusPart = get_substring(seriesPage, "Status", "</div>", -1);
    if (statusPart != NULL) {
        mangasee_info_search_and_write(statusPart, "\">", "</a>",
                "Status: ", "Status: ", infoFile);
        free(statusPart);
    }

    //No nonsense straight to the info
    char *description = get_substring(seriesPage,
            "description\">", "</div>", -1);
    if (description != NULL) {
        decode_html_entities_utf8(description, NULL);
        fprintf(infoFile, "Description:\n%s\n", description);
        free(description);
    }

    fflush(infoFile);
    fclose(infoFile);
    free(filePath);
}

//Setup a mangasee page for download
void setup_mangasee_download() {
    char *seriesPage = get_standard_manga_page(get_series_path());
    if (seriesPage == NULL) {
        fprintf(stderr, "This url: %s, is an invalid series location, skipping\n"
                , get_current_url());
        return;
    }
    char *isSeriesPage = get_substring(seriesPage, "startReading\">",
             "</div>", -1);
    if (isSeriesPage != NULL) {
        free(isSeriesPage);
        //SeriesPage
        parse_and_set_mangasee_series_folder(seriesPage, false);
        setup_mangasee_chapters_download(seriesPage);
        download_mangasee_thumbnail(seriesPage);
        download_mangasee_information(seriesPage);
        free(seriesPage);
    } else {
        parse_and_set_mangasee_series_folder(seriesPage, true);
        char *nameAndSpace = concat(get_manga_name(), " ");
        char *chapterNameLocation = get_substring(seriesPage, "<title>",
                "</title>", 26);
        decode_html_entities_utf8(chapterNameLocation, NULL);
        char *chapterName = get_substring(chapterNameLocation,
                nameAndSpace, " Page", 26);
        free(nameAndSpace), free(chapterNameLocation);
        char *realSeriesPath = get_substring(seriesPage,
                "<a href=\"", "\">", 26);
        if (chapterName[0] == '\0'|| chapterName[1] == '\0') {
            exit(26);
        }
        free(seriesPage);
        decode_html_entities_utf8(chapterName, NULL);
        Chapter *toAdd = (Chapter *) malloc(sizeof(Chapter));
        if (toAdd == NULL) {
            exit(21);
        }
        char *linkToAdd = make_permenent_string(get_series_path());
        toAdd->name = chapterName; //to skip a space
        toAdd->link = linkToAdd;
        add_to_download_list(toAdd);
        set_series_path(realSeriesPath);
    }
}
