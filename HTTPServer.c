//
// Created by mete on 13.04.2026.
//

#include "HTTPServer.h"
#include "Server.h"
#include "Logger.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ThreadPool.h"

#define BACKLOG_SIZE 512
#define READ_BUFFER_SIZE 4096

// Global flag for graceful shutdown
static volatile sig_atomic_t running = 1;

struct HTTPServer HTTPServer_constructor();
struct HTTPServer HTTPServer_config(int port, int thread_count);

void launch(struct HTTPServer *server);
void http_server_shutdown(struct HTTPServer *server, struct ThreadPool *pool);
void *handler(void *args);

// Signal handler for graceful shutdown
static void signal_handler(int sig)
{
    (void)sig;
    LOG_INFO("Received shutdown signal (signal=%d), stopping...", sig);
    running = 0;
}

struct ClientServer
{
    int client_fd;
    struct HTTPServer *server;
    char client_ip[64];
    int client_port;
};

// -------------------- UTILITY: FULL WRITE --------------------

static ssize_t write_all(int fd, const char *buf, size_t count)
{
    size_t total_written = 0;
    while (total_written < count) {
        ssize_t n = write(fd, buf + total_written, count - total_written);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) break; // Connection closed
        total_written += (size_t)n;
    }
    return (ssize_t)total_written;
}

// -------------------- UTILITY: READ FULL REQUEST --------------------

// Reads from socket until we have the full HTTP headers (ending with \r\n\r\n).
// Then reads the body based on Content-Length if present.
// Returns total bytes read, or -1 on error.
static int read_http_request(int client_fd, char *buffer, size_t buffer_size)
{
    size_t total_read = 0;
    int headers_found = 0;

    // Phase 1: Read until we find \r\n\r\n (end of headers)
    while (!headers_found) {
        if (total_read >= buffer_size - 1) {
            LOG_WARN("Request too large (>%zu bytes), truncating", buffer_size);
            break;
        }

        ssize_t n = read(client_fd, buffer + total_read,
                         buffer_size - total_read - 1);
        if (n < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("read() failed on client: %s", strerror(errno));
            return -1;
        }
        if (n == 0) {
            // Connection closed by client
            if (total_read == 0) return 0;
            break;
        }

        total_read += (size_t)n;
        buffer[total_read] = '\0';

        // Check for end of headers
        if (strstr(buffer, "\r\n\r\n") != NULL) {
            headers_found = 1;
        }
    }

    // Phase 2: If Content-Length is present, read the body
    // Look for Content-Length header
    const char *cl_pos = strstr(buffer, "Content-Length: ");
    if (cl_pos && headers_found) {
        long content_length = strtol(cl_pos + 16, NULL, 10);
        if (content_length > 0) {
            // Find where the body starts (after \r\n\r\n)
            char *body_start = strstr(buffer, "\r\n\r\n");
            if (body_start) {
                size_t headers_len = (size_t)(body_start - buffer) + 4;
                size_t body_already_read = total_read - headers_len;

                if ((long)body_already_read < content_length) {
                    size_t body_remaining = (size_t)content_length - body_already_read;

                    // Read remaining body bytes
                    while (body_remaining > 0 && total_read < buffer_size - 1) {
                        ssize_t n = read(client_fd, buffer + total_read,
                                         body_remaining < (buffer_size - total_read - 1)
                                             ? body_remaining
                                             : (buffer_size - total_read - 1));
                        if (n < 0) {
                            if (errno == EINTR) continue;
                            LOG_ERROR("read() failed reading body: %s", strerror(errno));
                            break;
                        }
                        if (n == 0) break;

                        total_read += (size_t)n;
                        body_remaining -= (size_t)n;
                    }
                    buffer[total_read] = '\0';
                }
            }
        }
    }

    return (int)total_read;
}

// -------------------- ROUTE FINDING --------------------

static struct Route* find_route(const struct HTTPServer *server, const char *uri,
                                 int method, int *method_not_allowed,
                                 int *allowed_methods)
{
    *method_not_allowed = 0;
    *allowed_methods = 0;

    for (int i = 0; i < server->router.count; i++)
    {
        const struct Route *r = &server->router.routes[i];

        if (strcmp(r->uri, uri) == 0) {
            // URI matches — check methods
            for (int j = 0; j < r->method_count; j++) {
                if (r->methods[j] == method) {
                    return (struct Route *)r; // Exact match
                }
                *allowed_methods |= (1 << r->methods[j]);
            }
            // URI matched but method didn't
            *method_not_allowed = 1;
            return NULL;
        }
    }
    return NULL;
}

// -------------------- ROUTE REGISTRATION --------------------

static int register_route_impl(struct HTTPServer *server,
                               int (*route_function)(struct HTTPServer *server,
                                                     struct HTTPRequest *request,
                                                     struct HTTPResponse *resp),
                               const char *uri, int method_num, ...)
{
    if (!server || !route_function) {
        LOG_ERROR("register_route: NULL server or route_function");
        return -1;
    }

    // Bounds check
    if (server->router.count >= MAX_ROUTES) {
        LOG_ERROR("Maximum route limit reached (%d)", MAX_ROUTES);
        return -1;
    }

    // URI validation
    if (!uri || uri[0] != '/') {
        LOG_ERROR("Invalid URI: %s (must start with '/')", uri ? uri : "(null)");
        return -1;
    }

    struct Route *route = &server->router.routes[server->router.count];

    va_list methods;
    va_start(methods, method_num);

    for (int i = 0; i < method_num; i++) {
        route->methods[i] = va_arg(methods, int);
    }

    route->method_count = method_num;
    route->uri = strdup(uri);
    if (!route->uri) {
        LOG_ERROR("strdup failed for route URI");
        va_end(methods);
        return -1;
    }
    route->route_function = route_function;

    server->router.count++;
    va_end(methods);

    LOG_INFO("Route registered: %s (%d methods)", uri, method_num);
    return 0;
}

// -------------------- CONSTRUCTOR --------------------

struct HTTPServer HTTPServer_constructor()
{
    return HTTPServer_config(DEFAULT_PORT, DEFAULT_THREAD_COUNT);
}

struct HTTPServer HTTPServer_config(int port, int thread_count)
{
    struct HTTPServer server;
    memset(&server, 0, sizeof(server));

    server.server = server_constructor(AF_INET, SOCK_STREAM, 0,
                                       INADDR_ANY, port, BACKLOG_SIZE);
    if (server.server.sockdf < 0) {
        LOG_ERROR("Failed to create server socket");
        return server;
    }

    server.router.count = 0;
    server.thread_count = thread_count > 0 ? thread_count : DEFAULT_THREAD_COUNT;
    server.register_route = register_route_impl;
    server.launch = launch;
    server.http_server_shutdown = http_server_shutdown;

    LOG_INFO("Server configured: port=%d, threads=%d", port, server.thread_count);
    return server;
}

// -------------------- LAUNCH --------------------

void launch(struct HTTPServer *server)
{
    if (!server || server->server.sockdf < 0) {
        LOG_ERROR("launch() called with invalid server");
        return;
    }

    struct ThreadPool* thread_pool = thread_pool_init(server->thread_count);
    if (!thread_pool) {
        LOG_ERROR("Failed to initialize thread pool");
        return;
    }

    // Setup signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    LOG_INFO("Server running on port %d (threads=%d)",
             server->server.port, server->thread_count);
    LOG_INFO("Press Ctrl+C to stop");

    while (running)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept(server->server.sockdf,
                               (struct sockaddr *)&client_addr, &addr_len);

        if (client_fd < 0) {
            if (errno == EINTR) {
                LOG_DEBUG("accept() interrupted by signal");
                continue;
            }
            LOG_ERROR("accept() failed: %s", strerror(errno));
            continue;
        }

        struct ClientServer *client_server =
            (struct ClientServer *)calloc(1, sizeof(struct ClientServer));
        if (!client_server) {
            LOG_ERROR("Failed to allocate ClientServer: %s", strerror(errno));
            close(client_fd);
            continue;
        }

        client_server->client_fd = client_fd;
        client_server->server = server;

        // Extract client IP and port
        inet_ntop(AF_INET, &client_addr.sin_addr,
                  client_server->client_ip, sizeof(client_server->client_ip));
        client_server->client_port = ntohs(client_addr.sin_port);

        struct ThreadJob threadJob = thread_job_create(handler, client_server);
        thread_pool_add_job(thread_pool, threadJob);
    }

    LOG_INFO("Stopping server...");
    http_server_shutdown(server, thread_pool);
    LOG_INFO("Server stopped gracefully");
}

// -------------------- REQUEST HANDLER --------------------

void* handler(void* args)
{
    struct ClientServer* client_server = (struct ClientServer *)args;
    if (!client_server || !client_server->server) {
        LOG_ERROR("handler() called with NULL client_server");
        return NULL;
    }

    char request_buffer[MAX_REQUEST_SIZE];
    int bytes = read_http_request(client_server->client_fd, request_buffer,
                                  sizeof(request_buffer));

    if (bytes <= 0) {
        if (bytes < 0) {
            LOG_ERROR("[%s:%d] read failed: %s",
                      client_server->client_ip, client_server->client_port,
                      strerror(errno));
        }
        close(client_server->client_fd);
        free(client_server);
        return NULL;
    }

    LOG_INFO("[%s:%d] Received %d bytes",
             client_server->client_ip, client_server->client_port, bytes);

    // Parse the request
    struct HTTPRequest request;
    int parse_result = HTTPRequest_constructor(&request, request_buffer);

    if (parse_result != 0 || request.Method == HTTP_INVALID) {
        // Malformed request → 400 Bad Request
        char *response = response_make(400, "Bad Request", "text/plain",
                                       "400 Bad Request");
        if (response) {
            write_all(client_server->client_fd, response, strlen(response));
            free(response);
        }
        LOG_WARN("[%s:%d] Malformed request → 400",
                 client_server->client_ip, client_server->client_port);
        close(client_server->client_fd);
        free(client_server);
        return NULL;
    }

    // Copy client info into the request
    snprintf(request.client_ip, sizeof(request.client_ip), "%s",
             client_server->client_ip);
    request.client_port = client_server->client_port;

    // Find matching route
    int method_not_allowed = 0;
    int allowed_methods = 0;

    struct Route *route = find_route(client_server->server, request.URI,
                                     request.Method, &method_not_allowed,
                                     &allowed_methods);

    struct HTTPResponse resp;
    response_init(&resp);

    if (route) {
        // Route found — call the handler
        LOG_INFO("[%s:%d] %s %s → route matched",
                 client_server->client_ip, client_server->client_port,
                 method_to_str(request.Method), request.URI);

        int handler_result = route->route_function(client_server->server,
                                                    &request, &resp);
        if (handler_result != 0) {
            // Handler indicated an error
            response_destroy(&resp);
            response_init(&resp);
            response_status(&resp, 500, "Internal Server Error");
            response_body(&resp, strdup("500 Internal Server Error"),
                          strlen("500 Internal Server Error"));
        }
    } else if (method_not_allowed) {
        // URI matched but method didn't → 405 Method Not Allowed
        LOG_WARN("[%s:%d] %s %s → 405 Method Not Allowed",
                 client_server->client_ip, client_server->client_port,
                 method_to_str(request.Method), request.URI);

        response_status(&resp, 405, "Method Not Allowed");

        // Build Allow header
        char allow_methods[256] = "";
        const char *method_names[] = {
            "GET", "POST", "PUT", "HEAD", "PATCH", "DELETE",
            "CONNECT", "OPTIONS", "TRACE"
        };
        int first = 1;
        for (int i = 0; i < 9; i++) {
            if (allowed_methods & (1 << i)) {
                if (!first) strncat(allow_methods, ", ", sizeof(allow_methods) - strlen(allow_methods) - 1);
                strncat(allow_methods, method_names[i], sizeof(allow_methods) - strlen(allow_methods) - 1);
                first = 0;
            }
        }
        response_header(&resp, "Allow", allow_methods);
        response_body(&resp, strdup("405 Method Not Allowed"),
                      strlen("405 Method Not Allowed"));
    } else {
        // No route found → 404
        LOG_INFO("[%s:%d] %s %s → 404 Not Found",
                 client_server->client_ip, client_server->client_port,
                 method_to_str(request.Method), request.URI);

        response_status(&resp, 404, "Not Found");
        response_body(&resp, strdup("404 Not Found"), strlen("404 Not Found"));
    }

    // Handle HEAD method: serialize but don't include body
    char *response = response_serialize(&resp);
    if (!response) {
        LOG_ERROR("Failed to serialize response");
        close(client_server->client_fd);
        free_request(&request);
        free(client_server);
        return NULL;
    }

    if (request.Method == HEAD) {
        // For HEAD, send headers only (strip the body part after \r\n\r\n)
        char *body_sep = strstr(response, "\r\n\r\n");
        if (body_sep) {
            *(body_sep + 4) = '\0';
            size_t headers_len = strlen(response);
            if (write_all(client_server->client_fd, response, headers_len) < 0) {
                LOG_ERROR("[%s:%d] write failed: %s",
                          client_server->client_ip, client_server->client_port,
                          strerror(errno));
            }
        }
    } else {
        if (write_all(client_server->client_fd, response, strlen(response)) < 0) {
            LOG_ERROR("[%s:%d] write failed: %s",
                      client_server->client_ip, client_server->client_port,
                      strerror(errno));
        }
    }

    free(response);
    response_destroy(&resp);
    free_request(&request);

    // Graceful TCP shutdown: signal we're done writing, then close
    shutdown(client_server->client_fd, SHUT_WR);
    close(client_server->client_fd);
    free(client_server);

    return NULL;
}

// -------------------- SHUTDOWN --------------------

void http_server_shutdown(struct HTTPServer *server, struct ThreadPool *pool)
{
    if (!server || !pool) {
        LOG_ERROR("shutdown called with NULL server or pool");
        return;
    }

    LOG_INFO("Shutting down thread pool...");
    thread_pool_destroy(pool);

    LOG_INFO("Freeing routes...");
    free_router(&server->router);

    LOG_INFO("Closing listening socket...");
    close(server->server.sockdf);
    server->server.sockdf = -1;

    LOG_INFO("Shutdown complete");
}
