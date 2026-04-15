//
// Created by mete on 15.04.2026.
//

#ifndef HTTP_SERVER_CLIENT_H
#define HTTP_SERVER_CLIENT_H

#include <sys/socket.h>
#include <netinet/in.h>

struct Client
{
    int domain;
    int port;
    int service;
    int protocol;
    u_long interface;
    int sockdf;

    void (*request)(struct Client* client,char* server_ip,char* request);

};
#endif //HTTP_SERVER_CLIENT_H
