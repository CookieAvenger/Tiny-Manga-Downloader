#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

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

void delete_folder(char *folder) {
    int pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        execlp("rm", "rm", "-rf", folder, NULL);
        exit(24); 
    }
    //parent
    int status;
    if (wait(&status) == -1) {             
        exit(21);                                                        
    }                                                                    
    //should never fail, we have read write permission to that folder
}                                                                        

void create_folder(char *folder) {                                         
    int pid = fork();                                                    
    if (pid == -1) {                                                     
        exit(21);                                                        
    } else if (pid == 0) {                                               
        //child                                                          
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
        //If it already exists then still return 0 - so all g            
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

char *read_all_from_fd(int fd) {
    FILE *source = fdopen(fd, "r");
    if (source == NULL) {
        exit(21);
    }
    //classic annoying read here 
    int next, dynamic = 4, count = 0;
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
    text[count] = '\0';
    fclose(source);
    return text;
}
