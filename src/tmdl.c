#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "tmdl.h"
#include "kissMangaDownload.h"
#include "generalMethods.h"
#include "chaptersToDownload.h"
#include "currentChapter.h"
#include <sys/wait.h>

bool delete = true;
bool verbose = true;
bool zip = true;
char *saveDirectory = NULL;
char *domain;
char *seriesPath;
char *currentUrl = NULL;
int remainingUrls;
//Weather to save settings and blacklist
bool save = false;
//weather we are using the settings file or not 
bool usingSettings = false;

bool get_using_settings() {
    return usingSettings;
}

bool get_delete() {
    return delete;
}

char *get_current_url() {
    return currentUrl;
}

bool get_zip_approval() {
    return zip;
}

char *get_series_path() {
    return seriesPath;
}

bool get_verbose() {
    return verbose;
}

char *get_save_directory() {
    return saveDirectory;
}

char *get_domain() {
    return domain;
}

void terminate_handler(int signal) {
    fputs("\n", stderr);
    exit(11);
}

void save_settings() {
    char *settingsPath = concat(get_series_folder(), ".settings.tmdl");
    FILE *settingsFile = fopen(settingsPath, "w");
    if (settingsFile == NULL) {
        if (verbose == true) {
            fputs("Cannot save settings... how have you "
                    "managed to do this :/", stderr);
        }
        return;
    }
    fprintf(settingsFile, "%s\n%s\n", domain, seriesPath); 
    if (verbose) {
        fputs("v\n", settingsFile);
    } else {
        fputs("s\n", settingsFile);
    }
    if (zip) {
        fputs("z\n", settingsFile);
    } else {
        fputs("f\n", settingsFile);
    }
    if (delete) {
        fputs("d\n", settingsFile);
    } else {
        fputs("k\n", settingsFile);
    }
    fclose(settingsFile);
}

//Prints appropriate error to stderr before exit
void print_error(int err, void *notUsing) {
    //don't care if wait fails, should fail most of the time in fact
    if (save) {
        int status;
        wait(&status);
        delete_folder(get_temporary_folder(), -1);
        save_settings();
        //save blacklist goes here too
    }
    if (currentUrl != NULL) {
        fprintf(stderr, "Error occured at: %s\n", currentUrl);
    }
    if (usingSettings && verbose && (err == 6 || err == 22 || err == 23 || err == 26)) {
        fputs("Try updating the domain in the first line of .settings "
                "or the series location (part after domain) if this keeps occuring"
                " the site location may be old now\n", stderr);
    }
    switch(err) {
        case 1:
            fputs("Usage: tmdl\n", stderr);
            fputs("   or: tmdl <url> [<urls>] <savelocation>|-c\n", stderr);
            fputs("   or: tmdl -u <savelocation> [<savelocations>]\n", stderr);
            fputs("post command options:\n", stderr);
            fputs("[-v] for verbose (default) or [-s] for silent\n", stderr);
            fputs("[-z] for zipped chapters (default) or [-f] for folders\n", stderr);
            fputs("[-d] to delete duplicates (default) or [-k] to keep\n", stderr);
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
        case 11:
            fputs("Stopping prematurely\n", stderr);
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
            fputs("Failed to copy to new folder\n", stderr);
            break;
        case 31:
            fputs("Settings file is invalid or does not exist\n", stderr);
    }
}

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

Site parse_settings(FILE *settingsFile) {
    Site domainUsed = other;
    domain = read_from_file(settingsFile, '\n', true);
    if (domain[0] == '\0') {
        exit(31);
    }
    if (strncmp(domain, "kissmanga", 9) == 0) {
        domainUsed = kissmanga;
    } else {
        //need other checks here
        exit(31);
    }
    seriesPath = read_from_file(settingsFile, '\n', true);
    if (seriesPath[0] == '\0') {
        puts("hi");
        exit(31);
    }
    char *parse;
    while(parse = read_from_file(settingsFile, '\n', true), parse[0] != '\0') {
        if (parse[0] == 's' && parse[1] == '\0') {
            verbose = false;
        } else if (parse[0] == 'v' && parse[1] == '\0') {
            verbose = true;
        } else if (parse[0] == 'f' && parse[1] == '\0') {
            zip = false;
        } else if (parse[0] == 'z' && parse[1] == '\0') {
            zip = true;
        } else if (parse[0] == 'd' && parse[1] == '\0') {
            delete = true;
        } else if (parse[0] == 'k' && parse[1] == '\0') {
            delete = false;
        } else {
            exit(31);
        }
        free(parse);
    }
    return domainUsed;
}

Site read_settings() {
    char *settingsPath = concat(get_series_folder(), ".settings.tmdl");
    FILE *settingsFile = fopen(settingsPath, "r");
    if (settingsFile == NULL) {
        exit(31);
    }
    Site domainUsed = parse_settings(settingsFile);    
    fclose(settingsFile);
    currentUrl = get_series_folder();
    remainingUrls = 0;
    return domainUsed;
}

bool process_flag (char *flag) {
    if (flag == NULL || flag[0] == '\0') {
        exit(1);
    } else if (flag[0] == 'v' && flag[1] == '\0') {
        verbose = true;
    } else if (flag[0] == 's' && flag[1] == '\0') {
        verbose = false;
    } else if (flag[0] == 'z' && flag[1] == '\0') {
        zip = true;
    } else if (flag[0] == 'f' && flag[1] == '\0') {
        zip = false;
    } else if (flag[0] == 'c' && flag[1] == '\0') {
        set_save_directory_as_current();
    } else if (flag[0] == 'u' && flag[1] == '\0') {
        return true;
    } else if (flag[0] == 'd' && flag[1] == '\0') {
        delete = true;
    } else if (flag[0] == 'k' && flag[1] == '\0') {
        delete = false;
    } else {
        exit(1);
    }
    return false;
}

Site process_first_url(char *url) {
    Site domainUsed = other; 
    char *domainCheck = strstr(url, "kissmanga");
    if (domainCheck != NULL) {
        domainUsed = kissmanga;
        seriesPath = strstr(domainCheck, "/");
        if (seriesPath == NULL) {
            exit(6);
        }
        size_t charectersInDomain = seriesPath - domainCheck;
        domain = (char *) malloc(sizeof(char) * (charectersInDomain + 1));
        if (domain == NULL) {
            exit(21);
        }
        strncpy(domain, domainCheck, charectersInDomain);
        domain[charectersInDomain] = '\0';
    } else {
        //put other domain checks here
    }
    return domainUsed;
}

void set_save_directory(char *lastArg) {
    if (saveDirectory == NULL) {
        if (lastArg != NULL) {
            if (access(lastArg, F_OK) != -1) {
                if (access(lastArg, W_OK|R_OK) != -1) {
                    remainingUrls--;
                    saveDirectory = realpath(lastArg, NULL);
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

bool current_folder_update_mode (int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-' || argv[i][1] == 'u') {
            return false;
        } 
    }
    return true;
}

Site argument_check(int argc, char** argv) {
    Site domainUsed = other;
    if (current_folder_update_mode(argc, argv)) {
        usingSettings = true;
        set_save_directory_as_current();
        set_series_folder(NULL);
        domainUsed = read_settings();
    }
    remainingUrls = argc - 1;
    int firstUrl = 0;
    char *lastArg = NULL;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            remainingUrls--;
            bool update = process_flag(argv[i]+1);
            if (update) {
                if (++i >= argc) {
                    exit(1);
                }
                usingSettings = true;
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

int duplicate_and_continue(int argc, char **argv) {
    if(remainingUrls <= 0) {
        return 0;
    }
    int pid = fork();
    if (pid == -1) {
        exit(21);
    } else {
        //child
        execvp(argv[0], remove_string_from_array(argc, argv, currentUrl));
        exit(24);
    }
    //parent
    signal(SIGINT, SIG_IGN);
    close(0);
    int status;
    if ((wait(&status) == -1) || (WIFEXITED(status) == 0)) {
        exit(21);
    }
    return WEXITSTATUS(status);
}

int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, terminate_handler);
    on_exit(print_error, NULL);
    Site domainUsed = argument_check(argc, argv);
    set_source(domainUsed);
    if (usingSettings) {
        //start blacklist load thread
    }
    if (domainUsed == kissmanga) {
        //if initial page is invalid just skip it
        setup_kissmanga_download();
    } else if (domainUsed == other) {
        fprintf(stderr, "This url: %s is from an unsupported domain, skipping\n"
                , currentUrl);
    } 
    //join blacklist thread somehow
    save = true;
    download_entire_queue();
    save_settings();
    //save blacklist here
    //fork and continue is needed
    //return it's status not 0
    return duplicate_and_continue(argc, argv);
}

/*
OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOUENOOOOOOOOOOOOOOOOOOOOOOOOOMMMMMMMNNNNDNNNMNMMOM
OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOMMMMMMNNMNDDDDNMMMMOM
OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOONMMNMNNMMNMDDDNMMMMOM
OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOONNNMMMNNDDDDMMMNON
OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOONNNMMMNDDDDDMMM8MM
OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOMDNMMMMMDDNDMMMMDN
OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOONMNMMMMDDNMDMDMNDD
OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOONNMMNMMMDDNDMDNMMD
OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOONMNMNMNMDDNDDDDDDD
OOOOOOOOOOOOOOOOOOOOOOOOOOOZOZZZZOOOOOOOOOOOOOOOOOOOOOOOOOOOOONDDMNNDNDNNDDDDDDM
OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO8OOOOOOOOOOOOOOOOOOOONDDMNNDMMDNDDDNMMM
OOOOOOOOOOOOOOOOOOOOOOOOOOO8OOOOOOOOOOOOMMMNMMOOOOOOOOOOOOO8MODDDMNNDNDDDDDMMMMM
OOOOOOOOOOOOOOOOOOOOOOOOOOOO8OOOOOOOOOOONMMNNMOOOOOOOOOOOOOMMODDDNDDMNDDDMNMMMMM
OOOOOOOOOOOOOOOOOOOOOOOOOOO8OOOOOOOOONNONMNNNNOOOOOOOOOOOOOMMDDDMDDD8DNDMMMMMMMM
OOOOOOONOO8OOOOOZ8OOOOOOOOOOOOOOOZOOOONMMMMMMMOOOOOOOOOOOOOMMODN8D88NNMMMMMMMMMM
NOMNMMNNOONNNNNNNOOOOOOOOOOZOOOOOOZOOONOMMMMMMOOOOOOOOOOOOONM8MM8DMMMMMMMMMMMNNM
NNNDDNDMMMMNNNNNNNOOOOOON8OZOOOOOOOOZONNNNNNDMOZOOOOOOOOOOODD888MNMMMMMMMMNNMNNN
MMMMMMNNMMMMNNNNNMOOOOOONNMNNNNNNZOO88D8OOOOOOOOOOOOOOOOOOOM88MNMMMMNMMNNNNNMNNN
MMMMMMNND8MNDONMMNOOOOOZO,,,MODODZOOOOD8OOOOOOOOOOZOOOOOOOO8NMMMMMMNNNNNNNNNNDNN
DMM8DMNMMMDMD8MMNNOOOOOZO77?=~=Z$$ZZZO88OOOOOOOOOOOOOOOOOOOO8ON8MNNNDNNNDDDDNDDD
MMMMMMMMMN?NN8NMMNOOOOOOZ$ZOD$7,.O7NND..ZOOOOOOOOOOZOOOOOOOO8ODONDDD8DDD88DDDDDD
7MM=DMNODN+DDO.NMNNN8ZZ8$$Z$7$D7$$ZZOZZOOOOOOOOOOOOOOOO8DD88O88O888888888888D8DM
MMMMMNDMNM8DMD,OD7MNO88I8ZD88ZZZZZOO8O88OOOOOOOOOOOOO88DNNDDODZD8OOOZOONO88888NN
MMMMMMMMMN$8MM.8DNNO8I8ZZOZO$ZNNMNMMMMMNMNNMMMMMMM888O8D8DNNN8Z8OZZNOODNOOM888ND
MMMMMMNMMND8MD.88NNO+88OZ$ZN$Z888OOOOOOZOOOOOOOOO88NO8ODNNND8OZOOZZNZZDDOOMO88ND
MMMMMMNMMNN8,MDDDNNO+888O$88Z8NZZOOOOOO88OO8OOO888ODO8DDDDDD88Z8Z$$N$ZDDZOMO8OND
.MM~,MMMNNODND88DND$.$O+D$+I7OZ8ZOOOO888888888OO8D8NDD8888DD8DZOZ$$D$$8DZOMOOODD
NMMMMMMMMMDDOND8NNNZ,MMDN$OO$OZOZOOO8DNND8DDDD8888DN8NNNDNN88DZD$$7D$$8DZZMO8OMN
NMMNNMMMNM8DDMNDMMMMMMNNMZMD$D8DO8OOODD8DD8MD888888OD888OO8D8DNDN8D8$$8DNOMOOOND
MMMMMMMNDND8DMNMMMMMMMMNMOMMZDZ8Z8OOO88O888DDNNDDO8888D8OO8NDDN8D$$D$OND8ONOOOND
MMMMMMMNMMMNDNNNMMMMMMMNN8NNNNZZOOOO888OO88D8DDDDD88D88O8O8D88NM877DD$DD8DNO8OMD
+MM~~MMMMMNMDDDDMMMMMNNNM8MNMD?ZZ$OOO88ONND88DDDDD888888OONDD88$8$$$ONDD8O8OOOMN
MMMNMMNODNNMDNDDMMMM8NMNMDMDNNZOO$OOO8MMMMMMD88888DDDDDNDD8NO8N8D$77ZOZZ8O8D8DM8
8888NMMNMMN?MMMN8MNNMNNNN8MMMMZ$ZZOO8MMMNNMMM8888DDNDDMNMMNDNNDNDN$$ZZ8ZDO8O8O8O
NN8DNN88O8N8M8MN8NN88N?DM8NNMZOOOOOO8NMMMMMMN888D8DNM8NNNNND$DNMDDNNMDZZ8NOOOOOO
DDDNMDMNNDDDND888NNNNZMNNDNMMM$ZZOO8DDMMMMMMDDDD8ND8NNDN$NNDOD$7DNMDDZ$OMDDDDNNN
DD88DNDI8DDDNIDDN8DDMM88M8NNMMDZZOOO8D8MMNN8DDD8N8NONDDN$8DDOZZOMDDNNNNNNNNNNNDN
NDNDNDND8DDDDDNDDOD8ZDDDN8D888O7ZOO88NMNDDDM8MMM8DNON8ZONNDDNNNNN8DN8NNNNNNNNNND
NNNNNDNNDDDDDDD8DO88DDDO888DOZZ8888NDMMMMMMNNNNNODNDDD8DDDDNNNNMNNDNDN8NNNNNMNNN
DMNDDNNMDDDDD8O8D888DDDDDD8OO8O88NNNMDNMMMNMNNNMDDDDDD8NNNDMNMDDNNMN8N8MNDNDMMDN
8MMNNNMMDNDDD8$8DDNDDDDD8DO8NOD8NMMMNMMMMMMMMMNMNMNN8N8OZO8Z$ODNNNMDNNDDN8DNNDDN
DNNNNNDNNDDD88+8D88NDD8DDM8DNDNDDMMMMMMMMMMMMMMMMNNDNDDDDNMZZNNMO8D$DNNOO88DN8DN
DNMNN88OOO8NDNODDDMMDNNNMNNN888DNMMMMMMMMMMMMMMMM8NNNNMNDNNNNNNNNN888DNMN8D8DDZD
8DNNDM8ZDDDNMNDD8NNDNNNNNNDDNDDDNMMMMMMMMMMMMMMMMDDNNDNDMMNNDNNNNNNNNNNNM8888NNN
8NDDDNMM8O8DMNDNNDDNNNDDNDDDDD88MMMNMMMMMMMMMMMMMNMDDNDDNDNMMMMNN8NNDNNNDNNMNNMM
MN8D88MNDDNN8NNNDDDNNNDNNNN8NDDOMMMMMMMMMMMMMMMMMNNNM8DNNDNNNNNNMMNNMODNDNNNMNNN
NMMNNMMMNNNNNDNDDD8D8DNDDDD8DDMNMMMMMMMMMMMMMMMMMNNNMMN$8NNNNNNNNNMMNNNNMNZDNNND
NMNMMMNNNDDDNNDDDD8DDNDDDNDNOMMNNNNMMMNNMMMMMMMMMNNNNNMMNODNNNNNNNMMMNNNNNNNNDZD
NMNNNNDNNNNNNDNDNDDDNDNDNNNMMMMNNMNMMNMMMMMMMMMMMNNNNNNNMMN7NMNNNNMNNNNNNNMNMMNN
NNNNNMDNMNDNNNNNNDDNDNNNNONMMMNNMNMMMMMMMMMMMMMMMNNNNNNNNNMMD$NNNNNNNNDDNMNMMNMN
NDNNNNNDNNDNNDDNNDNDNDD8MNNNNNNNNMMMMMMMMMMMMMMMMNNNNDNNNNNMNMD7NNNNNNNNNNNNNDNN
NNNNNNDNDNNDDNDDDDDDDN8NMMNNNNNNNMMMMMMMMMMNNNNMMNNNNNDNNNNNNMNMDZNNMNNNNNDDNNNN
DMDNNND8DNNNDDDN8NMMZMNMMMNNNNNNNNNMMMMMMMMMMMMMMNNNNNNNNNMNNMMMNMNZMDMNNNMNNNNN
NNDDNNDDDDNNDND8MDN8MDMMMMMNNNNNDNOMMMMMMMMMMMMDMNNDDNNNNDNMMMMNMMNM88MDNNNNNNDN
NNNNNNDNDDDNNDNDNDMNMMMMMMNNNNNNMMOMMMMMMDMMMMMNNNNNNNMNNNNMMMMMMMMMMN8ZNNMDDNND
NDNNNNDNDNNNNNNO8MMMMMMMMNNNNNNNMZZMMMMMMNMMMMMDNDNNNNNNMNMNMMMMMMMMODNNZOMDNNDN
NNNNNNDDNDDNNNDMNMMMMMMMMNNNNNNNMDDNMMMMN8MMMMMDNNNNNNNNMMMNMMMMMMMMMMMMNDO8NNNN
NNNDMNNDNNMNODNDDMMMMMMMMMNMNNMNNDDDMMMMDDDMMMMNNNNMNNNMMMMMMMMMMMMMMMMMMMNNODMN
NNNNMDDNNNODMMMMMMMMMMMMMMNMMNNNDDDDMMMMDDDMMMMNNNNNNNNNNNMMMMMMMMMMMMMMMMMNMNZ8
NNDNMN8DDDMMMMMMMMMMMMMMMMMMMMMMNNNDMMMMNNNMMMMNNNMNNNNNNNMMMMMMMMMMMMMMMMMMNMND
NNDDMNN8DMMMNMMMMMMMMMMMMMMMMMMMNNNNMMMMMNNMMMMNNNNNNNNNNNNMMMMMMMMMMMMMMMMMMMMN
NNMMNN8MMMMMMMMMMMMMMMMMMMMMMMMDNDDDNMM8DONMMMMNNMNMNMMNMMMNMMMMMMMMMMMMMMMMMMMM
NDDN8DMMMMMMMMMMMMMMMMMMMMNMMMNNNNNNMMMNNNNMMMMNMNNMNNNNNNNNMMMMMMMMMMMMMMMMMMMM
NNDDMMMMMMMMMMMMMMMMMMMMMMMMMNNNNNNNMMMMMNNNMMMNNNNNNMNNNNNNNMMMMMMMMMMMMMMMMNDM
M8DMMMMMMMMMMMMMMMMMMMMMMMMMMNNNNNNNMMMMNNNNMMMNNMMMMNNMMMMMMMMMMMMMMMMMMMMMMMMM
DMMMMMMMMMMMMMMMMMNMMMMMMMMMMNMNNNMMMMMMNNNNMMMNNNNNNNMNNNNNNNNMMMMMMMMMMMMMMMMM
MMNDNMMMMMMMMMMMMMMMMMMNMMMMNNNNNNNNMMMMNNNNMMMNNNNNNNNNNNNNNNMMMMMNMMMMMMMMMMMM
NMMMMMMMMMMMMMMMMMMMMMMMMMMMNNDDNNDNNMMMNNNNMMMNNNNNNNNMMNNNNMNMMMMMMMMMMMMMMMMM
MMMMMMMMMMMMMMMMMMMMMMMMMMMNMMMMMMMMMMMMMMMMMMMMMMMMMMMNMMMMNNNNNMMMMMMMMMMMMMMM
MMMMMMMMMMMMMMMMMMMMMMMMMMMMNNNNNNNNMMMMNNNMMMMMMNNNMNMMNMMMMMMMMMMMMMMMMMMMMMMM
MMMMMMMMMMMMMMMMMMMMMMMMMMNMMMMMMNMMNNNMNMMNNNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMNNNMMMMNMMMMMMMMMMMM
MMMMMMMMMMMMMMMMMMMMMMMMMMNMNNMNNNMMMMMMMMMMMMMMNMMMNNNNMMNMMMMMMMMMMMMMMMMMMMMM
MMMMMMMMMMMMMMMMMMMMMMMMMMNNMMMNNNMMMMMMMNMMMMMMMNNMMMNMMNMNMMMNMMMMMMMMMMMMMMMM
MMMMMMMMMMMMMMMMMMMMMMMMNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMNMMMMMNMMMMMMMMMMM
MMMMMMMMMMMMMMMMMMMMMMMMMMMMMNMNNNMMMMMMNMNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM
MMMMMMMMMMMMMMMMMMMMMMMMNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMNMMMMMMMMMMMMMMMMMMMMMMM
*/
