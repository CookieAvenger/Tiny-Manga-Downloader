#include <curl/curl.h>

int download_file(char *url, char *fileName) {
    CURLcode toReturn;                                                            
    CURL *handle;                                                               
                                                                             
    handle = curl_easy_init();                                                  
    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L);                           
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "curl/7.47.0");                 
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 50L);                           
    curl_easy_setopt(handle, CURLOPT_TCP_KEEPALIVE, 1L);                        
    
    FILE* fileToSave = fopen(fileName, "w");
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, fileToSave) ;
                                                                             
    toReturn = curl_easy_perform(handle);                                            
                                                                             
    curl_easy_cleanup(handle);                                                  
    handle = NULL;                                                              
    
    fclose(fileToSave);
    return (int)toReturn;
}
