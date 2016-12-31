#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include "tmdl.h"
#include <stdlib.h>
#include <string.h>

//FD that socket is on
int connection;

//Close connection after every read
void close_connection() {
    close(connection);
}

//Connect to the domain in the main method and save the connection there
void connect_to_domain() {
    struct addrinfo* addressInfo;
    int errorCheck = getaddrinfo(get_domain(), NULL, NULL, &addressInfo);
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
    struct sockaddr_in socketAddress;
    socketAddress.sin_family = AF_INET; // IP v4
    socketAddress.sin_port = htons(80); // Port no to connect to
    socketAddress.sin_addr.s_addr = ipAddress->s_addr;

    //Try to connect to the server
    if(connect(fd, (struct sockaddr*)&socketAddress, 
            sizeof(socketAddress)) < 0) {
        exit(22);
    }
    connection = fd;
    //Don't forget to eventually close the socket - close(fd);
}

//Send a http 1.1 request
void send_HTTP_request(char *file, char *cookie, char *userAgent) {
    connect_to_domain();
    char *requestString;
    if (cookie != NULL && userAgent != NULL) {
        //Send request with Cookie
        requestString = (char *)malloc(sizeof(char) * (strlen(file) +
                strlen(get_domain()) + strlen(cookie) +strlen(userAgent) + 
                50));
        if (requestString == NULL) {
            exit(21);
        } else {
            //Do I need a blank line at the end?
            sprintf(requestString, 
                    "GET %s HTTP/1.0\r\nHost: %s\r\nCookie: %s\r\n"
                    "User-Agent: %s\r\n\r\n", 
                    file, get_domain(), cookie, userAgent);
        }
    } else {
        //Send without cookie
        requestString = (char *)malloc(sizeof(char) * (strlen(file) +
                strlen(get_domain()) + 26));
        if (requestString == NULL) {
            exit(21);
        } else {
            sprintf(requestString, 
                    "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", 
                    file, get_domain());
        }
    }
    
    if (write(connection, requestString, strlen(requestString)) < 1) {
        exit(22);
    }
    
    free(requestString);
}

