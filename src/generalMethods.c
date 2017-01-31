#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/stat.h>

int sigintEvent = 0;

void sigint_enumerate() {
    sigintEvent++;
}

void enter_critical_code() {
    signal(SIGINT, sigint_enumerate);
}

void exit_critical_code() {
    signal(SIGINT, terminate_handler);
    if (sigintEvent > 0) {
        terminate_handler(2);
    }
}

//Kudos to http://stackoverflow.com/users/140740/digitalross
char *rstrstr(char *s1, char *s2) {
    size_t  s1len = strlen(s1);
    size_t  s2len = strlen(s2);
    if (s2len > s1len) {
        return NULL;
    }
    for (char *s = s1 + s1len - s2len; s >= s1; --s) {
        if (strncmp(s, s2, s2len) == 0) {
            return s;
        }
    }
    return NULL;
}

void delete_file (char *path) {
    int pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        close(1), close(2);
        execlp("rm", "rm", "-f", path, NULL);
        exit(24); 
    }
    //parent
    int status;
    wait(&status);             
    //should never fail, we have read write permission to that folder
}

void delete_folder (char *folder, int error) {
    int pid = fork();
    if (pid == -1) {
        if (error != -1) {
            exit(21);
        }
    } else if (pid == 0) {
        //child
        close(1), close(2);
        execlp("rm", "rm", "-rf", folder, NULL);
        exit(24); 
    }
    //parent
    int status;
    if ((wait(&status) == -1) || (WIFEXITED(status) == 0)) {             
        if (error != -1) {
            exit(21);
        }                                                        
    }
    if (WEXITSTATUS(status) != 0) {
        if (error != -1) {
            //is this the right error :/
            exit(3);
        }
    }                 
    //should never fail, we have read write permission to that folder
}                                                                        

void create_folder(char *folder) {                                         
    int pid = fork();                                                    
    if (pid == -1) {                                                     
        exit(21);                                                        
    } else if (pid == 0) {                                               
        //child                                                          
        close(1), close(2);
        execlp("mkdir", "mkdir", "-p", folder, NULL);           
        exit(24);                                                        
    }                                                                    
    //parent                                                             
    int status;                                                          
    if ((wait(&status) == -1) || (WIFEXITED(status) == 0)) {             
        exit(21);                                                        
    }                                                                    
    if (WEXITSTATUS(status) != 0) {                                      
        //This can only happen if something is a file instead of a folder
        //as we already know the file exists and we can write to it      
        exit(6);                                                         
    }                                                                    
}                                                                        

char *get_substring(char *string, char *start, char *end, int error) {
    char *startOfSubstring = strstr(string, start);
    if (startOfSubstring == NULL) {
        if (error == -1) {
            return NULL;
        }
        exit(error);
    }
    startOfSubstring += strlen(start);
    char *endOfSubstring = strstr(startOfSubstring, end);
    if (endOfSubstring == NULL) {
        if (error == -1) {
            return NULL;
        }
        exit(error);
    }
    size_t charectersInSubstring = endOfSubstring - startOfSubstring;
    char *substring = (char *) malloc(sizeof(char) *
            (charectersInSubstring + 1));
    if (substring == NULL) {
        exit(21);
    }
    strncpy(substring, startOfSubstring, charectersInSubstring);
    substring[charectersInSubstring] = '\0';
    return substring;
}

char **continuous_substring(char *string, char *start, char *end) {
    int count = 0;
    int dynamic = 4;
    char **substringsFound = (char **) malloc(sizeof(char *) * dynamic);
    if (substringsFound == NULL) {
        exit(21);
    }
    char *startOfSubstring;
    char *endOfSubstring;
    int startLength = strlen(start), endLength = strlen(end);
    int remainingLength = strlen(string);
    //0 == 0 kinda looks like a weird face... no other reason it's not 1
    while (0 == 0) {
        if (string == NULL) {
            break;
        }
        startOfSubstring = strstr(string, start);
        if (startOfSubstring == NULL) {
            break;
        }
        remainingLength -= (startOfSubstring - string);
        if (remainingLength > startLength) {
            startOfSubstring += startLength;
            remainingLength -= startLength;
        } else {
            break;
        }
        endOfSubstring = strstr(startOfSubstring, end);
        if (endOfSubstring == NULL) {
            break;
        }
        remainingLength -= (endOfSubstring - startOfSubstring);
        size_t charectersInSubstring = endOfSubstring - startOfSubstring;
        char *substring = (char *) malloc(sizeof(char) *
                (charectersInSubstring + 1));
        if (substring == NULL) {
            exit(21);
        }
        strncpy(substring, startOfSubstring, charectersInSubstring);
        substring[charectersInSubstring] = '\0';
        
        count ++;
        if (count == dynamic) {
            dynamic *= 2;
            substringsFound = (char **) realloc(substringsFound, 
                    sizeof(char *) * dynamic);
            if (substringsFound == NULL) {
                exit(21);
            }
        }
        substringsFound[count - 1] = substring;

        if (remainingLength > endLength) {
            string = endOfSubstring + endLength;
            remainingLength -= endLength;
        }
    }
    substringsFound[count] = NULL;
    return substringsFound;
}

int get_string_array_length(char **stringArray) {
    int count = 0;
    while(stringArray[count] != NULL) {
        count++;
    }
    return count;
}

void string_array_free(char **stringArray) {
    int length = get_string_array_length(stringArray);
    for (int i = 0; i < length; i ++) {
        free(stringArray[i]);
    }
    free(stringArray);
}


bool is_file(const char* name) {
    struct stat sb;
    if (stat(pathname, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        return false;
    }
    return true;
}

char *make_permenent_string(char *string) {
    char *persistantString = malloc(sizeof(char) * strlen(string));
    if (persistantString == NULL) {
        exit(21);
    }
    strcpy(persistantString, string);
    return persistantString;
}

char* concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1)+strlen(s2)+1);
    if (result == NULL) {
        exit(21);
    }
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

//for use with hash reading cuz gives perfect sized array back
char *read_from_file(FILE *source, int end, bool perfectSize) {
    int next;
    unsigned long dynamic = 4, count = 0;
    char *text = (char *) malloc(sizeof(char) * dynamic);
    if (text == NULL) {
        exit(21);
    }
    while(next = fgetc(source), (next != end)) {
        if (next == EOF) {
            if (count == 0) {
                free(text);
                return NULL;
            }
            break;
        }
        count++;
        if (count == dynamic) {
            dynamic *= 2;
            text = (char *) realloc(text, sizeof(char) * dynamic);
            if (text == NULL) {
                exit(21);
            }
        }
        text[count - 1] = (char) next;
    }
    text[count++] = '\0';
    if (perfectSize) {
        text = (char *) realloc(text, sizeof(char) * count);
    }
    return text;
}

char *read_all_from_fd(int fd, bool perfectSize) {
    FILE *source = fdopen(fd, "r");
    if (source == NULL) {
        exit(21);
    }
    //classic annoying read here 
    int next;
    unsigned long dynamic = 4, count = 0;
    char *text = (char *) malloc(sizeof(char) * dynamic);
    if (text == NULL) {
        exit(21);
    }
    while(next = fgetc(source), next != EOF) {
        count++;
        if (count == dynamic) {
            dynamic *= 2;
            text = (char *) realloc(text, sizeof(char) * dynamic);
            if (text == NULL) {
                exit(21);
            }
        }
        text[count - 1] = (char) next;
    }
    text[count++] = '\0';
    if (perfectSize) {
        text = (char *) realloc(text, sizeof(char) * count);
    }
    fclose(source);
    return text;
}

//Kudos to http://stackoverflow.com/users/44065/jmucchiello
// You must free the result if result is non-NULL.
char *str_replace(char *original, char *replace, char *alternative) {
    char *result; // the return string
    char *insert;    // the next insert point
    char *temporary;    // varies
    int replaceLength;  // length of rep (the string to remove)
    int alternativeLength; // length of with (the string to replace rep with)
    int differenceLength; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!original || !replace) {
        return NULL;
    }
    replaceLength = strlen(replace);
    if (replaceLength == 0) {
        return NULL; // empty rep causes infinite loop during count
    }
    if (alternative == NULL) {
        alternative = "";
    }
    alternativeLength = strlen(alternative);

    // count the number of replacements needed
    insert = original;
    for (count = 0; (temporary = strstr(insert, replace)) != NULL; ++count) {
        insert = temporary + replaceLength;
    }

    temporary = result = malloc(strlen(original) + (alternativeLength - 
            replaceLength) * count + 1);

    if (result == NULL) {
        exit(21);
    }

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        insert = strstr(original, replace);
        differenceLength = insert - original;
        temporary = strncpy(temporary, original, differenceLength) + differenceLength;
        temporary = strcpy(temporary, alternative) + alternativeLength;
        original += differenceLength + replaceLength; // move to next "end of rep"
    }
    strcpy(temporary, original);
    return result;
}

//For specific use with argv otherwise need to copy each char * individually
char **remove_string_from_array(int originalLength, char **originalArray
        , char *toRemove) {
    //if works right then just one thing will be removed and need a null so
    //just original length +1 is in case nothing gets removed
    char **finalArray = (char **) malloc(sizeof(char *) * (originalLength + 1));
    if (finalArray == NULL) {
        exit(21);
    }
    int j = 0;
    for (int i = 0; i < originalLength; i++) {
        if (strcmp(originalArray[i], toRemove) != 0) {
            finalArray[j] = originalArray[i];
            j++;
        }
    }
    finalArray[j] = NULL;
    return finalArray;
}
