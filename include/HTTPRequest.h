//
// Created by mete on 13.04.2026.
//

#ifndef HTTP_SERVER_HTTPREQUEST_H
#define HTTP_SERVER_HTTPREQUEST_H

#include <stddef.h>

#define MAX_HEADERS 100
#define MAX_REQUEST_SIZE 65536
#define MAX_URI_LENGTH 2048
#define MAX_METHOD_LENGTH 16
#define MAX_VERSION_LENGTH 32

typedef enum
{
    HTTP_INVALID = -1,
    GET,
    POST,
    PUT,
    HEAD,
    PATCH,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE
} HTTPMethod;

struct HTTPVersion
{
    int major;
    int minor;
};

struct Header {
    char *key;
    char *value;
};

struct HTTPRequest {
    HTTPMethod Method;
    char *URI;
    struct HTTPVersion version;

    struct Header headers[MAX_HEADERS];
    int header_count;

    char *body;
    size_t body_length;

    // Client info (populated by the server)
    char client_ip[64];
    int client_port;
};

// Parses a raw HTTP request buffer.
// Uses strtok_r for thread-safety.
// Returns: 0 on success, -1 on parse error.
int HTTPRequest_constructor(struct HTTPRequest *req, char *buffer);

// URL-decode a string in-place (writes into dst)
void url_decode(const char *src, char *dst, size_t dst_size);

const char* get_header(const struct HTTPRequest *req, const char *key);
void free_request(struct HTTPRequest *req);

// Method name helper
const char* method_to_str(HTTPMethod method);

#endif //HTTP_SERVER_HTTPREQUEST_H
