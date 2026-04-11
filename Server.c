//
// Created by mete on 11.04.2026.
//

#include "Server.h"
#include <stdio.h>
#include <stdlib.h>

struct Server server_constructor(int domain, int service, int protocol,
    u_long interface,int port,int backlog,void (*launch)(struct Server *server))
{
    struct Server server;

    server.domain = domain;
    server.service = service;
    server.protocol = protocol;
    server.backlog = backlog;
    server.port = port;
    server.backlog = backlog;

    server.address.sin_family = domain;
    server.address.sin_port = htons(port);
    server.address.sin_addr.s_addr = htonl(interface);

    server.socket = socket(domain, service, protocol);
    if (server.socket == 0)
    {
        perror("Failed to connect socket...\n");
        exit(1);
    }

    if ((bind(server.socket, (struct sockaddr *) &server.address,
        sizeof(server.address)) < 0))
    {
        perror("Failed to bind socket...\n");
        exit(1);
    }

    if (listen(server.socket, backlog) < 0)
    {
        perror("Failed to listen on socket...\n");
        exit(1);
    }

    server.launch = launch;


    return server;
}