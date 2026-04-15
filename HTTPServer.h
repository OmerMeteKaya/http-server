//
// Created by mete on 13.04.2026.
//

#ifndef HTTP_SERVER_HTTPSERVER_H
#define HTTP_SERVER_HTTPSERVER_H

#include "Server.h"
#include "HTTPRequest.h"
#define MAX_ROUTES 100
struct Route
{
    int methods[9];
    int method_count;
    char *uri;
    char *(*route_function)(struct HTTPServer *server, struct HTTPRequest *request);
};

struct Router
{
    struct Route routes[MAX_ROUTES];
    int count;
};
struct HTTPServer
{
    struct Server server;

    void (*register_routes)(struct HTTPServer *server,void (*route_function)(struct HTTPServer *server,struct HTTPRequest *request),
        char *uri,int method_num,...);
};

struct HTTPServer HTTPServer_constructor();
#endif //HTTP_SERVER_HTTPSERVER_H
