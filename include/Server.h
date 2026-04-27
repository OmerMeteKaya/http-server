//
// Created by mete on 11.04.2026.
//

#ifndef Server_h
#define Server_h

#include <sys/socket.h>
#include <netinet/in.h>

struct Server {
    int domain;
    int type;
    int protocol;
    u_long interface;
    int port;
    int backlog;

    struct sockaddr_in address;

    int sockdf;

};

struct Server server_constructor(int domain, int type, int protocol,
    u_long interface, int port, int backlog);

#endif // Server_h

