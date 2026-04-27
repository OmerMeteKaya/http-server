//
// Created by mete on 15.04.2026.
//

#ifndef HTTP_SERVER_HTTPRESPONSE_H
#define HTTP_SERVER_HTTPRESPONSE_H

#include <stddef.h>

#define MAX_RESPONSE_HEADERS 50
#define MAX_HEADER_KEY_LEN 128
#define MAX_HEADER_VALUE_LEN 512

struct ResponseHeader {
    char key[MAX_HEADER_KEY_LEN];
    char value[MAX_HEADER_VALUE_LEN];
};

struct HTTPResponse {
    int status_code;
    const char *status_text;

    struct ResponseHeader headers[MAX_RESPONSE_HEADERS];
    int header_count;

    // Body is allocated separately; the server serializes the full response.
    char *body;
    size_t body_length;
};

// Initialize a response with default values
void response_init(struct HTTPResponse *resp);

// Set the status code and text
void response_status(struct HTTPResponse *resp, int code, const char *text);

// Add a header (returns -1 if max headers reached)
int response_header(struct HTTPResponse *resp, const char *key, const char *value);

// Set the body (takes ownership of the pointer, or NULL)
void response_body(struct HTTPResponse *resp, char *body, size_t length);

// Serialize the full HTTP response into a malloc'd buffer.
// Caller must free the returned string.
char* response_serialize(const struct HTTPResponse *resp);

// Free internal allocations of the response (body, headers)
void response_destroy(struct HTTPResponse *resp);

// Convenience: build a simple text response
char* response_make(int status_code, const char *status_text,
                    const char *content_type, const char *body);

#endif //HTTP_SERVER_HTTPRESPONSE_H
