//
// Created by mete on 14.04.2026.
//

#ifndef HTTP_SERVER_ROUTER_H
#define HTTP_SERVER_ROUTER_H

#define MAX_ROUTES 256

// Forward declarations
struct HTTPServer;
struct HTTPRequest;
struct HTTPResponse;

struct Route
{
    int methods[9];
    int method_count;
    char *uri;
    // Route handlers return 0 on success, -1 on error.
    // The handler populates 'resp' with status, headers, and body.
    int (*route_function)(struct HTTPServer *server,
                          struct HTTPRequest *request,
                          struct HTTPResponse *resp);
};

struct Router
{
    struct Route routes[MAX_ROUTES];
    int count;
};

void free_router(struct Router *router);

#endif //HTTP_SERVER_ROUTER_H
