#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "tmdl.h"
#include "kissMangaDownload.h"
#include "kissMangaRead.h"
#include "generalMethods.h"
#include "chaptersToDownload.h"
#include "currentChapter.h"
#include "blacklist.h"
#include "experimental.h"
#include "mangaSeeSupport.h"
#include <curl/curl.h>
#include "updater.h"

//make this settable, percentage match required for dupe check
int dupMatch = 95;
//Weather or not to try to find fupes
bool findDupes = false;
//Weather or not to delete identical
bool delete = true;
//Say whats going on or be silent
bool verbose = true;
//Zip chapters or leav in folders
bool zip = true;
//Where initial save directory is set as
char *saveDirectory = NULL;
//Domain
char *domain = NULL;
//file entered
char *seriesPath = NULL;
//What this instance is dealing with
char *currentUrl = NULL;
//Number of urls or folders left to deal with
int remainingUrls;
//weather we are using the settings file or not 
bool usingSettings = false;

//get how similar dupes need to be
int get_similarity_percentage() {
    return dupMatch;
}

//If it gets to try to find dupes or not
bool get_to_find_dupes() {
    return findDupes;
}

//Weather settings file has been read or not
bool get_using_settings() {
    return usingSettings;
}

//To delete exact dupes or not
bool get_delete() {
    return delete;
}

//Current url or folder we are handling
char *get_current_url() {
    return currentUrl;
}

//Permission to zip
bool get_zip_approval() {
    return zip;
}

//File location on domain
char *get_series_path() {
    return seriesPath;
}

void set_series_path(char *newPath) {
    if (seriesPath != NULL) {
        free(seriesPath);
    }
    seriesPath = newPath;
}

//To speak or not to speak
bool get_verbose() {
    return verbose;
}

//get folder we are working inside
char *get_save_directory() {
    return saveDirectory;
}

//Get manga domain
char *get_domain() {
    return domain;
}

//sig int handler
void terminate_handler(int signalSent) {
    signal(SIGINT, SIG_IGN);
    fputs("Stopping prematurely\n", stderr);
    fputs("Please wait patiently as we wind down\n", stderr);
    if (get_dupe_started() && zip) {
        int status;
        while(wait(&status) != -1) {}
        rezip_all_folders();    
    }
    exit(11);
}

//Save settings in a settings filder in the series folder
void save_settings() {
    if (get_series_folder() == NULL) {
        return;
    }
    create_folder(get_series_folder());
    char *settingsPath = concat(get_series_folder(), ".settings.tmdl");
    FILE *settingsFile = fopen(settingsPath, "w");
    free(settingsPath);
    if (settingsFile == NULL) {
        if (verbose == true) {
            fputs("Cannot save settings... how have you "
                    "managed to do this :/\n", stderr);
        }
        return;
    }
    fprintf(settingsFile, "%s\n%s\n", domain, seriesPath); 
    if (verbose) {
        fputs("v", settingsFile);
    } else {
        fputs("s", settingsFile);
    }
    if (zip) {
        fputs("z", settingsFile);
    } else {
        fputs("f", settingsFile);
    }
    if (delete) {
        fputs("d", settingsFile);
    } else {
        fputs("k", settingsFile);
    }
    if (findDupes) {
        fputs("e", settingsFile);
    }
    fputs("\n", settingsFile);
    fflush(settingsFile);
    fclose(settingsFile);
}

//Prints appropriate error to stderr before exit and removes temporary files
void print_error(int err, void *notUsing) {
    //this is what happend when a fork of this fails
    if (err == 24 || err == 0) {
        return;
    }
    //in case we are zipping, want to wait for zip to finish before delete temp
    //folder
    curl_global_cleanup();
    int status;
    while(wait(&status) != -1) {}
    char *updateDirectory = get_update_directory();
    if (updateDirectory != NULL) {
        delete_folder(updateDirectory, -1);
    }
    char *pythonScript = get_python_script_location();
    if (pythonScript != NULL) {
        remove(pythonScript);
    }
    clean_up_page_decryption();
    char *bashScript = get_bash_script_location();
    if (bashScript != NULL) {
        remove(get_bash_script_location());
    }
    join_threaded_blacklist();
    delete_folder(get_incomplete_chapter_folder(), -1);
    if (currentUrl != NULL && err != 0) {
        fprintf(stderr, "\nError occured at: %s\n", currentUrl);
    }
    if (usingSettings && verbose && (err == 6 || err == 22 || err == 23 
            || err == 26)) {
        fputs("Try updating the domain in the first line of .settings "
                "or the series location (part after domain) if this keeps "
                "occuring the site location may be old now\n", stderr);
    }
    switch(err) {
        case 1:
            fputs("Usage: manga-dl [options]\n", stderr);
            fputs("   or: manga-dl <url> [<urls>] <savelocation> [options]\n",
                     stderr);
            fputs("   or: manga-dl -u <savelocation> [<savelocations>] "
                    "[options]\n", stderr);
            //Fix this part bro...
            fputs("Warning: behaviour if usage not strictly "
                    "followed is unknown\n", stderr);
            fputs("Try \"man manga-dl\" for more info.\n", stderr);
            break;
        case 2:
            fputs("Directory provided does not exist - ensure one is provided\n"
                    , stderr);
            break;
        case 3:
            fputs("Do not have nessacary read and write permissions "
                    "in provided save location or location "
                    "is not a directory\n", stderr);
            break;
        case 4:
            fputs("Do not have nessacary read and write permissions ", stderr);
            fputs("in current directory\n", stderr);
            break;
        case 6:
            fputs("Invalid series location\n", stderr);
            break;
        case 7:
            fputs("No save location given\n", stderr);
            break;
        case 21:
            fputs("System error\n", stderr);
            break;
        case 22:
            fputs("Network error\n", stderr);
            break;
        case 23:
            fputs("Unkown error parsing cookie information\n", stderr);
            break;
        case 24:
            //Not handled at all - exec failed
            break;
        case 25:
            //Handled elsewhere - cookie script failed
            break;
        case 26:
            fputs("Webpage parsing error\n", stderr);
            break;
        case 27:
            fputs("Zipping failed, try with storing in folders instead\n",
                    stderr);
            break;
        case 28:
            fputs("Unzipping failed... okay... maybe try all over again in "
                    "a new directory and keep zipping off, that should work "
                    "for now - and let mw know and I'll try to come up with a"
                    " more permenenat work around :P\n", stderr);
            break;
        case 31:
            fputs("Settings file is invalid or does not exist\n", stderr);
            break;
        case 33:
            fputs("Failed to download update\n", stderr);
            break;
        case 34:
            fputs("Failed to unzip the downloaded update\n", stderr);
            break;
        case 35:
            fputs("Failed to configure the downloaded update\nNew dependences"
                    " may be required, visit https://github.com/CookieAvenger/"
                    "Tiny-Manga-Downloader to find out.\n", stderr);
            break;
        case 36:
            fputs("Failed to compile the downloaded update\nNew dependences"
                    " may be required, visit https://github.com/CookieAvenger/"
                    "Tiny-Manga-Downloader to find out.\n", stderr);
            break;
        case 37:
            fputs("Failed to install the downloaded update, please ensure "
                    "program is being run with sudo\n", stderr);
            break;
        case 42:
            fputs("Please run again with sudo rights to check and install "
                    "updates.\nThis can be done by running \"sudo !!\"\n"
                    , stderr);
            break;
    }
}

//sets initial save directory as current working directory
void set_save_directory_as_current() {
    //reimpliment getcwd so you don't have to use a buffer
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        saveDirectory = make_permenent_string(cwd);
        if (access(saveDirectory, R_OK|W_OK) == -1) {
            exit(4);
        }
    } else {
       exit(21); 
    }
}

//Read and parse the settings file
Site read_settings() {
    char *settingsPath = concat(get_series_folder(), ".settings.tmdl");
    FILE *settingsFile = fopen(settingsPath, "r");
    free(settingsPath);
    if (settingsFile == NULL) {
        exit(31);
    }
    Site domainUsed = other;
    domain = read_from_file(settingsFile, '\n', true);
    if (domain == NULL || domain[0] == '\0') {
        exit(31);
    }
    if (strncmp(domain, "kissmanga", 9) == 0) {
        domainUsed = kissmanga;
    } else if (strncmp(domain, "mangasee", 8) == 0) {
        domainUsed = mangasee;
    } else {
        //need other checks here
        exit(31);
    }
    seriesPath = read_from_file(settingsFile, '\n', true);
    if (domain == NULL || seriesPath[0] == '\0') {
        exit(31);
    }
    char *parse = read_from_file(settingsFile, '\n', true);
    size_t i = 0;
    while(parse[i] != '\0') {
        switch(parse[i++]) {
            case 's':
                verbose = false;
                break;
            case 'v':
                verbose = true;
                break;
            case 'f':
                zip = false;
                break;
            case 'z':
                zip = true;
                break;
            case 'd':
                delete = true;
                break;
            case 'k':
                delete = false;
                break;
            case 'e':
                findDupes = true;
                break;
            default:
                exit(31);
        }
    }
    free(parse);
    fclose(settingsFile);
    return domainUsed;
}

//Set settings based on flags
bool process_flag (char *flag) {
    switch(flag[0]) {
        case '\0':
            return false;
        case 'v':
            verbose = true;
            return process_flag(flag+1);
        case 's':
            verbose = false;
            return process_flag(flag+1);
        case 'z':
            zip = true;
            return process_flag(flag+1);
        case 'f':
            zip = false;
            return process_flag(flag+1);
        case 'u':        
            process_flag(flag+1);
            return true;
        case 'd':
            delete = true;
            return process_flag(flag+1);
        case 'k':
            delete = false;
            return process_flag(flag+1);
        case 'e':
            if (!findDupes) {
                findDupes = true;
            } else {
                set_files_changed();
            }
            return process_flag(flag+1);
        case 'y':
            set_yes();
            return process_flag(flag+1);
        case 'n':
            set_no();
            return process_flag(flag+1);
        default:
            exit(1);
    }
}

//Set domain and series based on url
void domain_and_series_set(char *domainCheck) {
    if (domain != NULL) {
        free(domain);
    }
    if (seriesPath != NULL) {
        free(seriesPath);
    }
    char *tempSeriesPath = strchr(domainCheck, '/');
    if (tempSeriesPath == NULL) {
        exit(6);
    }
    seriesPath = make_permenent_string(tempSeriesPath);
    size_t charectersInDomain = tempSeriesPath - domainCheck;
    domain = (char *) malloc(sizeof(char) * (charectersInDomain + 1));
    if (domain == NULL) {
        exit(21);
    }
    strncpy(domain, domainCheck, charectersInDomain);
    domain[charectersInDomain] = '\0';
    size_t seriesLength = strlen(seriesPath);
    while (seriesPath[--seriesLength] == '/') {
        seriesPath[seriesLength] = '\0';
    }
}

//process and setup the first url
Site process_first_url(char *url) {
    char *domainCheck = strstr(url, "kissmanga");
    if (domainCheck != NULL) {
        domain_and_series_set(domainCheck);
        return kissmanga;
    }
    domainCheck = strstr(url, "mangasee");
    if (domainCheck != NULL) {
        domain_and_series_set(domainCheck);
        return mangasee;
    }
    return other;
}

//Set the initial save directory
void set_save_directory(char *lastArg) {
    if (saveDirectory == NULL) {
        if (lastArg != NULL) {
            if ((access(lastArg, F_OK) != -1) && (!is_file(lastArg))) {
                if (access(lastArg, W_OK|R_OK) != -1) {
                    remainingUrls--;
                    char *tempToFree = realpath(lastArg, NULL);
                    saveDirectory = make_permenent_string(tempToFree);
                    free(tempToFree);
                } else {
                    exit(3);
                }
            } else {
                exit(2);
            }
        } else {
            exit(7);
        }
    }
}

//check if we are in current folder update mode
bool current_folder_update_mode (int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            return false;
        } 
    }
    return true;
}

//Check arguments sent from cmd line
Site argument_check(int argc, char** argv) {
    Site domainUsed = other;
    if (current_folder_update_mode(argc, argv)) {
        usingSettings = true;
        set_save_directory_as_current();
        set_series_folder(NULL);
        currentUrl = get_series_folder();
        domainUsed = read_settings();
    }
    remainingUrls = argc - 1;
    int firstUrl = 0;
    char *lastArg = NULL;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            remainingUrls--;
            if (argv[i][1] == '\0') {
                exit(1);
            }
            bool update = process_flag(argv[i]+1);
            if (update) {
                if (++i >= argc) {
                    exit(1);
                }
                usingSettings = true;
                currentUrl = argv[i];
                set_save_directory(argv[i]);
                set_series_folder(NULL);
                domainUsed = read_settings();
                firstUrl++;
            }
        } else if (firstUrl == 0) {
            remainingUrls--, firstUrl++;
            currentUrl = argv[i];
            domainUsed = process_first_url(argv[i]);
        } else {
            lastArg = argv[i];
        }
    }
    set_save_directory(lastArg);
    if (currentUrl == NULL) {
        exit(1);
    }
    return domainUsed;
}

//Fork exec tmdl to continue doing other urls or folders
int duplicate_and_continue(int argc, char **argv) {
    if(remainingUrls <= 0) {
        return 0;
    }
    pid_t pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        execvp(argv[0], remove_string_from_array(argc, argv, currentUrl));
        exit(24);
    }
    close(0), close(1), close(2);
    //parent
    signal(SIGINT, SIG_IGN);
    int status;
    if ((waitpid(pid, &status, 0) == -1) || (WIFEXITED(status) == 0)) {
        exit(21);
    }
    return WEXITSTATUS(status);
}

//Where it all happens :)
int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, terminate_handler);
    on_exit(print_error, NULL);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    if (strcmp(argv[1], "update") == 0) {
        perform_flag_yes_no_checks(argc, argv);
        perform_update_operations(true);
        return 0;
    }
    Site domainUsed = argument_check(argc, argv);
    set_source(domainUsed);
    perform_update_operations(false);
    if (usingSettings) {
        threaded_load_blacklist();
    }
    if (domainUsed == kissmanga) {
        //if initial page is invalid just skip it
        setup_kissmanga_download();
    } else if (domainUsed == mangasee) {
        setup_mangasee_download();
    } else if (domainUsed == other) {
        fprintf(stderr, "This url: %s is from an unsupported domain, skipping\n"
                , currentUrl);
    } 
    save_settings();
    download_entire_queue();
    clean_up_page_decryption();
    curl_global_cleanup();
    //join that final blacklist save(true)
    join_threaded_blacklist();
    experimental_find_dupes();
    //fork and continue is needed
    //return it's status not 0
    return duplicate_and_continue(argc, argv);
}
