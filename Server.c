//
// Created by mete on 11.04.2026.
//

#include "Server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Server server_constructor(int domain, int type, int protocol,
                                 u_long interface,int port,int backlog)
{
    struct Server server;

    server.domain = domain;
    server.type = type;
    server.protocol = protocol;
    server.backlog = backlog;
    server.port = port;
    server.interface = interface;

    memset(&server.address, 0, sizeof(server.address));
    server.address.sin_family = domain;
    server.address.sin_port = htons(port);
    server.address.sin_addr.s_addr = htonl(interface);


    server.sockdf = socket(domain, type, protocol);
    if (server.sockdf < 0)
    {
        perror("Failed to connect socket...");
        exit(1);
    }

    int opt = 1;
    setsockopt(server.sockdf, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if ((bind(server.sockdf, (struct sockaddr *) &server.address,
        sizeof(server.address)) < 0))
    {
        perror("Failed to bind socket...");
        exit(1);
    }

    if (listen(server.sockdf, backlog) < 0)
    {
        perror("Failed to listen on socket...");
        exit(1);
    }





    return server;
}
