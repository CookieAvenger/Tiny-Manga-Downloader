typedef enum site { kissmanga, other } Site;

typedef struct chapter {
    char *name;
    char *link;
} Chapter;

void set_series_folder(char *folder);
void set_source(Site domainUsed);
int get_download_length();
int get_current_downloadi_number();
void free_download_array();
void add_to_download_list(Chapter *toAdd);
Chapter *pop_from_download();
