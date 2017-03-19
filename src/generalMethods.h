#include <stdio.h>
#include <stdbool.h>

bool is_file(const char* path);
char *concat (const char *s1, const char *s2);
char *read_from_file (FILE *source, int end, bool perfectSize);
char *read_all_from_fd (int fd, bool perfectSize);
char *make_permenent_string (char *string);
char *get_substring (char *string, char *start, char *end, int error);
char **continuous_substring (char *string, char *start, char *end);
size_t get_pointer_array_length(void **pointerArray);
void pointer_array_free (void **pointerArray);
void delete_folder (char *folder, int error);
void create_folder (char *folder);
char *rstrstr (char *s1, char *s2);
char *str_replace (char *original, char *replace, char *alternative);
char *size_to_string (unsigned long value);
char **remove_string_from_array (int originalLength, char **originalArray
        , char *toRemove);
char *make_bash_ready (char *toChange);
bool is_directory_empty (char *directoryPath);
void write_string_array_to_file (char *initial, char **strings,
        char *betweenArray, char *end, FILE *toWriteTo);
size_t run_html_decode_on_strings(char **strings);
char *continuous_find_and_replace(char *toRemoveFrom, char *removeStart,
        char *removeEnd, char *replaceWith);
char *replace_leading_whitespace(char *toTrim, char *toReplace);
