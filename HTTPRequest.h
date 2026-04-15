//
// Created by mete on 13.04.2026.
//

#ifndef HTTP_SERVER_HTTPREQUEST_H
#define HTTP_SERVER_HTTPREQUEST_H

#include <stddef.h>

#define MAX_HEADERS 100
#define MAX_REQUEST_SIZE 16384
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
}HTTPMethod;
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
};

struct HTTPRequest HTTPRequest_constructor(char *buffer);
const char* get_header(const struct HTTPRequest *req, const char *key);
void free_request(struct HTTPRequest *req);

#endif //HTTP_SERVER_HTTPREQUEST_H
