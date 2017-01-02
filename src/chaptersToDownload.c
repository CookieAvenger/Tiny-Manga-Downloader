#include "chaptersToDownload.h"

Site source;
char *folderName;
Chapter *downloadArray;

void set_folder_name(char *folder) {
    folderName = folder;
}

void set_source(Site domainUsed) {
    source = domainUsed;
}
