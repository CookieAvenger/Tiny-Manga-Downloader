#include "networking.h"
#include "generalMethods.h"
#include "tmdl.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

char *cookie = NULL;
char *userAgent = NULL;

//Some repitition here - tried to solve with get_substring method but couldn't
//as second substring requred end of first - will tackle again later
void update_cookie(char *cookieInfo) {
    if (cookie != NULL && userAgent != NULL) {
        free(cookie);
        free(userAgent);
    }
    char *startOfCookie = strstr(cookieInfo, "'");
    if (startOfCookie == NULL) {
        exit(23);
    }
    char *endOfCookie = strstr((startOfCookie + 1), "'");
    if (endOfCookie == NULL) {
        exit(23);
    }
    int charectersInCookie = strlen(startOfCookie) - strlen(endOfCookie) - 1;
    cookie = (char *) malloc(sizeof(char) * (charectersInCookie + 1));
    if (cookie == NULL) {
        exit(21);
    }
    strncpy(cookie, (startOfCookie + 1), charectersInCookie);
    cookie[charectersInCookie] = '\0';
    
    char *startOfAgent = strstr((endOfCookie + 1), "'");
    if (startOfAgent == NULL) {
        exit(23);
    }
    char *endOfAgent = strstr((startOfAgent + 1), "'");
    if (endOfAgent == NULL) {
        exit(23);
    }
    int charectersInAgent = strlen(startOfAgent) - strlen(endOfAgent) - 1;
    userAgent = (char *) malloc(sizeof(char) * (charectersInAgent + 1));
    if (userAgent == NULL) {
        exit(21);
    }
    strncpy(userAgent, (startOfAgent + 1), charectersInAgent);
    userAgent[charectersInAgent] = '\0';
    free(cookieInfo);
}

//Create python script - temporary solution
char *makeCookieScript() {
    char *scriptName = concat(get_save_directory(), "/cookiegrabber.py");
    FILE *cookieScript = fopen(scriptName, "w");
    if (cookieScript == NULL) {
        exit(3);
    }
    fputs("import cfscrape\n", cookieScript);
    fprintf(cookieScript,
            "print(cfscrape.get_cookie_string(\"http://%s\"))", 
            get_domain());
    fclose(cookieScript);
    return scriptName;
}

//Currently makes and runs a python script, intend to make in house at some
//point - current implementation needs cfscrape installed
void bypassDDOSprotection() {
    if (get_verbose()) {
        puts("Attempting to bypass cloudflares DDOS protection");
    }
    char *script = makeCookieScript();
    int fds[2];
    pipe(fds);
    int pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        dup2(fds[1], 1);
        execlp("python", "python", script, NULL); 
        //Exit 24 is python not installed but no point printing out
        //no one is reading the childs stderr
        exit(24);
    }
    //parent
    close(fds[1]);
    update_cookie(read_all_from_fd(fds[0])); 
    int status;
    //put here || WIFEXITED
    if ((wait(&status) == -1) || (WIFEXITED(status) == 0)) {
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
    remove(script);
    free(script);
    if (get_verbose()) {
        puts("Bypassed cloudflare DDOS protection");
    }
}

char *handle_codes(char *page);

char *get_kissmanga_page(char *file) {
    char *page = NULL;
    do {
        if (page != NULL) {
            free(page);
            if (get_verbose()) {
                fputs("Failed to access kissmanga, trying again.", stderr);
            }
            bypassDDOSprotection();
        }
        int fd = send_HTTP_request(file, cookie, userAgent);
        page = read_all_from_fd(fd);
        page = handle_codes(page);
        if (page == NULL) {
            return NULL;
        }
    } while (strncmp(page + 9, "503 Service Temporarily Unavailable",
                35) == 0); //+9 is to get past HTTP/1.1
    return page;
}

char *handle_codes(char *page) {
    //3XX error codes are redirects
    if (strncmp(page + 9, "3", 1) == 0) {
        char *redirectTo = get_substring(page, "Location: ", "\n", 6);
        return get_kissmanga_page(redirectTo);
    //4XX error codes are issues like page doesnt exist ect.
    } else if (strncmp(page + 9, "4", 1) == 0) {
        return NULL;
    }
    return page;
}

void download_kissmanga_series(char *randomChapterLink) {
    
}

void start_kissmanga_download() {
    bypassDDOSprotection();
    char *test = get_kissmanga_page(get_series_path());
    if (test == NULL) {
        exit(6);
    }
    char *testString = (char *) malloc(sizeof(char) * 
            strlen(get_series_path()) + 7);
    sprintf(testString, "href=\"%s/", get_series_path());
    char *result = strstr(test, testString);
    //Need a better way of testing
    if (result != NULL) {
        //Series Page
        //Now we open any chapter
        char *chapterLink = get_substring(test, testString, "\"", 6);
        free(testString);
        free(test);
        download_kissmanga_series(chapterLink);
    } else {
        //Chapter Page
        printf("%s\n", test);
        free(testString);
        free(test);
    }
}
