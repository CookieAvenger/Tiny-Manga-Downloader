#include "networking.h"
#include "generalMethods.h"
#include "tmdl.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

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
    //if returns non-0, say either install pip and do pip install, or retry 
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
    char *cookieInfo = read_all_from_fd(fds[0]);
    printf("%s\n", cookieInfo);
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
                "and \"pip3\" instead of just \"pip\".", stderr);
        exit(25);
    } 
    remove(script);
    free(script);
}
