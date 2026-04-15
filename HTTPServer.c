//
// Created by mete on 13.04.2026.
//

#include "HTTPServer.h"
#include "Server.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

#include "ThreadPool.h"

#define BACKLOG_SIZE 10
#define RESPONSE_BUFFER_SIZE 8192

// Global flag for graceful shutdown
static volatile sig_atomic_t running = 1;

struct HTTPServer HTTPServer_constructor();

void launch(struct HTTPServer *server);
void http_server_shutdown(struct HTTPServer *server, struct ThreadPool *pool);
void *handler(void *args);

// Signal handler for graceful shutdown
static void signal_handler(int sig)
{
    (void)sig;
    printf("\n[Server] Received shutdown signal, stopping...\n");
    running = 0;
}

struct ClientServer
{
    int client;
    struct HTTPServer *server;
};

// -------------------- UTILITY FUNCTIONS --------------------

// Full write helper to ensure all bytes are written
static ssize_t write_all(int fd, const char *buf, size_t count)
{
    size_t total_written = 0;
    while (total_written < count) {
        ssize_t n = write(fd, buf + total_written, count - total_written);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        total_written += (size_t)n;
    }
    return (ssize_t)total_written;
}

// HTTP response builder
static char* build_response(int status_code, const char *status_text,
                            const char *content_type, const char *body)
{
    char *response = (char *)malloc(RESPONSE_BUFFER_SIZE);
    if (!response) return NULL;

    size_t body_len = body ? strlen(body) : 0;

    snprintf(response, RESPONSE_BUFFER_SIZE,
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             status_code, status_text,
             content_type ? content_type : "text/plain",
             body_len,
             body ? body : "");

    return response;
}

// -------------------- ROUTE FINDING --------------------

static struct Route* find_route(const struct HTTPServer *server, const char *uri, int method)
{
    for (int i = 0; i < server->router.count; i++)
    {
        const struct Route *r = &server->router.routes[i];

        if (strcmp(r->uri, uri) == 0)
        {
            for (int j = 0; j < r->method_count; j++)
            {
                if (r->methods[j] == method)
                    return (struct Route *)r;
            }
        }
    }
    return NULL;
}

// -------------------- ROUTE REGISTRATION --------------------

char* register_routes(struct HTTPServer *server, char* (*route_function)(struct HTTPServer *server, struct HTTPRequest *request),
                      char *uri, int method_num, ...)
{
    // Bounds check
    if (server->router.count >= MAX_ROUTES) {
        fprintf(stderr, "[Router] Maximum route limit reached (%d)\n", MAX_ROUTES);
        return NULL;
    }

    // URI validation
    if (!uri || uri[0] != '/') {
        fprintf(stderr, "[Router] Invalid URI: %s (must start with '/')\n", uri ? uri : "(null)");
        return NULL;
    }

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

    printf("[Router] Registered: %s (%d methods)\n", uri, method_num);
    return NULL;
}

// -------------------- CONSTRUCTOR --------------------

struct HTTPServer HTTPServer_constructor() {
    return HTTPServer_config(8080, 10);
}

struct HTTPServer HTTPServer_config(int port, int thread_count) {
    struct HTTPServer server;
    server.server = server_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, port, 255);
    server.router.count = 0;
    server.register_routes = register_routes;
    server.launch = launch;
    server.http_server_shutdown = http_server_shutdown;
    (void)thread_count; // Future use: store in server for dynamic thread pool sizing
    return server;
}

// -------------------- LAUNCH --------------------

void launch(struct HTTPServer *server) {
    struct ThreadPool* thread_pool = thread_pool_init(10);
    struct sockaddr *sock_adress = (struct sockaddr *)&server->server.address;
    unsigned long addr_len = sizeof(server->server.address);

    // Setup signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("[Server] Server running on port 8080\n");
    printf("[Server] Press Ctrl+C to stop\n");

    while (running)
    {
        int client_fd = accept(server->server.sockdf, sock_adress, (socklen_t*)&addr_len);

        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("[Server] accept failed");
            continue;
        }

        struct ClientServer *client_server = (struct ClientServer *)malloc(sizeof(struct ClientServer));
        if (!client_server) {
            close(client_fd);
            fprintf(stderr, "[Server] Failed to allocate client structure\n");
            continue;
        }

        client_server->client = client_fd;
        client_server->server = server;

        struct ThreadJob threadJob = thread_job_create(handler, client_server);
        thread_pool_add_job(thread_pool, threadJob);
    }

    printf("[Server] Stopping server...\n");
    http_server_shutdown(server, thread_pool);
    printf("[Server] Server stopped gracefully\n");
}

// -------------------- REQUEST HANDLER --------------------

void* handler(void* args)
{
    struct ClientServer* client_server = (struct ClientServer *)args;

    char request_string[MAX_REQUEST_SIZE];
    int bytes = read(client_server->client, request_string, sizeof(request_string) - 1);

    if (bytes <= 0) {
        if (bytes < 0) {
            perror("[Handler] read failed");
        }
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

    if (route) {
        char *body = route->route_function(client_server->server, &request);
        response = build_response(200, "OK", "text/html", body);
        if (body) free(body);
    } else {
        response = build_response(404, "Not Found", "text/plain", "Not Found");
    }

    if (response) {
        if (write_all(client_server->client, response, strlen(response)) < 0) {
            perror("[Handler] write failed");
        }
        free(response);
    }

    close(client_server->client);

    free_request(&request);
    free(client_server);

    return NULL;
}

// -------------------- SHUTDOWN --------------------

void http_server_shutdown(struct HTTPServer *server, struct ThreadPool *pool)
{
    thread_pool_destroy(pool);
    free_router(&server->router);
    close(server->server.sockdf);
}
