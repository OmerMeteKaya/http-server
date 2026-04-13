//
// Created by mete on 13.04.2026.
//

#include "HTTPRequest.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>



// -------------------- METHOD PARSER --------------------

HTTPMethod method_select(const char *method)
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
    char *line = strtok(buffer, "\r\n");

    if (!line) {
        printf("Invalid request\n");
        return req;
    }

    char method[16];
    char uri[1024];
    char version_str[32];

    if (sscanf(line, "%15s %1023s %31s", method, uri, version_str) != 3) {
        printf("Invalid request line\n");
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
    char *body_ptr = strtok(NULL, "");
    if (body_ptr) {
        req.body = body_ptr;
        req.body_length = strlen(body_ptr);
    }

    return req;
}

// -------------------- HEADER GETTER --------------------
char* get_header(struct HTTPRequest *req, const char *key)
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

}