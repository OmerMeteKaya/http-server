//
// Created by mete on 12.04.2026.
//

#include "HTTPServer.h"
#include "Logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// HTTP SERVER - TEST APPLICATION
//
// Demonstrates a complete HTTP server with multiple routes,
// JSON responses, POST handling, and error handling.
//
// Run: ./cmake-build-debug/http_server
// Test: curl http://localhost:8080/
// ============================================================================

// -------------------- JSON STRING ESCAPE HELPER --------------------

// Escapes special characters for safe JSON string embedding
static void json_escape(const char *src, char *dst, size_t dst_size)
{
    size_t di = 0;
    size_t si = 0;
    size_t src_len = strlen(src);

    while (si < src_len && di < dst_size - 2) {
        char c = src[si];
        switch (c) {
            case '"':  dst[di++] = '\\'; dst[di++] = '"'; break;
            case '\\': dst[di++] = '\\'; dst[di++] = '\\'; break;
            case '\n': dst[di++] = '\\'; dst[di++] = 'n'; break;
            case '\r': dst[di++] = '\\'; dst[di++] = 'r'; break;
            case '\t': dst[di++] = '\\'; dst[di++] = 't'; break;
            default:   dst[di++] = c; break;
        }
        si++;
    }
    dst[di] = '\0';
}


// -------------------- ROUTE HANDLERS --------------------
// Signature: int handler(HTTPServer*, HTTPRequest*, HTTPResponse*)
// Return 0 on success, -1 on error.
// Populate resp->body, resp.status_code, etc.

// GET /
int handle_home(struct HTTPServer *server, struct HTTPRequest *request,
                struct HTTPResponse *resp)
{
    (void)server;
    (void)request;

    const char *body =
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>HTTP Server Test</title></head>"
        "<body>"
        "<h1>Welcome to the HTTP Server!</h1>"
        "<ul>"
        "  <li><a href=\"/api/status\">GET /api/status</a> - Server status</li>"
        "  <li><a href=\"/api/hello\">GET /api/hello</a> - Hello JSON</li>"
        "  <li><a href=\"/api/about\">GET /api/about</a> - About page</li>"
        "  <li>POST /api/echo - Echo POST data</li>"
        "  <li>GET /api/error - Test error handling</li>"
        "  <li>GET /api/headers - Dump request headers</li>"
        "</ul>"
        "</body>"
        "</html>";

    response_header(resp, "Content-Type", "text/html");
    response_body(resp, strdup(body), strlen(body));
    return 0;
}

// GET /api/status
int handle_status(struct HTTPServer *server, struct HTTPRequest *request,
                  struct HTTPResponse *resp)
{
    (void)request;

    char *body = (char *)malloc(512);
    if (!body) return -1;

    snprintf(body, 512,
             "{\"status\": \"running\", \"routes\": %d, \"version\": \"1.0.0\"}",
             server->router.count);

    response_header(resp, "Content-Type", "application/json");
    response_body(resp, body, strlen(body));
    return 0;
}

// GET /api/hello
int handle_hello(struct HTTPServer *server, struct HTTPRequest *request,
                 struct HTTPResponse *resp)
{
    (void)server;
    (void)request;

    const char *body = "{\"status\": \"success\", \"message\": \"Hello, World!\"}";
    response_header(resp, "Content-Type", "application/json");
    response_body(resp, strdup(body), strlen(body));
    return 0;
}

// GET /api/about
int handle_about(struct HTTPServer *server, struct HTTPRequest *request,
                 struct HTTPResponse *resp)
{
    (void)server;
    (void)request;

    const char *body =
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>About</title></head>"
        "<body>"
        "<h1>About This Server</h1>"
        "<p>This is a multi-threaded HTTP server written in C.</p>"
        "<p>Features:</p>"
        "<ul>"
        "  <li>Thread pool for concurrent request handling</li>"
        "  <li>HTTP request parsing (GET, POST, etc.)</li>"
        "  <li>Route-based request handling</li>"
        "  <li>Graceful shutdown on SIGINT/SIGTERM</li>"
        "  <li>Structured logging with timestamps</li>"
        "  <li>Thread-safe request parsing (strtok_r)</li>"
        "  <li>URL decoding support</li>"
        "</ul>"
        "</body>"
        "</html>";

    response_header(resp, "Content-Type", "text/html");
    response_body(resp, strdup(body), strlen(body));
    return 0;
}

// POST /api/echo
int handle_echo(struct HTTPServer *server, struct HTTPRequest *request,
                struct HTTPResponse *resp)
{
    (void)server;

    char *body = (char *)malloc(4096);
    if (!body) return -1;

    const char *content_type = get_header(request, "Content-Type");
    char uri_escaped[2048];
    json_escape(request->URI ? request->URI : "(null)", uri_escaped, sizeof(uri_escaped));
    char ct_escaped[512];
    json_escape(content_type ? content_type : "(none)", ct_escaped, sizeof(ct_escaped));
    char body_escaped[2048];
    json_escape(request->body ? request->body : "(empty)", body_escaped, sizeof(body_escaped));

    snprintf(body, 4096,
             "{\"method\": \"POST\", "
             "\"uri\": \"%s\", "
             "\"content_type\": \"%s\", "
             "\"body_length\": %zu, "
             "\"body\": \"%s\"}",
             uri_escaped, ct_escaped,
             request->body_length, body_escaped);

    response_header(resp, "Content-Type", "application/json");
    response_body(resp, body, strlen(body));
    return 0;
}

// GET /api/error
int handle_error_test(struct HTTPServer *server, struct HTTPRequest *request,
                      struct HTTPResponse *resp)
{
    (void)server;
    (void)request;

    // Return a proper 500 status
    response_status(resp, 500, "Internal Server Error");

    const char *body =
        "{\"status\": \"error\", "
        "\"message\": \"This is a test error response\", "
        "\"code\": 500}";

    response_header(resp, "Content-Type", "application/json");
    response_body(resp, strdup(body), strlen(body));
    return 0;
}

// GET /api/headers
int handle_headers(struct HTTPServer *server, struct HTTPRequest *request,
                   struct HTTPResponse *resp)
{
    (void)server;

    // Dynamically allocate to handle any number of headers
    size_t estimated = 256;
    for (int i = 0; i < request->header_count; i++) {
        estimated += strlen(request->headers[i].key) +
                     strlen(request->headers[i].value) + 32;
    }

    char *body = (char *)malloc(estimated);
    if (!body) return -1;

    char *ptr = body;
    char *end = body + estimated;

    ptr += snprintf(ptr, end - ptr, "{\"headers\": {");

    for (int i = 0; i < request->header_count; i++) {
        if (i > 0) ptr += snprintf(ptr, end - ptr, ", ");

        char key_esc[512], val_esc[512];
        json_escape(request->headers[i].key, key_esc, sizeof(key_esc));
        json_escape(request->headers[i].value, val_esc, sizeof(val_esc));

        ptr += snprintf(ptr, end - ptr, "\"%s\": \"%s\"", key_esc, val_esc);
    }

    ptr += snprintf(ptr, end - ptr, "}}");

    response_header(resp, "Content-Type", "application/json");
    response_body(resp, body, strlen(body));
    return 0;
}


// -------------------- MAIN --------------------

int main()
{
    // Initialize the global logger
    logger_init(&g_logger, LOG_DEBUG, stdout, 1);

    LOG_INFO("=== HTTP Server Test Application ===");
    LOG_INFO("Starting up...");

    // Create server on port 8080 with 50 threads
    struct HTTPServer server = HTTPServer_config(8080, 50);

    if (server.server.sockdf < 0) {
        LOG_ERROR("Failed to create server — exiting");
        logger_destroy(&g_logger);
        return 1;
    }

    // Register routes
    // Methods: GET=0, POST=1, PUT=2, DELETE=3, HEAD=4, PATCH=5

    server.register_route(&server, handle_home, "/", 1, 0);
    server.register_route(&server, handle_status, "/api/status", 1, 0);
    server.register_route(&server, handle_hello, "/api/hello", 1, 0);
    server.register_route(&server, handle_about, "/api/about", 1, 0);
    server.register_route(&server, handle_echo, "/api/echo", 1, 1);       // POST
    server.register_route(&server, handle_error_test, "/api/error", 1, 0);
    server.register_route(&server, handle_headers, "/api/headers", 1, 0);

    LOG_INFO("Registered %d routes", server.router.count);
    LOG_INFO("Starting server...");

    // Launch server (runs until Ctrl+C)
    server.launch(&server);

    LOG_INFO("Server stopped");
    logger_destroy(&g_logger);
    return 0;
}
