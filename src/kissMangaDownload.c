#include "kissMangaRead.h"
#include "generalMethods.h"
#include "chaptersToDownload.h"
#include "tmdl.h"
#include <stdlib.h>
#include <string.h>
#include "customParser.h"
#include "currentChapter.h"
#include "networking.h"
#include "mangaSeeSupport.h"

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
    char **toReturn = continuous_substring(unparsedImageList, "\"", "\"");
    free(unparsedImageList);
    return toReturn;
}

//to get link just substring " and ", for chapter name > and \n
void fill_up_queue(char **unparsedChapters) {
    size_t chaptersNumber = get_pointer_array_length((void **)unparsedChapters);
    for (size_t i = 0; i < chaptersNumber; i++) {
        Chapter *toAdd = (Chapter *) malloc(sizeof(Chapter));
        if (toAdd == NULL) {
            exit(21);
        }
        char *linkToAdd = get_substring(unparsedChapters[i], "\"", "\"", 26);
        char *nameToAdd = get_substring(unparsedChapters[i], "\n", "\n", 26);
        decode_html_entities_utf8(nameToAdd, NULL);
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
    parse_and_set_kissmanga_series_folder(initialPage); 
    char *skipFirst = strstr(initialPage, "</select>");
    if (skipFirst == NULL) {
        free(initialPage);
        exit(26);
    }
    skipFirst = skipFirst + 9;
    char * chaptersSection = get_substring(skipFirst, "</select>", 
            "</select>", 26);
    free(initialPage);
    char **chaptersUnparsed = continuous_substring(chaptersSection, 
            "<option value=", "</option>");
    free(chaptersSection);
    fill_up_queue(chaptersUnparsed);
}

void download_kissmanga_thumbnail(char *seriesPage) {
    char *fileName = "thumbnail"; // = get_manga_name();
    char *thumbnailLink = get_substring(seriesPage, 
            "<link rel=\"image_src\" href=\"", "\"/>", -1);
    if (thumbnailLink == NULL) {
        return;
        //don't really care if it fails tbh
    }
    char *isKissMangaFile = strstr(thumbnailLink, get_domain());
    char *thumbnailPath = concat(get_series_folder(), fileName);
    if (isKissMangaFile != NULL) {
        char *fileLink = isKissMangaFile + strlen(get_domain());
        if (fileLink[0] == '\0') {
            free(thumbnailPath), free(thumbnailLink);
            return;
        }
        FILE *imageFile = fopen(thumbnailPath, "w");
        if (imageFile == NULL) {
            free(thumbnailPath), free(thumbnailLink);
            return;
        }
        //this is a bit of hack and only is okay cuz we don't care it it fails
        int fd = send_HTTP_request(get_domain(), fileLink,
                get_kissmanga_cookie(), get_kissmanga_useragent());
        save_url_as_file(fd, imageFile); 
        fflush(imageFile);
        fclose(imageFile);
    } else {
        int curlSuccess = download_file(thumbnailLink, thumbnailPath);
        if (curlSuccess != 0) {
            free(thumbnailPath), free(thumbnailLink);
            //don't really care if it fails :3
            return;
        }
    }
    char *useless = sort_out_file_extension(thumbnailPath, fileName, thumbnailLink);
    free(useless), free(thumbnailPath), free(thumbnailLink);
}

void workout_plurality_of_info(char *informationToParse, char *topic,
        FILE *infoFile) {
    char *informationToFind = informationToParse;
    char *foundInfo = get_substring(informationToFind, "\">", "</a>", -1);
    char *tempSkip;
    if (foundInfo == NULL) {
        return;
    }
    tempSkip = strstr(informationToFind, "\">");
    if (tempSkip == NULL) {
        return;
    }
    tempSkip = strstr(&tempSkip[2], "\">");
    if (tempSkip == NULL) {
        fprintf(infoFile, "%s: ", topic);
    } else {
        char *testFind = get_substring(&tempSkip[2], "\">", "</a>", -1);
        if (testFind == NULL) {
            fprintf(infoFile, "%s: ", topic);
        } else {
            fprintf(infoFile, "%ss: ", topic);
            free(testFind);
        }
    }
    free(foundInfo);
}

//This method is weird but gotta live with it!
//Also not the most efficient, but honestly don't use it much so meh
void kissmanga_info_search_and_write(char *informationToParse, char *topic,
        FILE *infoFile) {
    workout_plurality_of_info(informationToParse, topic, infoFile);
    bool firstLoop = true;
    while (1 == 1) {
        char *foundInfo = get_substring(informationToParse, "\">", "</a>", -1);
        if (foundInfo == NULL) {
            if (!firstLoop) {
                fputs("\n", infoFile);
            }
            return;
        }
        if (firstLoop) {
            firstLoop = false;
        } else {
            fputs(", ", infoFile);
        }
        fprintf(infoFile, "%s", foundInfo);
        free(foundInfo);
        char *movingAlong = strstr(informationToParse, "\">") + 2;
        char *toSkip = strstr(movingAlong, "\">");
        if (toSkip != NULL) {
            informationToParse = toSkip + 2;
        } else {
            fputs("\n", infoFile);
            return;
        }
    }
}

//info to be in this order
//Name
//Other Names
//Author (maintain support for multiple authors)
//Genres
//Type (mangasee)
//Release Date (mangasee)
//Status (ongoing or not)
//Description
void download_kissmanga_information(char *seriesPage) {
    char *fileName = "information.txt"; //= concat(get_manga_name(), ".info");
    char *filePath = concat(get_series_folder(), fileName);
    FILE *infoFile = fopen(filePath, "w");
    if (infoFile == NULL) {
        free(filePath);
        return; //don't actually care
    }
    fprintf(infoFile, "Name: %s\n", get_manga_name());

    char *otherNamePart = get_substring(seriesPage, "Other name:", "</p>", -1);
    if (otherNamePart != NULL) {
        mangasee_info_search_and_write(otherNamePart, "\">", "</a>",
                "Alternate Name: ", "Alternate Names: ", infoFile);
        free(otherNamePart);
    }
    
    char *authorPart = get_substring(seriesPage, "Author:", "</p>", -1);
    if (authorPart != NULL) {
        kissmanga_info_search_and_write(authorPart, "Author", infoFile);
        free(authorPart);
    }

    char *genresPart = get_substring(seriesPage, "Genres:", "</p>", -1);
    if (genresPart != NULL) {
        kissmanga_info_search_and_write(genresPart, "Genre", infoFile);
        free(genresPart);
    }
    
    char *statusPart = get_substring(seriesPage, "Status:", "Veiws:", -1);
    if (statusPart != NULL) {
        char *statusToSave = get_substring(statusPart, "&nbsp;", "\n", -1);
        free(statusPart);
        if (statusToSave != NULL) {
            decode_html_entities_utf8(statusToSave, NULL);
            fprintf(infoFile, "Status: %s\n", statusToSave);
            free(statusToSave);
        }
    }

    char *summaryPart = get_substring(seriesPage, "Summary:", "\n</p>", -1);
    if (summaryPart != NULL) {
        char *skipFirst = strchr(summaryPart, '>');
        if (skipFirst == NULL) {
            free(summaryPart);
        } else {
            skipFirst += 1;
            char **summaryLines = continuous_substring(skipFirst,
                    ">", "</p>");
            size_t numberOfLines = run_html_decode_on_strings(summaryLines);
            if (numberOfLines > 0) {
                write_string_array_to_file("Description:\n", summaryLines,
                        "\n", "\n", infoFile);
            }
            pointer_array_free((void **) summaryLines);
            free(summaryPart);
        }
    }

    fflush(infoFile);
    fclose(infoFile);
    free(filePath);
}

void setup_kissmanga_download() {
    bypass_DDOS_protection();
    char *testType = get_kissmanga_page(get_series_path());
    if (testType == NULL) {
        fprintf(stderr, "This url: %s, is an invalid series location, skipping\n"
                , get_current_url());
        return;
    }
    char *decodedPath = url_decode(get_series_path());
    char *testString = (char *) malloc(sizeof(char) * 
            (strlen(decodedPath) + 8));
    if (testString == NULL) {
        exit(21);
    }
    sprintf(testString, "href=\"%s/", decodedPath);
    free(decodedPath);
    char *result = strstr(testType, testString);
    //Need a better way of testing
    if (result != NULL) {
        //Series Page
        char *chapterLink = get_substring(testType, testString, "\"", 26);
        free(testString);
        download_kissmanga_series(chapterLink);
        free(chapterLink);
        download_kissmanga_thumbnail(testType);
        download_kissmanga_information(testType);
        free(testType);
    } else {
        //Chapter Page
        //To do
        printf("%s\n", testType);
        fflush(stdout);
        free(testString);
        free(testType);
        exit(100);
    }
}
