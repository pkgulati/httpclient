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
    
//    ret = client.get("http://google.com");
//    printf("response len %d\n", client.response_length);
//    if (client.response) {
//        printf("\nResponse[%*.*s]\n\n", client.response_length, client.response_length, client.response);
//    }
    
    ret = client.post("http://localhost:8080", "test", 4);
    printf("response len %d\n", client.response_length);
    if (client.response) {
        printf("\nResponse[%*.*s]\n\n", client.response_length, client.response_length, client.response);
    }
    
    return 0;
}
