#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include "tmdl.h"
#include <stdlib.h>
#include <string.h>
#include "generalMethods.h"
#include <curl/curl.h>

//Connect to the domain in the main method and save the connection there
int connect_to_domain(char *domain) {
    struct addrinfo *addressInformation;
    int errorCheck = getaddrinfo(domain, "80", NULL, &addressInformation);
    if (errorCheck) {
        exit(5);
    }
    struct in_addr* ipAddress = &(((struct sockaddr_in*)
            (addressInformation->ai_addr))->sin_addr);
    //Create TCP socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        exit(21);
    }
    //Populate struct
    struct sockaddr_in socketAddress;
    socketAddress.sin_family = AF_INET; // IP v4
    socketAddress.sin_port = htons(80); // Port no to connect to
    socketAddress.sin_addr.s_addr = ipAddress->s_addr;

    //Try to connect to the server
    if(connect(fd, (struct sockaddr*)&socketAddress, 
            sizeof(socketAddress)) < 0) {
        exit(22);
    }
    freeaddrinfo(addressInformation);
    return fd;
    //Don't forget to eventually close the socket - close(fd);
}

//Send a http 1.1 request
int send_HTTP_request(char *domain, char *file, char *cookie, char *userAgent) {
    int fd = connect_to_domain(domain);
    char *requestString;
    if (cookie != NULL && userAgent != NULL) {
        //Send request with Cookie
        requestString = (char *) malloc(sizeof(char) * (strlen(file) +
                strlen(domain) + strlen(cookie) +strlen(userAgent) + 
                50));
        if (requestString == NULL) {
            exit(21);
        }
        //Do I need a blank line at the end?
        sprintf(requestString, 
                "GET %s HTTP/1.0\r\nHost: %s\r\nCookie: %s\r\n"
                "User-Agent: %s\r\n\r\n", 
                file, domain, cookie, userAgent);
        
    } else {
        //Send without cookie
        requestString = (char *) malloc(sizeof(char) * (strlen(file) +
                strlen(domain) + 26));
        if (requestString == NULL) {
            exit(21);
        } 
        sprintf(requestString, 
                "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", 
                file, domain);
        
    }
    
    if (write(fd, requestString, strlen(requestString)) < 1) {
        exit(22);
    }
    
    free(requestString);
    return fd;
}

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
