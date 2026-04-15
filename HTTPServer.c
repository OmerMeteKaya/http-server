//
// Created by mete on 13.04.2026.
//

#include "HTTPServer.h"
#include "Server.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ThreadPool.h"

struct HTTPServer HTTPServer_constructor();

void launch(struct HTTPServer *server);
void http_server_shutdown(struct HTTPServer *server, struct ThreadPool *pool);
void *handler(void *args);
struct ClientServer
{
    int client;
    struct HTTPServer *server;
};

struct Route* find_route(struct HTTPServer *server, char *uri, int method)
{
    for (int i = 0; i < server->router.count; i++)
    {
        struct Route *r = &server->router.routes[i];

        if (strcmp(r->uri, uri) == 0)
        {
            for (int j = 0; j < r->method_count; j++)
            {
                if (r->methods[j] == method)
                    return r;
            }
        }
    }
    return NULL;
}

char* register_routes(struct HTTPServer *server,char* (*route_function)(struct HTTPServer *server,struct HTTPRequest *request),
    char *uri,int method_num,...)
{
    struct Route *route = &server->router.routes[server->router.count];

    va_list methods;
    va_start(methods, method_num);

    for (int i = 0; i < method_num; i++)
    {
        route->methods[i] = va_arg(methods, int);
    }

    route->method_count = method_num;
    route->uri = strdup(uri);
    route->route_function = route_function;

    server->router.count++;

    va_end(methods);
    
    return NULL;
}

struct HTTPServer HTTPServer_constructor() {

    struct HTTPServer server;
    server.server = server_constructor(AF_INET,SOCK_STREAM,0,INADDR_ANY,
        8080,255);
    server.router.count = 0;
    server.register_routes = register_routes;
    server.launch = launch;
    server.http_server_shutdown = http_server_shutdown;
    return server;
}


void launch(struct HTTPServer *server) {
    struct ThreadPool* thread_pool = thread_pool_init(10);
    struct sockaddr *sock_adress = (struct sockaddr *)&server->server.address;
    unsigned long addr_len = sizeof(server->server.address);
    while (1)
    {
        struct ClientServer *client_server = (struct ClientServer *)malloc(sizeof(struct ClientServer));
        client_server->client = accept(server->server.sockdf,sock_adress,(socklen_t*)&addr_len);
        client_server->server = server;
        struct ThreadJob threadJob = thread_job_create((void*)handler,client_server);
        thread_pool_add_job(thread_pool, threadJob);
    }
}

void* handler(void* args)
{
    struct ClientServer* client_server = (struct ClientServer *)args;

    char request_string[16000];
    int bytes = read(client_server->client, request_string, sizeof(request_string) - 1);
    
    if (bytes <= 0) {
        close(client_server->client);
        free(client_server);
        return NULL;
    }
    
    request_string[bytes] = '\0';

    struct HTTPRequest request = HTTPRequest_constructor(request_string);

    struct Route *route = find_route(client_server->server,
                                     request.URI,
                                     request.Method);

    char *response;

    if (route)
        response = route->route_function(client_server->server, &request);
    else
        response = "HTTP/1.1 404 Not Found\r\n\r\nNot Found";

    write(client_server->client, response, strlen(response));

    close(client_server->client);
    
    free_request(&request);
    free(client_server);

    return NULL;
}

void http_server_shutdown(struct HTTPServer *server, struct ThreadPool *pool)
{
    thread_pool_destroy(pool);
    free_router(&server->router);
    close(server->server.sockdf);
}
