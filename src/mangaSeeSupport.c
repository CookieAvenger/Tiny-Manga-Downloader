#include "networking.c"
#include "generalMethods.c"

char *get_mangasee_page(char *file);

char *handle_error_codes(char *page) {
    if (strncmp(page + 9, "3", 1) == 0) {
        char *redirectTo = get_substring(page, "Location: ", "\n", 6);
        free(page);
        page = get_mangasee_page(redirectTo);
        free(redirectTo);
        return page;
    } else if (strncmp(page + 9, "4", 1) == 0) {
        free(page);
        return NULL;
    }
    return page;
}

char *get_mangasee_page(char *file) {
    char *page = NULL;
    int fd = send_HTTP_request(get_domain(), file, NULL, NULL);
    page = read_all_from_fd(fd, false);
    page = handle_error_codes(page);
    return page;
}

void parse_and_set_series_folder(char *seriesPage) {
    
}
