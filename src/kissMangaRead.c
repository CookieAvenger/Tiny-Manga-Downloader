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

//kissmanga cookie
char *cookie = NULL;
//agent set with cookie
char *userAgent = NULL;
//location of cookie grabbing script
char *script = NULL;

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
int update_cookie(char *cookieInfo) {
    if (cookie != NULL && userAgent != NULL) {
        free(cookie);
        free(userAgent);
    }
    char *startOfCookie = strchr(cookieInfo, '\'');
    if (startOfCookie++ == NULL) {
        return 23;
    }
    char *endOfCookie = strchr(startOfCookie, '\'');
    if (endOfCookie == NULL) {
        return 23;
    }
    size_t charectersInCookie = endOfCookie - startOfCookie;
    cookie = (char *) malloc(sizeof(char) * (charectersInCookie + 1));
    if (cookie == NULL) {
        return 21;
    }
    strncpy(cookie, startOfCookie, charectersInCookie);
    cookie[charectersInCookie] = '\0';
    
    char *startOfAgent = strchr((endOfCookie + 1), '\'');
    if (startOfAgent++ == NULL) {
        return 23;
    } 
    char *endOfAgent = strchr(startOfAgent, '\'');
    if (endOfAgent == NULL) {
        return 23;
    }
    size_t charectersInAgent = endOfAgent - startOfAgent;
    userAgent = (char *) malloc(sizeof(char) * (charectersInAgent + 1));
    if (userAgent == NULL) {
        return 21;
    }
    strncpy(userAgent, startOfAgent, charectersInAgent);
    userAgent[charectersInAgent] = '\0';
    free(cookieInfo);
    return 0;
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
            "print(cfscrape.get_cookie_string(\"http://%s\"))", 
            get_domain());
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
    pipe(fds);
    pid_t pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        dup2(fds[1], 1);
        close(fds[0]), close(2);
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
        fputs("If the latest version of cfscrape and a version of python is " 
                "already installed please just rerun your last command.\n"
                "To install cfscrape please run the following command:\n"
                "sudo -H pip install cfscrape\n"
                "Some systems may require to install with both \"pip2\" "
                "and \"pip3\" instead of just \"pip\".\n", stderr);
        exit(25);
    } 
    int error = update_cookie(read_all_from_fd(fds[0], false)); 
    close(fds[0]);
    if (error != 0) {
        exit(error);
    }
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
}
