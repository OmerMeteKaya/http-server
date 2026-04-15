//
// Created by mete on 12.04.2026.
//

#include "HTTPServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *simple_handler(struct HTTPServer *server, struct HTTPRequest *request)
{
    (void)server;
    (void)request;
    return "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Hello, World!</h1></body></html>";
}

int main()
{
    struct HTTPServer server = HTTPServer_constructor();
    
    server.register_routes(&server, simple_handler, "/", 1, 0);
    
    printf("Server starting on port 8080...\n");
    server.launch(&server);
    
    return 0;
}
