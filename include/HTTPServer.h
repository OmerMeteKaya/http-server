//
// Created by mete on 13.04.2026.
//

#ifndef HTTP_SERVER_HTTPSERVER_H
#define HTTP_SERVER_HTTPSERVER_H

#include "Server.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "Route.h"
#include "ThreadPool.h"

#define DEFAULT_PORT 8080
#define DEFAULT_THREAD_COUNT 50

struct HTTPServer
{
    struct Router router;
    struct Server server;
    int thread_count;

    int (*register_route)(struct HTTPServer *server,
                          int (*route_function)(struct HTTPServer *server,
                                                struct HTTPRequest *request,
                                                struct HTTPResponse *resp),
                          const char *uri, int method_num, ...);
    void (*launch)(struct HTTPServer *server);
    void (*http_server_shutdown)(struct HTTPServer *server, struct ThreadPool *pool);
};

struct HTTPServer HTTPServer_constructor();
struct HTTPServer HTTPServer_config(int port, int thread_count);

#endif //HTTP_SERVER_HTTPSERVER_H
