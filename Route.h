//
// Created by mete on 14.04.2026.
//

#ifndef HTTP_SERVER_ROUTER_H
#define HTTP_SERVER_ROUTER_H

struct Router
{
    int methods[9];
    char *uri;
    char *(*route_function)(struct HTTPServer *server, struct HTTPRequest *request);
};
#endif //HTTP_SERVER_ROUTER_H
