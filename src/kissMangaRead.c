#include "networking.h"
#include "generalMethods.h"
#include "tmdl.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "chaptersToDownload.h"
#include "customParser.h"
#include <signal.h>
#include "kissMangaDownload.h"

//kissmanga cookie
char *cookie = NULL;
//agent set with cookie
char *userAgent = NULL;
//location of cookie grabbing script
char *script = NULL;

//ca.js location
char *caJs = NULL;
//lo.js location
char *loJs = NULL;
char *decryptionScript = NULL;
char **currentKeys = NULL;
pid_t decryptionProcess = 0;
FILE *scriptRead;
FILE *scriptWrite;

//Return kissmanga cookie
char *get_kissmanga_cookie() {
    return cookie;
}

//return kissmanga agent
char *get_kissmanga_useragent() {
    return userAgent;
}

//return saved script path
char *get_python_script_location() {
    return script;
}

//Parse and set cookie information
void update_cookie(int fd) {
    if (cookie != NULL && userAgent != NULL) {
        free(cookie);
        free(userAgent);
    }
    FILE *cookieInfo = fdopen(fd, "r");
    if (cookieInfo == NULL) {
        exit(21);
    }
    cookie = read_from_file(cookieInfo, '\n', true);
    if (cookie == NULL || cookie[0] == '\0') {
        exit(23);
    }
    userAgent = read_from_file(cookieInfo, '\n', true);
    if (userAgent == NULL || userAgent[0] == '\0') {
        exit(23);
    }
    fclose(cookieInfo);
}

//Create python script - temporary solution
char *makeCookieScript() {
    char *scriptName = concat(get_save_directory(), "/.cookiegrabber.py");
    FILE *cookieScript = fopen(scriptName, "w");
    if (cookieScript == NULL) {
        exit(3);
    }
    fputs("import cfscrape\n", cookieScript);
    fprintf(cookieScript,
            "cookie_value, user_agent = cfscrape.get_cookie_string(\"http://%s\")"
            , get_domain());
    fputs("\nprint(cookie_value)\nprint(user_agent)\n", cookieScript);
    fflush(cookieScript);
    fclose(cookieScript);
    return scriptName;
}

//Currently to get past kissmanga's DDOS protection makes and runs a python 
//script, intend to make in house at somepoint 
//- current implementation needs cfscrape installed
void bypass_DDOS_protection() {
    if (get_verbose()) {
        puts("Attempting to bypass cloudflares DDOS protection");
        fflush(stdout);
    }
    script = makeCookieScript();
    int fds[2];
    if (pipe(fds) == -1) {
        exit(21);
    }
    pid_t pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        dup2(fds[1], 1);
        close(fds[0]), close(2);
        execlp("python2.7", "python2.7", script, NULL); 
        execlp("python", "python", script, NULL); 
        //Exit 24 is python not installed but no point printing out
        //no one is reading the childs stderr
        exit(24);
    }
    //parent
    close(fds[1]);
    int status;
    //put here || WIFEXITED
    if ((waitpid(pid, &status, 0) == -1) || (WIFEXITED(status) == 0)) {
        exit(21);
    }
    if (WEXITSTATUS(status) != 0) {
        fputs("Failed to run program to bypass cloudflares DDOS protection\n"
                "Ensure the latest version of cfscrape and python 2.7 is installed and kissmanga is up - otherwise just try again - cloudflare sometimes put sadditional security randomly\n"
                "To install cfscrape please run the following command:\n"
                "sudo -H pip install cfscrape\n"
                "Some systems may require to install with \"pip2\" "
                "instead of just \"pip\".\n", stderr);
        exit(25);
    } 
    update_cookie(fds[0]); 
    remove(script);
    char *toFree = script;
    script = NULL;
    free(toFree);
    if (get_verbose()) {
        puts("Bypassed cloudflares DDOS protection");
        fflush(stdout);
    }
}

//Read a kissmanga page
char *get_kissmanga_page(char *file) {
    char *page = NULL;
    do {
        if (page != NULL) {
            free(page);
            if (get_verbose()) {
                fputs("Failed to access kissmanga, trying again.\n", stderr);
            }
            bypass_DDOS_protection();
        }
        int fd = send_HTTP_request(get_domain(), file, cookie, userAgent);
        page = read_all_from_fd(fd, false);
        page = handle_error_codes(page);
        if (page == NULL) {
            return NULL;
        }
    } while (strncmp(page + 9, "503 Service Temporarily Unavailable",
                35) == 0); //+9 is to get past HTTP/1.1
    return page;
}

//May seem weird cuz I can do it better for kissmanga series page
//But this is for support of just a chapter page given
void parse_and_set_kissmanga_series_folder(char *chapterPage) {
    char *folder = get_substring(chapterPage, "Read manga\n", 
            "\n", 26);
    decode_html_entities_utf8(folder, NULL);
    set_series_folder(folder);
    if (folder != NULL) {
        free(folder);
    }
}

void setup_page_decryption();

//private method only ever used by run_page_decryption
void start_decryption_program() {
    //starting up program
    if (get_verbose()) {
        puts("Downloading and starting up chapter decryption scripts for kissmanga...");
        fflush(stdout);
    }
    if (decryptionScript == NULL) {
        setup_page_decryption(); 
    }
    int fds[2];
    int fds2[2];
    if ((pipe(fds) == -1) || (pipe(fds2) == -1)) {
        exit(21);
    }
    decryptionProcess = fork();
    if (decryptionProcess == -1) {
        exit(21);
    } else if (decryptionProcess == 0) {
        //child
        dup2(fds[1], 1), dup2(fds2[0], 0);
        close(2);
        execlp("python2.7", "python2.7", decryptionScript, NULL); 
        execlp("python2", "python2", decryptionScript, NULL); 
        //Exit 24 is python not installed but no point printing out
        //no one is reading the childs stderr
        exit(24);
    }
    //parent
    close(fds[1]), close(fds2[0]);
    scriptRead = fdopen(fds[0], "r");
    scriptWrite = fdopen(fds2[1], "w");
    char *readyCheck = read_from_file(scriptRead, '\n', false);
    if (readyCheck == NULL || strcmp(readyCheck, "ready") != 0) {
        waitpid(decryptionProcess, NULL, 0);
        decryptionProcess = 0;
        fputs("Failed to run program to decrypt kissmanga links\n"
                "Make sure you have python 2.7 installed and not just 3.5,"
                " otherwise please send an error report\n", stderr); 
        exit(25);
    }
    puts("Decryption section successfully up and running");
    fflush(stdout);
}

//Manages newKeys memory, and return its answer in encryptedSites
char **run_page_decryption(char **newKeys, char **encryptedSites) {
//caller of this needs to print message decrupting what chapter
//set it up so if its not already running, starts it up, then send input
    //decryption pages of chapter... 
    if (decryptionProcess == 0) {
        start_decryption_program();
    } 
    //if keys entered are same as what I already have, don't run kets again
    //check keys with compare_arrays
    //also in cleanup remember to free the keys array
    if (get_verbose()) {
        printf("Decrypting links for chapter %zu/%zu\n",
                get_current_download_chapter(), get_download_length());
        fflush(stdout);
    }
    bool runKeys = false;
    if (currentKeys == NULL) {
        currentKeys = newKeys;
        runKeys = true;
    } else if (!compare_arrays((void **)currentKeys, (void **)newKeys, string_comparator_wrapper)) {
        pointer_array_free((void **) currentKeys);
        currentKeys = newKeys; 
        runKeys = true;
    } else {
        pointer_array_free((void **) newKeys);
    }
    if (runKeys) {
        for (size_t i = 0; currentKeys[i] != NULL; i++) {
            fprintf(scriptWrite, "x\n%s\n", currentKeys[i]);
        }
    }
    for (size_t i = 0; encryptedSites[i] != NULL; i++) {
        fprintf(scriptWrite, "v\n%s\n", encryptedSites[i]);
        fflush(scriptWrite);
        free(encryptedSites[i]);
        encryptedSites[i] = read_from_file(scriptRead, '\n', true);
    }
    return encryptedSites;
}

//a method that stops program from executing to be run after while loops in download enture queue
void stop_decryption_program() {
    //only stops if still running
    if (decryptionProcess == 0) {
        return;
    }
    if (get_verbose()) {
        puts("Finished for now, stopping decryption program");
        fflush(stdout);
    }
    free_kissmanga_regex();
    if (currentKeys != NULL) {
        pointer_array_free((void **) currentKeys);
        currentKeys = NULL;
    }
    fputs("q\n", scriptWrite);
    fflush(scriptWrite);
    char *finishCheck = read_from_file(scriptRead, '\n', false);
    if (finishCheck == NULL || strcmp(finishCheck, "finished") != 0) {
        //otherwise kill
        kill(decryptionProcess, SIGKILL);
    }
    waitpid(decryptionProcess, NULL, 0);
    //close read write
    fclose(scriptRead), fclose(scriptWrite);
    decryptionProcess = 0;
}

void clean_up_page_decryption() {
    stop_decryption_program(); 
    if (caJs != NULL) {
        remove(caJs);
        free(caJs);
        caJs = NULL;
    }
    if (loJs != NULL) {
        remove(loJs);
        free(loJs);
        loJs = NULL;
    }
    if (decryptionScript != NULL) {
        remove(decryptionScript);
        free(decryptionScript);
        decryptionScript = NULL;
    }
    if (currentKeys != NULL) {
        pointer_array_free((void **) currentKeys);
        currentKeys = NULL;
    }
}

void try_decryption_setup_again() {
    fputs("Failed to download required scripts from kissmanga, trying again\n", stderr);
    clean_up_page_decryption();
    bypass_DDOS_protection();
    setup_page_decryption();
}

//Should only have to download the scripts once
void setup_page_decryption() {
    char *caPage = "/Scripts/ca.js";     
    char *loPage = "/Scripts/lo.js";     

    caJs = concat(get_series_folder(), ".ca.js");
    FILE *caSave = fopen(caJs, "w");
    if (caSave == NULL) {
        exit(3);
    }
    //Same hack as in kissmanga downloader, we do care alot if it fails, so we
    //check it doesn't
    int fd = send_HTTP_request(get_domain(), caPage, cookie, userAgent);
    char buffer[12] = {'\0'};
    int seen = read(fd, buffer, 11);
    //quick check as handle error codes, but just doesn't follow redirects
    if (seen < 11 || *(buffer + 9) != '2') {
        try_decryption_setup_again();
        return;
    }
    save_url_as_file(fd, caSave);
    fflush(caSave);
    fclose(caSave);

    loJs = concat(get_series_folder(), ".lo.js");
    FILE *loSave = fopen(loJs, "w");
    if (loSave == NULL) {
        exit(3);
    }
    fd = send_HTTP_request(get_domain(), loPage, cookie, userAgent);
    memset(buffer, '\0', sizeof(char)*12);
    seen = read(fd, buffer, 11);
    if (seen < 11 || *(buffer + 9) != '2') {
        try_decryption_setup_again();
        return;
    }
    save_url_as_file(fd, loSave);
    fflush(loSave);
    fclose(loSave); 

    //now write the python script
    decryptionScript = concat(get_series_folder(), ".decryptionMachine.py");
    FILE *decryptWrite = fopen(decryptionScript, "w");
    if (decryptWrite == NULL) {
        exit(3);
    }
    fputs("import js2py\nimport sys\n\n"
            "caCont = js2py.get_file_contents(\".ca.js\")\n"
            "loCont = js2py.get_file_contents(\".lo.js\")\n"
            "js = js2py.EvalJs()\n"
            "js.execute(\"\"\"escape = function(text){pyimport urllib; return urllib.quote(text)};\n"
            "unescape = function(text){pyimport urllib; return urllib.unquote(text)};\n"
            "encodeURI = function(text){pyimport urllib; return urllib.quote(text, safe='~@#$&()*!+=:;,.?/\\\\'')};\n"
            "decodeURI = unescape;\n"
            "encodeURIComponent = function(text){pyimport urllib; return urllib.quote(text, safe='~()*!.\\\\'')};\n"
            "decodeURIComponent = unescape;\"\"\")\n"
            "js.execute(caCont)\njs.execute(loCont)\n"
            "sys.stdout.write('ready\\n')\nsys.stdout.flush()\n\n"
            "while True:\n"
            "   line = sys.stdin.readline()\n"
            "   if line == \"x\\n\":\n"
            "       read = sys.stdin.readline()\n"
            "       js.execute(read)\n"
            "   elif line == \"v\\n\":\n"
            "       read = sys.stdin.readline()\n"
            "       sys.stdout.write(js.eval(read)+\"\\n\")\n"
            "       sys.stdout.flush()\n"
            "   elif line == \"q\\n\":\n"
            "       sys.stdout.write('finished\\n')\n"
            "       quit(0)\n"
            "   else:\n"
            "       quit(1)\n", decryptWrite);
    fflush(decryptWrite);
    fclose(decryptWrite);
}
