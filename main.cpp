//
//  main.cpp
//  http_client
//
//  Created by kpraveen on 2019-03-16.
//  Copyright © 2019 Praveen. All rights reserved.
//

#include <iostream>
#include <unistd.h>

#include "http_client.h"

int main(int argc, const char * argv[]) {
    
    HttpClient client;
   
    int ret = 0;

    ret = client.get("http://localhost:8080");
    printf("status code %d\n", client.status_code);
    if (ret == HTTP_STATUS_OK) {
        if (client.response) {
            printf("\nResponse[%*.*s]\n\n", client.response_length, client.response_length, client.response);
        }
    }
    
    return 0;
}
