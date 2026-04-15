//
// Created by mete on 13.04.2026.
//

#include "HTTPRequest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>



// -------------------- METHOD PARSER --------------------

static HTTPMethod method_select(const char *method)
{
    if (strcasecmp(method, "GET") == 0) return GET;
    if (strcasecmp(method, "POST") == 0) return POST;
    if (strcasecmp(method, "PUT") == 0) return PUT;
    if (strcasecmp(method, "HEAD") == 0) return HEAD;
    if (strcasecmp(method, "PATCH") == 0) return PATCH;
    if (strcasecmp(method, "DELETE") == 0) return DELETE;
    if (strcasecmp(method, "CONNECT") == 0) return CONNECT;
    if (strcasecmp(method, "OPTIONS") == 0) return OPTIONS;
    if (strcasecmp(method, "TRACE") == 0) return TRACE;

    return HTTP_INVALID;
}


// -------------------- MAIN PARSER --------------------

struct HTTPRequest HTTPRequest_constructor(char *buffer)
{
    struct HTTPRequest req;

    req.Method = HTTP_INVALID;
    req.URI = NULL;
    req.version.major = 0;
    req.version.minor = 0;
    req.header_count = 0;
    req.body = NULL;
    req.body_length = 0;

    // -------- REQUEST LINE --------
    char *request_line = strtok(buffer, "\r\n");

    if (!request_line) {
        fprintf(stderr, "[HTTP] Invalid request: empty request line\n");
        return req;
    }

    char method[MAX_METHOD_LENGTH];
    char uri[MAX_URI_LENGTH];
    char version_str[MAX_VERSION_LENGTH];

    if (sscanf(request_line, "%15s %2047s %31s", method, uri, version_str) != 3) {
        fprintf(stderr, "[HTTP] Invalid request line format\n");
        return req;
    }

    req.Method = method_select(method);
    req.URI = strdup(uri);

    // HTTP Version parse
    char *slash = strchr(version_str, '/');
    if (slash) {
        sscanf(slash + 1, "%d.%d", &req.version.major, &req.version.minor);
    }

    // -------- HEADERS --------
    char *header_line;

    while ((header_line = strtok(NULL, "\r\n")) != NULL) {


        if (strlen(header_line) == 0)
            break;

        char *colon = strchr(header_line, ':');
        if (!colon)
            continue;

        *colon = '\0';

        char *key = header_line;
        char *value = colon + 1;


        while (*value == ' ') value++;

        if (req.header_count < MAX_HEADERS) {
            req.headers[req.header_count].key = strdup(key);
            req.headers[req.header_count].value = strdup(value);
            req.header_count++;
        }
    }

    // -------- BODY --------
    // Check Content-Length header for accurate body parsing
    const char *content_length_str = get_header(&req, "Content-Length");
    char *body_ptr = strtok(NULL, "");
    
    if (body_ptr && strlen(body_ptr) > 0) {
        if (content_length_str) {
            size_t content_length = (size_t)atol(content_length_str);
            size_t available = strlen(body_ptr);
            size_t body_len = (content_length < available) ? content_length : available;
            
            req.body = (char *)malloc(body_len + 1);
            memcpy(req.body, body_ptr, body_len);
            req.body[body_len] = '\0';
            req.body_length = body_len;
        } else {
            req.body = strdup(body_ptr);
            req.body_length = strlen(body_ptr);
        }
    }

    return req;
}

// -------------------- HEADER GETTER --------------------
const char* get_header(const struct HTTPRequest *req, const char *key)
{
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].key, key) == 0)
            return req->headers[i].value;
    }
    return NULL;
}

// -------------------- CLEANUP --------------------

void free_request(struct HTTPRequest *req) {
    if (req->URI)
        free(req->URI);

    for (int i = 0; i < req->header_count; i++) {
        free(req->headers[i].key);
        free(req->headers[i].value);
    }

    if (req->body)
        free(req->body);

}
