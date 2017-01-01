#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//Get substring of string inbetween a start and end
char *get_substring(char *string, char *start, char *end, int error) {
    char *startOfSubstring = strstr(string, start);
    if (startOfSubstring == NULL) {
        exit(error);
    }
    char *endOfSubstring = strstr((startOfSubstring + strlen(start)), end);
    if (endOfSubstring == NULL) {
        exit(error);
    }
    int charectersInSubstring = (strlen(startOfSubstring) - strlen(start)) -
            strlen(endOfSubstring);
    char *substring = (char *) malloc(sizeof(char) *
            (charectersInSubstring + 1));
    if (substring == NULL) {
        exit(21);
    }
    strncpy(substring, (startOfSubstring + strlen(start)), 
            charectersInSubstring);
    substring[charectersInSubstring] = '\0';
    return substring;
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
    //classic annoying read here 
    char *text = (char *) malloc(sizeof(char) * 4);
    int next, dynamic = 4, count = 0;
    while(count++, next = fgetc(source), next != EOF) {
        if (count == dynamic) {
            dynamic *= 2;
            text = (char *) realloc(text, dynamic);
        }
        text[count - 1] = (char) next;
    }
    text[count] = '\0';
    fclose(source);
    return text;
}
