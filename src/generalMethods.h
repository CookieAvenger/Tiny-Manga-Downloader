
char *concat(const char *s1, const char *s2);
char *read_all_from_fd(int fd);
char *make_permenent_string(char *string);
char *get_substring(char *string, char *start, char *end, int error);
char **continuous_substring(char *string, char *start, char *end);
int get_string_array_length(char **stringArray);
