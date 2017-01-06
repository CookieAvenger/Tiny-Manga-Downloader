#include <curl/curl.h>

int download_a_file(char *url) {
    CURLcode ret;                                                            
    CURL *hnd;                                                               
                                                                             
    hnd = curl_easy_init();                                                  
    curl_easy_setopt(hnd, CURLOPT_URL, url);
    curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);                           
    curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.47.0");                 
    curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);                           
    curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);                        
                                                                             
    ret = curl_easy_perform(hnd);                                            
                                                                             
    curl_easy_cleanup(hnd);                                                  
    hnd = NULL;                                                              
    return (int)ret;
}
