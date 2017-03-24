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
#include <regex.h>

regex_t keyFinder = { .re_nsub = 0 };

void free_kissmanga_regex() {
    if (keyFinder.re_nsub != 0) {
        regfree(&keyFinder);
        keyFinder.re_nsub = 0;
    }
}

void compile_kissmanga_regex() {
    if (regcomp(&keyFinder, "(.*CryptoJS.*)", REG_EXTENDED|REG_NEWLINE) != 0) {
        fputs("Could not compile regex", stderr);
        exit(24);
    }
}

//Get every image to download in a kissmanga chapter
char **setup_kissmanga_chapter(Chapter *current) {
    //do only if customData is NULL
    if (current->customData != NULL) {
        return current->customData;
    }
    if (keyFinder.re_nsub == 0) {
        compile_kissmanga_regex();
    }
    char *page = get_kissmanga_page(current->link);
    if (page == NULL) {
        fprintf(stderr, "Failed to access: %s\n", current->name);
        return NULL;
    }
    char **allKeys = find_all_occurances(page, &keyFinder);
    //we know there are only ever 2 :p
    //regexSucces could != 0 due to memory problems or no match among other
    if (allKeys == NULL || allKeys[0] == NULL) {
        free(page);
        fprintf(stderr, "Couldn't parse chapter: %s\n", current->name);
        return NULL;
    }
    //need to find and parse keys here
    
    char *unparsedImageList = get_substring(page, "lstImages", "currImage", -1);
    free(page);
    if (unparsedImageList == NULL) {
        fprintf(stderr, "Couldn't parse chapter: %s\n", current->name);
        return NULL;
    }
    char **toReturn = continuous_substring(unparsedImageList, 
            "lstImages.push(", ");");
    free(unparsedImageList);
    //below method mangaes allkeys memory, and returns answer in toReturn
    return run_page_decryption(allKeys, toReturn);
}

//Parse chapters section and push onto chaptersQueue
void fill_up_queue(char **unparsedChapters) {
    char *seriesPath = concat(get_series_path(), "/");
    size_t chaptersNumber = get_pointer_array_length((void **)unparsedChapters);
    for (size_t i = 0; i < chaptersNumber; i++) {
        Chapter *toAdd = (Chapter *) malloc(sizeof(Chapter));
        if (toAdd == NULL) {
            exit(21);
        }
        toAdd->customData = NULL;
        toAdd->doneWith = false;
        char *linkToAdd = get_substring(unparsedChapters[i], "\"", "\"", 26);
        char *concatedLinkToAdd = concat(seriesPath, linkToAdd);
        free(linkToAdd);
        char *nameToAdd = get_substring(unparsedChapters[i], "\n", "\n", 26);
        decode_html_entities_utf8(nameToAdd, NULL);
        char *nameRevised = str_replace(nameToAdd, "\\", "|");
        free(nameToAdd);
        toAdd->name = nameRevised;
        toAdd->link = concatedLinkToAdd;
        add_to_download_list(toAdd);
        free(unparsedChapters[i]);
    } 
    free(unparsedChapters);
    free(seriesPath);
}

//Setup to download the entire seties
void download_kissmanga_series(char *randomChapterLink) {
    char *seriesPath = concat(get_series_path(), "/");
    char *concatedRandomLink = concat(seriesPath, randomChapterLink);
    free(seriesPath);
    char *initialPage = get_kissmanga_page(concatedRandomLink);
    free(concatedRandomLink);
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

//Download a thumbnail
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

//Figure out if the information is 1 or more
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
//Write kissmanga manga information
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

//Setup a kissmanga link for download
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
        free(testString);
        parse_and_set_kissmanga_series_folder(testType);
        char *chapterName = get_substring(testType, "selected>\n", "\n<", 26);
        char *seriesLink = get_substring(testType, 
                "<p>\n<a href=\"", "\">", 26);
        free(testType);
        decode_html_entities_utf8(chapterName, NULL);
        Chapter *toAdd = (Chapter *) malloc(sizeof(Chapter));
        if (toAdd == NULL) {
            exit(21);
        }
        char *linkToAdd = make_permenent_string(get_series_path());
        toAdd->name = chapterName;
        toAdd->link = linkToAdd;
        add_to_download_list(toAdd);
        process_first_url(seriesLink);
        free(seriesLink);
    }
}
