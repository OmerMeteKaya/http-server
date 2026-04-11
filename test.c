//
// Created by mete on 12.04.2026.
//

#include "Server.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void launch(struct Server *server)
{
    int addrlen = sizeof(server->address);
    int new_socket;
    char *hello = "HTTP/1.1 200 OK\nDate: Mon,27 Jul 2009 12:28:53 GMT\nServer: Apache/2.2.14 (Win31)\nLast-Modified: Wed, 22 Jul"
                         " 2009 19:15:56 GMT\nContent-Length: 88\nContent-Type: text/html\nConnection: Closed\n\n<html><body><h1>Hello,World!"
                         "</h1></body></html>";
    char buffer[30000];
    while (1) {

        printf("~>~>~>~> WAITING FOR CONNECTION <~<~<~<~\n");

        new_socket = accept(server->socket,(struct sockaddr *) &server->address,
            (socklen_t *) &addrlen);

        read(new_socket, buffer, 30000);
        printf("%s\n", buffer);

        write(new_socket, hello, strlen(hello));
        close(new_socket);
    }
}

int main()
{

    struct Server server = server_constructor(AF_INET,
        SOCK_STREAM, 0, INADDR_ANY, 8078, 10,launch);

    server.launch(&server);
}