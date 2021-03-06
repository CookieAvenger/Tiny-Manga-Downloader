#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "tmdl.h"
#include <sys/types.h>
#include <dirent.h>
#include "customParser.h"
#include <ctype.h>
#include <regex.h>
#include "generalMethods.h"

//Kudos to http://stackoverflow.com/users/140740/digitalross
//DOES NOT RETURN NEW MALLOC STRING
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

//Delets a folder and it's contents
void delete_folder (char *folder, int error) {
    if (folder == NULL) {
        return;
    }
    pid_t pid = fork();
    if (pid == -1) {
        if (error != -1) {
            exit(21);
        }
    } else if (pid == 0) {
        //child
        close(0), close(1), close(2);
        execlp("rm", "rm", "-rf", folder, NULL);
        exit(24); 
    }
    //parent
    int status;
    if ((waitpid(pid, &status, 0) == -1) || (WIFEXITED(status) == 0)) {             
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

//Makes a folder and it's parents if needed
void create_folder(char *folder) {
    pid_t pid = fork();
    if (pid == -1) {
        exit(21);
    } else if (pid == 0) {
        //child
        close(0), close(1), close(2);
        execlp("mkdir", "mkdir", "-p", folder, NULL);
        exit(24);
    }
    //parent
    int status;
    if ((waitpid(pid, &status, 0) == -1) || (WIFEXITED(status) == 0)) {
        exit(21);
    }
    if (WEXITSTATUS(status) != 0) {
        //This can only happen if something is a file instead of a folder
        //as we already know the file exists and we can write to it
        exit(6);
    }
}

//Searched searched a string and return substring between two searches
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

//Searches a string continously for substrings
char **continuous_substring(char *string, char *start, char *end) {
    size_t count = 0;
    size_t dynamic = 4;
    char **substringsFound = (char **) malloc(sizeof(char *) * dynamic);
    if (substringsFound == NULL) {
        exit(21);
    }
    char *startOfSubstring;
    char *endOfSubstring;
    size_t startLength = strlen(start), endLength = strlen(end);
    size_t remainingLength = strlen(string);
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
        
        if (++count == dynamic) {
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

//Writes an array to file with something before, after and inbetween array
void write_string_array_to_file (char *initial, char **strings, 
        char *betweenArray, char *end, FILE *toWriteTo) {
    if (initial != NULL) {
        fprintf(toWriteTo, "%s", initial);
    }
    size_t count = 0;
    char *toCheck = strings[count];
    while (toCheck != NULL) {
        fprintf(toWriteTo, "%s", toCheck);
        toCheck = strings[++count];
        if (toCheck != NULL && betweenArray != NULL) {
            fprintf(toWriteTo, "%s", betweenArray);
        }
    }
    if (end != NULL) {
        fprintf(toWriteTo, "%s", end);
    }
}

//decodes html entities in array and returns the size of the array
size_t run_html_decode_on_strings(char **strings) {
    size_t count = 0;
    while(strings[count] != NULL) {
        decode_html_entities_utf8(strings[count], NULL);
        count++;
    }
    return count;
}

//length of a pointer array
size_t get_pointer_array_length(void **pointerArray) {
    size_t count = 0;
    while(pointerArray[count] != NULL) {
        count++;
    }
    return count;
}
 
//Frees pointer array
void pointer_array_free (void **pointerArray) {
    size_t count = 0;
    while (pointerArray[count] != NULL) {
        free(pointerArray[count]);
        count++;
    }
    free(pointerArray);
}

//Checks if path is a file
bool is_file(const char* path) {
    struct stat test;
    if (stat(path, &test) == 0 && S_ISDIR(test.st_mode)) {
        return false;
    }
    return true;
}

//Mallocs argument as a new string
char *make_permenent_string(char *string) {
    size_t stringLength = strlen(string) + 1;
    char *persistantString = (char *) malloc(sizeof(char) * stringLength);
    if (persistantString == NULL) {
        exit(21);
    }
    memcpy(persistantString, string, stringLength);
    return persistantString;
}

//Concatinates two strings
char* concat(const char *s1, const char *s2) {
    size_t l1 = strlen(s1), l2 = strlen(s2);
    char *result = (char *) malloc(sizeof(char) * (l1 + l2 +1));
    if (result == NULL) {
        exit(21);
    }
    memcpy(result, s1, l1);
    memcpy(result + l1, s2, l2 + 1);
    return result;
}

//Reads from file untill EOF or char end
char *read_from_file(FILE *source, int end, bool perfectSize) {
    int next;
    size_t dynamic = 4, count = 0;
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
        if (++count == dynamic) {
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

//Read everything from an fd
char *read_all_from_fd(int fd, bool perfectSize) {
    FILE *source = fdopen(fd, "r");
    if (source == NULL) {
        exit(21);
    }
    //classic annoying read here 
    int next;
    size_t dynamic = 4, count = 0;
    char *text = (char *) malloc(sizeof(char) * dynamic);
    if (text == NULL) {
        exit(21);
    }
    while(next = fgetc(source), next != EOF) {
        if (++count == dynamic) {
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
    size_t replaceLength;  // length of rep (the string to remove)
    size_t alternativeLength; // length of with (the string to replace rep with)
    size_t differenceLength; // distance between rep and end of last rep
    size_t count;    // number of replacements

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
    size_t j = 0;
    for (size_t i = 0; i < originalLength; i++) {
        char *toCompareTo = originalArray[i];
        if (toCompareTo == NULL) {
            break;
        }
        if (strcmp(toCompareTo, toRemove) != 0) {
            finalArray[j] = originalArray[i];
            j++;
        }
    }
    finalArray[j] = NULL;
    return finalArray;
}

//Parses a size_t into a string
char *size_to_string (size_t value) {
    size_t charectersRequired = snprintf(NULL, 0, "%zu", value);
    char *toReturn = (char *) malloc (sizeof(char) * (++charectersRequired));
    snprintf(toReturn, charectersRequired, "%zu", value);
    return toReturn; 
}

//Makes a command bash ready - atm just sounds with ""
char *make_bash_ready (char *toChange) {
    char *toChangeRevised = str_replace(toChange, "\"", "\\\"");
    char *start = concat("\"", toChangeRevised);
    free(toChangeRevised);
    char *final = concat(start, "\"");
    free(start);
    return final;
}

//Checks if a directory is empty
bool is_directory_empty (char *directoryPath) {
    size_t fileCount = 0;
    struct dirent *aFile;
    DIR *directory = opendir(directoryPath);
    if (directory == NULL) {
        //not a directory
        return true;
    }
    while ((aFile = readdir(directory)) != NULL) {
        if(++fileCount > 2) {
            break;
        }
    }
    closedir(directory);
    if (fileCount <= 2) {
        return true;
    }
    else {
        return false;
    }
}

//Not best code I've ever written, maybe go through again later
//free returned value ;)
char *continuous_find_and_replace(char *toRemoveFrom, char *removeStart,
        char *removeEnd, char *replaceWith) {
    char *toRemove, *tempToSave, *toRemoveTemp, *toRemoveFinal;
    char *removedVersion = make_permenent_string(toRemoveFrom);
    char *toSearchFrom = toRemoveFrom;
    while (toRemove = get_substring(toSearchFrom, removeStart, removeEnd, -1),
            toRemove != NULL) {
        toRemoveTemp = concat(removeStart, toRemove);
        free(toRemove);
        toRemoveFinal = concat(toRemoveTemp, removeEnd);
        free(toRemoveTemp);
        toSearchFrom = strstr(toSearchFrom, toRemoveFinal) + strlen(toRemoveFinal); //can't fail already found it
        tempToSave = str_replace(removedVersion, toRemoveFinal, replaceWith);  
        free(removedVersion), free(toRemoveFinal);
        removedVersion = tempToSave;
    }
    return removedVersion;
}

char *replace_leading_whitespace(char *toTrim, char *toReplace) {
    if (toReplace == NULL) {
        toReplace = "";
    }
    if (toTrim == NULL) {
        return make_permenent_string(toReplace);
    }
    char *toReturn = malloc(sizeof(char) * strlen(toTrim));
    if (toReturn == NULL) {
        exit(21);
    }
    size_t pointer = 0, returnPointer = 0, replacePointer = 0, replaceSize = strlen(toReplace);
    while (toTrim[pointer] != '\0') {
        while (toTrim[pointer] != '\0' && isspace(toTrim[pointer])) {
            pointer++;
        }
        //put replace here
        while(toTrim[pointer] != '\0' && replacePointer < replaceSize) {
            toReturn[returnPointer++] = toReplace[replacePointer++];
        }
        replacePointer = 0;
        while (toTrim[pointer] != '\n' && toTrim[pointer] != '\0') {
            toReturn[returnPointer++] = toTrim[pointer++];
        }
        toReturn[returnPointer++] = '\n';
    }
    toReturn[returnPointer] = '\0';
    return toReturn;
}

int string_comparator_wrapper(const void *a, const void *b) {
    return strcmp((char *) a, (char *) b);
}

//Returns true if two arrays are identical
bool compare_arrays(void **arr1, void **arr2, 
        int (*comparator) (const void *, const void *)) {
    size_t numberOfElements = get_pointer_array_length(arr1);
    if (numberOfElements != get_pointer_array_length(arr2)) {
        return false;
    }
    for (size_t i = 0; i < numberOfElements; i++) {
        if (comparator(arr1[i], arr2[i]) != 0) {
            return false;
        }
    }
    return true;
}

char **find_all_occurances(char *page, regex_t *compiledReg) {
    size_t nsub = 1, returnPointer = 0, dynamicArray = 4;
    regmatch_t matchLocations[nsub];
    char **toReturn = (char **) malloc(sizeof(char *) * dynamicArray);
    if (toReturn == NULL) {
        exit(21);
    }
    bool keepRunning = true;
    do {
        if (regexec(compiledReg, page, nsub, matchLocations, 0) != 0) {
            break;
        }
        for (size_t i = 0; i < nsub; i++) {
            if (matchLocations[i].rm_so == -1) {
                keepRunning = false;
                break;
            }
            size_t lengthOfKey = matchLocations[i].rm_eo - matchLocations[i].rm_so;
            char *newKey = (char *) malloc(sizeof(char) * (lengthOfKey + 1));
            if (newKey == NULL) {
                exit(21);
            }
            memcpy(newKey, page+(matchLocations[i].rm_so), lengthOfKey);
            newKey[lengthOfKey] = '\0';
            if (++returnPointer == dynamicArray) {
                dynamicArray *= 2;
                toReturn = (char **) realloc(toReturn,
                        sizeof(char *) * dynamicArray);
                if (toReturn == NULL) {
                    exit(21);
                }
            }
            toReturn[returnPointer - 1] = trim_whitespaces(newKey);
            free(newKey);
        }
        page += matchLocations[0].rm_eo;
    } while(keepRunning);
    toReturn[returnPointer] = NULL;
    return toReturn;
}


//Shout out to http://stackoverflow.com/users/9530/adam-rosenfield
//free str after this
char *trim_whitespaces(const char *str) {
    size_t len = strlen(str);
    if (len == 0) {
        return NULL;
    }
    char *out = malloc(sizeof(char) * (++len));
    if (out == NULL) {
        exit(21);
    }

    const char *end;
    size_t out_size;

    // Trim leading space
    while(isspace((unsigned char)*str)) {
        str++;
    }

    if(*str == '\0') {
        free(out);
        return NULL;
    }

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) {
        end--;
    }
    end++;

    // Set output size to minimum of trimmed string length and buffer size minus 1
    out_size = (end - str) < len-1 ? (end - str) : len-1;

    // Copy trimmed string and add null terminator
    memcpy(out, str, out_size);
    out[out_size] = '\0';

    return out;
}
