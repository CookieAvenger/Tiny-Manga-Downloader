#include "networking.h"
#include <sys/stat.h>
#include "generalMethods.h"
#include "tmdl.h"
#include <stdlib.h>
#include <stdio.h>

char *makeCookieScript() {
    char *scriptName = concat(get_save_directory(), "cookiegrabber.py");
    FILE *cookieScript = fopen(scriptName, "w");
    if (fopen == NULL) {
        exit(3);
    }
    fputs("import cfscrape\n", cookieScript);
    fprintf(cookieScript,
            "print(cfscrape.get_cookie_string(\"http:\/\/%s\"))", 
            get_domain());
    fclose(cookieScript);
    return scriptName;
}

//Currently makes and runs a python script, intend to make in house at some
//point - current implementation needs cfscrape installed
void bypassDDOSprotection() {
    //if returns non-0, say either install pip and do pip install, or retry 
    char *script = makeCookieScript();
    
    remove(script);
    free(script);
}
