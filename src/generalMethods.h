#include <stdio.h>

char *concat(const char *s1, const char *s2);
char *read_from_file(FILE *source, int end);
char *read_all_from_fd(int fd);
char *make_permenent_string(char *string);
char *get_substring(char *string, char *start, char *end, int error);
char **continuous_substring(char *string, char *start, char *end);
int get_string_array_length(char **stringArray);
void string_array_free(char **stringArray);
void delete_folder(char *folder, int error);
void create_folder(char *folder);
char *rstrstr(char *s1, char *s2);
char *str_replace(char *original, char *replace, char *alternative);
char **remove_string_from_array(int originalLength, char **originalArray
        , char *toRemove);
