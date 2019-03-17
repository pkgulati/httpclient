//
//  http_client.cpp
//  http_client
//
//  Created by kpraveen on 2019-03-16.
//  Copyright © 2019 Praveen. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>

#include "http_client.h"

struct request_data {
    char* path;
    char* body;
    size_t bodylen;
    int completed;
    char* header;
    size_t headerlen;
};

static int socket_connect(char *host, in_port_t port){
    struct hostent *hp;
    struct sockaddr_in addr;
    int on = 1, sock;
    
    if((hp = gethostbyname(host)) == NULL){
        printf("gethostbyname failed\n");
        exit(1);
    }
    bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    //setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));
    
    if(sock == -1){
        perror("setsockopt");
        exit(1);
    }
    
    printf("connecting...\n");
    if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
        perror("connect");
        printf("could not connect\n");
        sock = -1;
    }
    return sock;
}

static int on_message_begin(http_parser* parser) {
    return 0;
}

static int on_headers_complete(http_parser* parser) {
    return 0;
}

static int on_message_complete(http_parser* parser) {
    // printf("on_message_complete set data.completed true\n");
    request_data* data = (request_data*)(parser->data);
    data->completed = true;
    return 0;
}

static int on_url(http_parser* parser, const char* at, size_t length) {
    printf("on url\n");
    return 0;
}

// strcasecmp POSIX.1-2001 and 4.4BSD
static int on_header_field(http_parser* parser, const char* at, size_t length) {
    request_data* data = (request_data*)(parser->data);
    data->header = const_cast<char*>(at);
    data->headerlen = length;
    return 0;
}

static int on_header_value(http_parser* parser, const char* at, size_t length) {
    //printf("on_header_value %.*s\n", length,at);
    if (strncasecmp(at, "content-type", 12) == 0) {
        printf("header 1 %.*s\n", length, at);
    }
    return 0;
}

static int on_body(http_parser* parser, const char* at, size_t length) {
    request_data* data = (request_data*)(parser->data);
    if (!(data->body)) {
        data->body = const_cast<char*>(at);
    }
    data->bodylen += length;
    return 0;
}


static int setSocketNonBlocking(int fd) {
    int     lFlags;
    lFlags = fcntl(fd, F_GETFL, 0);
    if (lFlags < 0)
    {
        return -1;
    }
    lFlags |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, lFlags) == 0)
    {
        return -1;
    }
    return 0;
}

static int setSocketBlocking(int fd) {
    int     lFlags;
    lFlags = fcntl(fd, F_GETFL, 0);
    if (lFlags < 0)
    {
        return -1;
    }
    lFlags &= ~O_NONBLOCK;
    if(fcntl(fd, F_SETFL, lFlags) == 0)
    {
        return -1;
    }
    return 0;
}


int HttpClient::connect() {
    fd = socket_connect(hostname, portnum);
    return fd;
}

int
HttpClient::_send_receive(char* url) {

    struct http_parser_url u;
    
    printf("url  %s\n", url);
    
    char path[300] = "/";
    
    int has_protocol = 0;
    if (strncmp(url, "http", 4) == 0) {
        has_protocol = 0;
    };
    
    http_parser_url_init(&u);
    int result = http_parser_parse_url(url, strlen(url), has_protocol, &u);
    printf("url result %d\n", result);
    
    if ((u.field_set & (1 << UF_HOST))) {
        char temp[256];
        if (u.field_data[UF_PORT].len > 255) {
            printf("invalid host");
            return -1;
        }
        strncpy(temp, url+u.field_data[UF_HOST].off, u.field_data[UF_HOST].len);
        temp[u.field_data[UF_HOST].len] = NULL;
        if (fd >= 0) {
            if (strcmp(hostname, temp) != 0) {
                shutdown(fd, SHUT_RDWR);
                portnum = 80;
                fd = -1;
                strcpy(hostname, temp);
            }
        }
        else {
            strcpy(hostname, temp);
        }
    }
    
    if (hostname[0] == 0) {
        printf("host not set\n");
        return -1;
    }
    
    if ((u.field_set & (1 << UF_PORT))) {
        char temp[6];
        if (u.field_data[UF_PORT].len > 5) {
            printf("invalid port");
            return -1;
        }
        strncpy(temp, url+u.field_data[UF_PORT].off, u.field_data[UF_PORT].len);
        int newport = atoi(temp);
        if (fd >= 0) {
            if (newport != portnum) {
                 shutdown(fd, SHUT_RDWR);
            }
        }
        portnum = newport;
        printf("port %d\n", portnum);
    }
    
    if ((u.field_set & (1 << UF_PATH))) {
        printf("path set\n");
        strncpy(path, url+u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);
        path[u.field_data[UF_PATH].len] = NULL;
    }
    
    int retry_done = 0;
    char req[2056];
    
    if (method == HTTP_GET) {
        if (request_headers) {
            sprintf(req, "GET %s HTTP/1.1\r\nHost: %s \r\n%s\r\n\r\n", path, hostname, request_headers);
        }
        else {
            sprintf(req, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", path, hostname);
        }
    } else if (method == HTTP_POST) {
        if (request_headers) {
            sprintf(req, "POST %s HTTP/1.1\r\nHost: %s \r\n%s\r\n\r\n", path, hostname, request_headers);
        }
        else {
            sprintf(req, "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Length: %d\r\n\r\n", path, hostname, request_length);
        }
    } else {
        printf("inalid method\n");
        return -1;
    }
    
RETRY_REQUEST:
    bufread = 0;
    parsepos = bufstart;
    response = NULL;
    response_length = 0;
    
    if (fd == -1) {
        connect();
        setSocketBlocking(fd);
    }
    else {
        setSocketNonBlocking(fd);
        char temp;
        int peeked = recv(fd, &temp, 1, MSG_PEEK);
        if (peeked == 0) {
            fd = -1;
        }
        else if (peeked < 0) {
            int lasterror = errno;
            if (lasterror == EWOULDBLOCK) {
                printf("would block means socket is alive\n");
            }
        }
        else {
            printf("data present\n");
        }
        if (fd == -1) {
            connect();
        } else {
            printf("socket is alive\n");
            setSocketBlocking(fd);
        }
    }
    
    if (fd < 0) {
        return -1;
    }
    
    struct request_data data;
    data.path = path;
    data.completed = false;
    data.bodylen = 0;
    data.body = NULL;
    // data.parser = this;
    
    parser.data = &data;
    int sb = 0;
    if (method == HTTP_GET) {
         sb = send(fd, req, strlen(req), 0);
    } else {
        int flags = 0;
#ifdef __linux__
        printf("linux\n");
        flags = flags & MSG_MORE;
#endif
        sb = send(fd, req, strlen(req), flags);
        if (sb > 0) {
            flags = 0;
            printf("sending body %.*s\n", request_length, request_body);
            sb = send(fd, request_body, request_length, flags);
        }
    }
    
    printf("sent bytes %d\n", sb);
    if (sb <= 0) {
        printf("Could not send\n");
        return sb;
    }
  
    int totalparsed = 0;
    while(1) {
        
        if (capacity-bufread < 1024) {
            bufstart = (char*)realloc(bufstart, capacity+4096);
            capacity += 4096;
        };
        
        printf("wait for response\n");
        int recived_now = recv(fd, bufstart+bufread, capacity-bufread, 0);
        if (recived_now < 0) {
            printf("Error in receive %d %d\n", recived_now, errno);
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            return recived_now;
        }
        
        if (recived_now == 0) {
            printf("connection closed? %d\n", errno);
            if (!retry_done) {
                retry_done = 1;
                printf("reconnect\n");
                connect();
                goto RETRY_REQUEST;
            }
            return recived_now;
        }
        if (recived_now <= 0) {
            printf("nothig to recieve\n");
            return recived_now;
        }
        bufread += recived_now;
        printf("total read %d\n", bufread);
        printf("received buffer [%*.*s]\n", bufread, bufread, bufstart);
        
        char* headerend = strstr(bufstart, "\r\n\r\n");
        if (!headerend) {
            printf("wait more\n");
            continue;
        }
        
        int toparse = bufread - totalparsed;
        printf("parse %d bytes\n", toparse);
        int nparsed = http_parser_execute(&parser, &settings, parsepos, toparse);
        if (nparsed != toparse) {
            printf("parsed partial");
            return -1;
        }
        
        parsepos = parsepos + toparse;
        totalparsed = totalparsed + toparse;
        if (data.completed) {
            response = data.body;
            response_length = data.bodylen;
            break;
        }
    }
    return HTTP_STATUS_OK;
}

int
HttpClient::get(char* url) {
    method = HTTP_GET;
    return _send_receive(url);
}

int
HttpClient::post(char* url, char* body, int len) {
    method = HTTP_POST;
    request_body = body;
    request_length = len;
    return _send_receive(url);
}

HttpClient::HttpClient() {
    portnum = 80;
    fd = -1;
    memset(&settings, 0, sizeof(settings));
    settings.on_message_begin = on_message_begin;
    settings.on_url = on_url;
    settings.on_header_field = on_header_field;
    settings.on_header_value = on_header_value;
    settings.on_headers_complete = on_headers_complete;
    settings.on_body = on_body;
    settings.on_message_complete = on_message_complete;
    parser.data = NULL;
    http_parser_init(&parser, HTTP_RESPONSE);
    capacity = 4096;
    bufread = 0;
    bufstart = (char*)malloc(4096);
    response = NULL;
    response_length = 0;
    strcpy(hostname, "");
    portnum = 80;
}



