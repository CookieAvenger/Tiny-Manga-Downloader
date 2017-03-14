#include <stdbool.h>
#include "networking.h"
#include "generalMethods.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include "tmdl.h"
#include <unistd.h>
#include <pwd.h>
#include "customParser.h"
#include "experimental.h"
#include <sys/types.h>
#include <dirent.h>

//current version - 0.1.1
long major = 0;
long normal = 1;
long minor = 1;
char *currentVersion = "v0.1.1";

//Weather to continue even if there is an update
bool continueAnyway = false;
//To stop immedietly if there is an update
bool stopAtUpdate = false;
//Simple storage of if an error ever happened
bool errorOccured = false;
//Directory to download and update
char *updateDirectory = NULL;

char *get_update_directory() {
    return updateDirectory;
}

//Set up update diretory - only ever gets called once
void setUpdateDirectory() {
    struct passwd *passwordEntry = getpwuid(getuid());
    if (passwordEntry == NULL) {
        exit(21);
    }
    char *homeDirectory = passwordEntry->pw_dir;
    updateDirectory = concat(homeDirectory, "/.tmdl/");
    //remove old files in case there are any then make new ones
    delete_folder(updateDirectory, -1);
    create_folder(updateDirectory);
}

//Set prompt to auto yes
void set_yes() {
    continueAnyway = true;
    stopAtUpdate = false;
}

//Set prompt to auto no
void set_no() {
    stopAtUpdate = true;
    continueAnyway = false;
}

//Find all flags and try to check em all for y's and n's
void perform_flag_yes_no_checks(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == '\0') {
                exit(1);
            } else {
                process_flag(argv[i] + 1);
            }
        } else if (strcmp(argv[i], "update") != 0) {
            exit(1);
        }
    }
}

//Print parsing error with regards to updating
void print_parse_error() {
    fputs("Github parsing error, please report.", stderr);
    fputs("Failed to check for updates, continuing anyway.", stderr);
    errorOccured = true;
}

//Figures out if the string version is newer, older or the same
long process_version(char *version) {
    char *leftOvers = '\0';
    long newestMajor = strtol(version, &leftOvers, 10);
    if (leftOvers[0] != '.' || leftOvers[1] == '\0' || errno == ERANGE) {
        print_parse_error();
        return 0;
    }
    char *secondPass = '\0';
    long newestNormal = strtol(leftOvers + 1, &secondPass, 10);
    if (secondPass[0] != '.' || secondPass[1] == '\0' || errno == ERANGE) {
        print_parse_error();
        return 0;
    }
    char *lastTry = '\0';
    long newestMinor = strtol(secondPass + 1, &lastTry, 10);
    if (*lastTry != '\0' || errno == ERANGE) {
        print_parse_error();
        return 0;
    }
    if (major != newestMajor) {
        return major - newestMajor;
    } else if (normal != newestNormal) {
        return normal - newestNormal;
    } else {
        return minor - newestMinor;
    }
}

//Prompts user for yes or no to continue or stop
void continue_prompt() {
    if (continueAnyway) {
        return;
    } else if (stopAtUpdate) {
        exit(0);
    }
    printf("Would you like to continue anyway? [Y/n]:");
    char response = fgetc(stdin);
    char end = fgetc(stdin);
    if (end != '\n' && end != EOF) {
        while (end != '\n' && end != EOF) {
            end = fgetc(stdin);
        }
        puts("Sorry that is not a recognised response, please try again");
        continue_prompt();
    }
    if (response == 'y' || response == 'Y') {
        return;
    } else if (response == 'n' || response == 'Y') {
        exit(0);
    }
    puts("Sorry that is not a recognised response, please try again");
    continue_prompt();    
}

//Untar's the downloaded file and returns the path to the uncompressed folder
char *untar_update(char *fileLocation) {
    char *folderToSend = make_bash_ready(updateDirectory);
    char *fileToSend = make_bash_ready(fileLocation);
    char *untarScript = (char *) malloc(sizeof(char) * (strlen(folderToSend)
            + strlen(fileToSend) + 30));
    if (untarScript == NULL) {
        exit(21);
    }
    sprintf(untarScript, "(cd %s && tar --overwrite -xf %s)",
            folderToSend, fileToSend);
    char *scriptName = concat(updateDirectory, ".extract.sh");
    (void) write_script(scriptName, untarScript, true); 
    free(untarScript);
    //execute script frees scriptName
    //want to do tar -xvf, but -v in piping causes broken pipe :/
    char *newFolderName = execute_script(scriptName, 34, true, '\n', get_verbose());
    //atm just making magic happen, tried using find to duplicate -v effect 
    //but that also threw broken pipe
    //to delete
    if (newFolderName != NULL) {
        free(newFolderName);
    }
    newFolderName = make_permenent_string("Tiny-Manga-Downloader/");
    //end of to delete
    char *emptyFolderCheck = concat(updateDirectory, newFolderName);
    //this also returns if path doesn't exist
    if (newFolderName == NULL || is_directory_empty(emptyFolderCheck)) {
        exit(34);
    }
    free(emptyFolderCheck), free(folderToSend), free(fileToSend);
    char *finalFolder = concat(updateDirectory, newFolderName);
    free(newFolderName);
    return finalFolder;
}

void install_update(char *folderToRunIn) {
    char *folderToSend = make_bash_ready(folderToRunIn);
    size_t folderLength = strlen(folderToSend);
    puts("Configuring updating...");
    char *configureScript = (char *) malloc(sizeof(char) * (folderLength + 26));
    if (configureScript == NULL) {
        exit(21);
    } 
    sprintf(configureScript, "(cd %s && bash ./configure)", folderToSend);
    char *scriptName = concat(updateDirectory, ".configure.sh");
    (void) write_script(scriptName, configureScript, true); 
    free(configureScript);
    (void) execute_script(scriptName, 35, false, EOF, get_verbose());
    puts("Configuration complete");
    puts("Compiling program...");
    char *makeScript = (char *) malloc(sizeof(char) * (folderLength + 14));
    if (makeScript == NULL) {
        exit(21);
    }
    sprintf(makeScript, "(cd %s && make)", folderToSend);
    scriptName = concat(updateDirectory, ".make.sh");
    (void) write_script(scriptName, makeScript, true);
    free(makeScript);
    (void) execute_script(scriptName, 36, false, EOF, get_verbose());
    puts("Compilation complete");
    puts("Installing update...");
    char *installScript = (char *) malloc(sizeof(char) * (folderLength + 
            strlen(updateDirectory) + 70));
    if (installScript == NULL) {
        exit(21);
    }
    sprintf(installScript, "(cd %s && sudo make install && rm -fr %s && echo"
            " 'Installation complete')", folderToSend, updateDirectory);
    free(folderToSend);
    scriptName = concat(updateDirectory, ".install.sh");
    (void) write_script(scriptName, installScript, true);
    execlp("sh", "sh", scriptName, NULL); 
    exit(37); 
}

void full_update_installation_process(char *downloadLink, char *downloadName) {
    setUpdateDirectory();
    //download into file
    char *downloadFile = concat(updateDirectory, downloadName);
    puts("Downloading File...");
    int success = download_file(downloadLink, downloadFile);
    if (success != 0) {
        exit(33);
    }
    puts("Download complete\nUncompressing File...");
    char *uncompressedFolder = untar_update(downloadFile);
    puts("Decompression complete");
    install_update(uncompressedFolder);
    //stuff that never actually happens
    //free(downloadFile), free(uncompressedFolder);
    //delete_folder(updateDirectory, -1);
}

//Check for update, do it if asked and allowed to
void perform_update_operations(bool update) {
    //geteuid == 0 means we have sudo rights
    if (update && geteuid() != 0) {
        exit(42);
    }
    if (get_verbose()) {
        puts("Checking for program update");
    }
    char *page = curl_get_page("https://github.com/CookieAvenger/Tiny-Manga-Downloader/releases");
    if (page == NULL) {
        fputs("Failed to check for updates, continuing anyway.", stderr);
        return;
    }
    char *releaseLocation = strstr(page, "Latest release");
    if (releaseLocation == NULL) {
        print_parse_error();
        return;
    }
    char *version = get_substring(releaseLocation, "truncate-target\">"
            , "</span", -1);
    if (version == NULL || version[0] != 'v' || version[1] == '\0') {
        print_parse_error();
        return;
    }
    //One day maybe read and print the message that went along with the release
    //Impliment to print all changlog scince this version :p
    char *lastOfUs = rstrstr(releaseLocation, currentVersion);
    if (lastOfUs != NULL) {
        char *nextLocation = lastOfUs;
        char *messageToProcess, *finalMessage;
        size_t initialMessage = 0;
        while (nextLocation = rstrstr(nextLocation, "markdown-body"),
                nextLocation != NULL) {
            messageToProcess = get_substring(nextLocation, "\">", "</div>", -1);
            if (messageToProcess == NULL) {
                break;
            }
            finalMessage = continuous_find_and_replace(messageToProcess, "<", ">", "");
            free(messageToProcess);
            if (++initialMessage == 1) {
                puts("Changelog:");
                //may be printed even if finalmessage is empty, I know, that's okay :)
            }
            char *nameSection = rstrstr(nextLocation, "release-title\">");
            if (nameSection != NULL) {
                char *nameOfChangelogVersion = get_substring(nameSection, "\">", "</a>", -1);
                if (nameOfChangelogVersion != NULL) {
                    printf("%s\n", nameOfChangelogVersion);
                    free(nameOfChangelogVersion);
                }
                //may be printed even if finalmessage is empty, I know, that's okay :)
            }
            if (finalMessage != NULL) {
                printf("%s\n", finalMessage);
                free(finalMessage);
            }
        }
    }
    char *downloadSection = strstr(releaseLocation, "Downloads");
    if (downloadSection == NULL) {
        print_parse_error();
        return;
    }
    char *downloadLink = get_substring(downloadSection, "<a href=\"", "\"", -1);
    if (downloadLink == NULL) {
        print_parse_error();
        return;
    }
    char *downloadName = get_substring(downloadSection, "<strong>",
            "</strong>", -1);
    if (downloadName == NULL) {
        print_parse_error();
        return;
    }
    free(page);
    decode_html_entities_utf8(downloadName, NULL);
    char *fullDownloadLink = concat("https://github.com", downloadLink);;
    free(downloadLink);
    int difference = process_version(version + 1);
    if (errorOccured) {
        return;
    }
    if (update) {
        if (difference == 0) {
            puts("Nothing to update, already on the latest stable release");
        } else if (difference > 0) {
            puts("You are currently on a newer version that the latest stable,"
                    " continuing will downgrade to the latest stable.");
            continue_prompt();
            //do update thing
            full_update_installation_process(fullDownloadLink, downloadName);
        } else {
            //do update thing
            full_update_installation_process(fullDownloadLink, downloadName);
        }
    } else {
        if (difference < 0) {
            printf("New stable version v%ld.%ld.%ld->%s of Tiny-Manga-Downloader is now "
                    "available, it is highly recommended to update\n"
                    "You can do that with \"sudo manga-dl update\"\n",
                    major, normal, minor, version);
            continue_prompt();
        }
    }
    free(version), free(fullDownloadLink), free(downloadName);
}
