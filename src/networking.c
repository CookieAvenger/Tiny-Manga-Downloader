#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "tmdl.h"
#include <stdlib.h>

//Connect to the domain in the main method and save the connection there
void connect_to_domain() {
    struct addrinfo* addressInfo;
    int errorCheck = getaddrinfo(getDomain(), NULL, NULL, &addressInfo);
    if (errorCheck) {
        exit(5);
    }
    struct in_addr* ipAddress = &(((struct sockaddr_in*)(addressInfo->ai_addr))->sin_addr);
    //Create TCP socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        exit(21);
    }
    //Populate struct
    struct sockaddr_in socketAddress
    socketAddress.sin_family = AF_INET; // IP v4
    socketAddress.sin_port = htons(80); // Port no to connect to
    socketAddress.sin_addr.s_addr = ipAddress->s_addr;

    //Try to connect to the server
    if(connect(fd, (struct sockaddr*)&socketAddr, sizeof(socketAddr)) < 0) {
        exit(22);
    }
    setConnection(fd);
    //Don't forget to eventually close the socket - close(fd);
}

void send_HTTP_request(char* file, char* cookie) {
    char *requestString;
    if (cookie != NULL) {
        //Send request with Cookie
        requestString = (char *)malloc();
        if (requestString == NULL) {
            exit(21);
        } else {
            //Do I need a blank line at the end?
            sprintf(requestString, 
                "GET %s HTTP/1.0\r\nHost: %s\r\nCookie: %s\r\n\r\n", 
                file, cookie);
        }
    } else {
        //Send without cookie
        requestString = (char *)malloc();
        if (requestString == NULL) {
            exit(21);
        } else {
            sprintf(requestString, 
                "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", 
                file);
        }
    }
    
    if (write(get_connection(), requestString, strlen(requestString)) < 1) {
        exit(22);
    }
    
    free(requestString);
}
