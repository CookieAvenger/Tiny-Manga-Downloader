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
#include <stdio.h>

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

//Kudos to http://stackoverflow.com/users/903194/david-m-syzdek
void save_url_as_file(int s, FILE *fp) {
   int       isnheader;
   ssize_t   readed;
   size_t    len;
   size_t    offset;
   size_t    pos;
   char      buffer[1024];
   char    * eol; // end of line
   char    * bol; // beginning of line

   isnheader = 0;
   len       = 0;

   // read next chunk from socket
   while((readed = read(s, &buffer[len], (1023-len))) > 0)
   {
      // write rest of data to FILE stream
      if (isnheader != 0)
         fwrite(buffer, 1, readed, fp);

      // process headers
      if (isnheader == 0)
      {
         // calculate combined length of unprocessed data and new data
         len += readed;

         // NULL terminate buffer for string functions
         buffer[len] = '\0';

         // checks if the header break happened to be the first line of the
         // buffer
         if (!(strncmp(buffer, "\r\n", 2)))
         {
            if (len > 2)
               fwrite(buffer, 1, (len-2), fp);
            continue;
         };
         if (!(strncmp(buffer, "\n", 1)))
         {
            if (len > 1)
               fwrite(buffer, 1, (len-1), fp);
            continue;
         };

         // process each line in buffer looking for header break
         bol = buffer;
         while((eol = index(bol, '\n')) != NULL)
         {
            // update bol based upon the value of eol
            bol = eol + 1; 

            // test if end of headers has been reached
            if ( (!(strncmp(bol, "\r\n", 2))) || (!(strncmp(bol, "\n", 1))) )
            {
               // note that end of headers has been reached
               isnheader = 1;

               // update the value of bol to reflect the beginning of the line
               // immediately after the headers
               if (bol[0] != '\n')
                  bol += 1;
               bol += 1;

               // calculate the amount of data remaining in the buffer
               len = len - (bol - buffer);

               // write remaining data to FILE stream
               if (len > 0)
                  fwrite(bol, 1, len, fp);

               // reset length of left over data to zero and continue processing
               // non-header information
               len = 0;
            };
         };

         if (isnheader == 0)
         { 
            // shift data remaining in buffer to beginning of buffer
            offset = (bol - buffer);
            for(pos = 0; pos < offset; pos++)
               buffer[pos] = buffer[offset + pos];

            // save amount of unprocessed data remaining in buffer
            len = offset;
         };
      };
   };
   return;
}
