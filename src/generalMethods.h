#include <stdio.h>
#include <stdbool.h>

void enter_critical_code();
void enter_critical_code();
bool is_file(const char* name);
char *concat (const char *s1, const char *s2);
char *read_from_file (FILE *source, int end, bool perfectSize);
char *read_all_from_fd (int fd, bool perfectSize);
char *make_permenent_string (char *string);
char *get_substring (char *string, char *start, char *end, int error);
char **continuous_substring (char *string, char *start, char *end);
int get_string_array_length (char **stringArray);
void string_array_free (char **stringArray);
void delete_file (char *path);
void delete_folder (char *folder, int error);
void create_folder (char *folder);
char *rstrstr (char *s1, char *s2);
char *str_replace (char *original, char *replace, char *alternative);
char **remove_string_from_array (int originalLength, char **originalArray
        , char *toRemove);
