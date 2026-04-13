//
// Created by mete on 12.04.2026.
//

#include "Server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void launch(struct Server *server)
{

    int addrlen = sizeof(server->address);
    int new_socket;
    char *body = "<html><body><h1>Hello,World!</h1></body></html>";

    char header[1024];

    sprintf(header,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %lu\r\n"
        "Connection: close\r\n"
        "\r\n",
        strlen(body));
    /*char *hello = "HTTP/1.1 200 OK\r\nDate: Mon,27 Jul 2009 12:28:53 GMT\r\nServer: Apache/2.2.14 (Win31)\r\nLast-Modified: Wed, 22 Jul"
                         " 2009 19:15:56 GMT\r\nContent-Length: 54\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n<html><body><h1>Hello,World!"
                         "</h1></body></html>";*/

    char buffer[30000];
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);

        printf("~>~>~>~> WAITING FOR CONNECTION <~<~<~<~\n");

        new_socket = accept(server->sockdf,(struct sockaddr *) &client_addr,
            (socklen_t *) &addrlen);

        if (new_socket < 0)
        {
            perror("accept failed");
            continue;
        }
       //int bytes = read(new_socket, buffer, 29999);
       // buffer[bytes] = '\0';
        read(new_socket, buffer, 30000);

        printf("%s\n", buffer);

        write(new_socket, header, strlen(header));
        write(new_socket, body, strlen(body));
        close(new_socket);

       memset(buffer, 0, sizeof(buffer));
    }
}

int main()
{

    struct Server server = server_constructor(AF_INET,
        SOCK_STREAM, 0, INADDR_ANY, 8096, 10,launch);

    server.launch(&server);
}