#include "networking.h"
#include "generalMethods.h"
#include "tmdl.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "chaptersToDownload.h"

char *cookie = NULL;
char *userAgent = NULL;

//Some repitition here - tried to solve with get_substring method but couldn't
//as second substring requred end of first - will tackle again later
int update_cookie(char *cookieInfo) {
    if (cookie != NULL && userAgent != NULL) {
        free(cookie);
        free(userAgent);
    }
    char *startOfCookie = strstr(cookieInfo, "'") + 1;
    if (startOfCookie == NULL) {
        return 23;
    }
    char *endOfCookie = strstr((startOfCookie + 1), "'");
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
    
    char *startOfAgent = strstr((endOfCookie + 1), "'") + 1;
    if (startOfAgent == NULL) {
        return 23;
    }
    char *endOfAgent = strstr(startOfAgent, "'");
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
void bypass_DDOS_protection() {
    if (get_verbose()) {
        puts("Attempting to bypass cloudflares DDOS protection");
    }
    char *script = makeCookieScript();
    int fds[2];
    pipe(fds);
    int pid = fork();
    if (pid == -1) {
        remove(script);
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
    int error = update_cookie(read_all_from_fd(fds[0])); 
    if (error != 0) {
        remove(script);
        exit(error);
    }
    int status;
    //put here || WIFEXITED
    if ((wait(&status) == -1) || (WIFEXITED(status) == 0)) {
        remove(script);
        exit(21);
    }
    if (WEXITSTATUS(status) != 0) {
        fputs("If the latest version of cfscrape and a version of python is " 
                "already installed please just rerun your last command.\n"
                "To install cfscrape please run the following command:\n"
                "sudo -H pip install cfscrape\n"
                "Some systems may require to install with both \"pip2\" "
                "and \"pip3\" instead of just \"pip\".\n", stderr);
        remove(script);
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
                fputs("Failed to access kissmanga, trying again.\n", stderr);
            }
            bypass_DDOS_protection();
        }
        int fd = send_HTTP_request(get_domain(), file, cookie, userAgent);
        page = read_all_from_fd(fd);
        page = handle_codes(page);
        if (page == NULL) {
            return NULL;
        }
    } while (strncmp(page + 9, "503 Service Temporarily Unavailable",
                35) == 0); //+9 is to get past HTTP/1.1
    return page;
}

char *get_kissmanga_chapter(char *link) {
    char *path = concat(get_series_path(), "/");
    char *chapter = concat(path, link);
    free(path);
    free(link);
    char *page = get_kissmanga_page(chapter);
    free(chapter);
    return page;
}

char *handle_codes(char *page) {
    //3XX error codes are redirects
    if (strncmp(page + 9, "3", 1) == 0) {
        char *redirectTo = get_substring(page, "Location: ", "\n", 6);
        free(page);
        page = get_kissmanga_page(redirectTo);
        free(redirectTo);
        return page;
    //4XX error codes are issues like page doesnt exist ect.
    } else if (strncmp(page + 9, "4", 1) == 0) {
        return NULL;
    }
    return page;
}

void parse_and_set_series_folder(char *chapterPage) {
    char *testString = (char *) malloc(sizeof(char) * (strlen(get_domain())
            + strlen(get_series_path()) + 25));
    if (testString == NULL) {
        exit(21);
    }
    sprintf(testString, "<a href=\"http://%s%s\">\nManga\n", get_domain(),
            get_series_path());
    char *folder = get_substring(chapterPage, testString, 
            "\ninformation</a>", 26);
    free(testString);
    set_series_folder(folder);
}
