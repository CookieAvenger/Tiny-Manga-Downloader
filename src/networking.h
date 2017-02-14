#include <stdio.h>

int send_HTTP_request(char *domain, char *file, char *cookie, char *userAgent);
int download_file(char *url, char *fileName);
void save_url_as_file(int s, FILE *fp);
