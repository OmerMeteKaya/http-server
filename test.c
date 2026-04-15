//
// Created by mete on 12.04.2026.
//

#include "HTTPServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// HTTP SERVER - TEST APPLICATION
// 
// This file demonstrates a complete HTTP server with multiple routes,
// JSON responses, POST handling, and error handling.
//
// Run: ./cmake-build-debug/http_server
// Test: curl http://localhost:8080/
// ============================================================================

// -------------------- HELPER: JSON RESPONSE BUILDER --------------------

static char* make_json_response(const char *key, const char *value)
{
    char *body = (char *)malloc(1024);
    if (!body) return NULL;
    
    snprintf(body, 1024,
             "{\"status\": \"success\", \"%s\": \"%s\"}",
             key, value);
    return body;
}

// -------------------- ROUTE HANDLERS --------------------

// GET /
char* handle_home(struct HTTPServer *server, struct HTTPRequest *request)
{
    (void)server;
    (void)request;
    
    char *body = (char *)malloc(2048);
    if (!body) return strdup("Error");
    
    snprintf(body, 2048,
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
        "</ul>"
        "</body>"
        "</html>");
    return body;
}

// GET /api/status
char* handle_status(struct HTTPServer *server, struct HTTPRequest *request)
{
    (void)request;
    
    char *body = (char *)malloc(512);
    if (!body) return NULL;
    
    snprintf(body, 512,
             "{\"status\": \"running\", \"routes\": %d, \"version\": \"1.0.0\"}",
             server->router.count);
    return body;
}

// GET /api/hello
char* handle_hello(struct HTTPServer *server, struct HTTPRequest *request)
{
    (void)server;
    (void)request;
    
    return make_json_response("message", "Hello, World!");
}

// GET /api/about
char* handle_about(struct HTTPServer *server, struct HTTPRequest *request)
{
    (void)server;
    (void)request;
    
    char *body = (char *)malloc(1024);
    if (!body) return strdup("Error");
    
    snprintf(body, 1024,
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
        "</ul>"
        "</body>"
        "</html>");
    return body;
}

// POST /api/echo
char* handle_echo(struct HTTPServer *server, struct HTTPRequest *request)
{
    (void)server;
    
    char *body = (char *)malloc(2048);
    if (!body) return strdup("Error");
    
    const char *content_type = get_header(request, "Content-Type");
    
    snprintf(body, 2048,
             "{\"method\": \"POST\", "
             "\"uri\": \"%s\", "
             "\"content_type\": \"%s\", "
             "\"body_length\": %zu, "
             "\"body\": \"%s\"}",
             request->URI ? request->URI : "(null)",
             content_type ? content_type : "(none)",
             request->body_length,
             request->body ? request->body : "(empty)");
    return body;
}

// GET /api/error
char* handle_error_test(struct HTTPServer *server, struct HTTPRequest *request)
{
    (void)server;
    (void)request;
    
    // Simulate an error response
    char *body = (char *)malloc(512);
    if (!body) return NULL;
    
    snprintf(body, 512,
             "{\"status\": \"error\", "
             "\"message\": \"This is a test error response\", "
             "\"code\": 500}");
    return body;
}

// GET /api/headers
char* handle_headers(struct HTTPServer *server, struct HTTPRequest *request)
{
    (void)server;
    
    char *body = (char *)malloc(2048);
    if (!body) return strdup("Error");
    
    char *ptr = body;
    char *end = body + 2048;
    
    ptr += snprintf(ptr, end - ptr, "{\"headers\": {");
    
    for (int i = 0; i < request->header_count; i++) {
        if (i > 0) ptr += snprintf(ptr, end - ptr, ", ");
        ptr += snprintf(ptr, end - ptr, "\"%s\": \"%s\"",
                        request->headers[i].key,
                        request->headers[i].value);
    }
    
    ptr += snprintf(ptr, end - ptr, "}}");
    
    return body;
}

// -------------------- MAIN --------------------

int main()
{
    printf("=== HTTP Server Test Application ===\n\n");
    
    // Create server on port 8080
    struct HTTPServer server = HTTPServer_constructor();
    
    // Register routes
    // Methods: GET=1, POST=2, PUT=3, DELETE=4, HEAD=5, PATCH=6
    
    server.register_routes(&server, handle_home, "/", 1, 1);
    server.register_routes(&server, handle_status, "/api/status", 1, 1);
    server.register_routes(&server, handle_hello, "/api/hello", 1, 1);
    server.register_routes(&server, handle_about, "/api/about", 1, 1);
    server.register_routes(&server, handle_echo, "/api/echo", 1, 2);  // POST only
    server.register_routes(&server, handle_error_test, "/api/error", 1, 1);
    server.register_routes(&server, handle_headers, "/api/headers", 1, 1);
    
    printf("\n[Test] Registered %d routes\n", server.router.count);
    printf("[Test] Starting server...\n\n");
    
    // Launch server (runs until Ctrl+C)
    server.launch(&server);
    
    // This line is never reached due to infinite loop in launch()
    printf("[Test] Server stopped\n");
    return 0;
}
