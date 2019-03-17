//
//  http_client.h
//  http_client
//
//  Created by kpraveen on 2019-03-16.
//  Copyright © 2019 Praveen. All rights reserved.
//

#ifndef http_client_h
#define http_client_h

#include "http_parser.h"


// This client does not support pipelining of multiple requests....
class HttpClient {
    
    public:
    
    HttpClient();
    
    int get(char* url);
    int post(char* url, char* body, int len);
    int close();
    
    char*   response_header;
    int     response_header_length;
    char*   response;
    int     response_length;
    int     message_completed;
    
    private :
        int connect();
        int _send_receive(char* url);
        int fd;
        char*   request_headers;
        char* request_body;
        int request_length;
        int keepalive;
        int method;
        char hostname[256];
        int portnum;
        http_parser_settings settings;
        http_parser parser;
        char*   bufstart;
        size_t  bufread;
        char* parsepos;
        size_t  capacity;
};

#endif /* http_client_h */
