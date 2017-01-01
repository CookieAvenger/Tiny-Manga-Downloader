typedef enum site { kissmanga, other } Site;

typedef struct chapter {
    char *name;
    char *link;
} Chapter;

void set_folder_name(char *folder);
void set_source(Site domainUsed);
