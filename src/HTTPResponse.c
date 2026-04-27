//
// Created by mete on 15.04.2026.
//

#include "../include/HTTPResponse.h"
#include "../include/Logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

void response_init(struct HTTPResponse *resp)
{
    resp->status_code = 200;
    resp->status_text = "OK";
    resp->header_count = 0;
    resp->body = NULL;
    resp->body_length = 0;
}

void response_status(struct HTTPResponse *resp, int code, const char *text)
{
    resp->status_code = code;
    resp->status_text = text;
}

int response_header(struct HTTPResponse *resp, const char *key, const char *value)
{
    if (resp->header_count >= MAX_RESPONSE_HEADERS) {
        LOG_WARN("Response header limit reached (%d)", MAX_RESPONSE_HEADERS);
        return -1;
    }

    int idx = resp->header_count;
    snprintf(resp->headers[idx].key, MAX_HEADER_KEY_LEN, "%s", key);
    snprintf(resp->headers[idx].value, MAX_HEADER_VALUE_LEN, "%s", value);
    resp->header_count++;
    return 0;
}

void response_body(struct HTTPResponse *resp, char *body, size_t length)
{
    // Free existing body if set
    if (resp->body) {
        free(resp->body);
    }
    resp->body = body;
    resp->body_length = length;
}

char* response_serialize(const struct HTTPResponse *resp)
{
    // Calculate total size needed
    size_t size = 256; // HTTP line + Connection + blank line + margin

    // Status line
    char status_line[128];
    int slen = snprintf(status_line, sizeof(status_line),
                        "HTTP/1.1 %d %s\r\n",
                        resp->status_code,
                        resp->status_text ? resp->status_text : "Unknown");
    if (slen < 0) return NULL;
    size += (size_t)slen;

    // Date header
    char date_header[128];
    time_t now = time(NULL);
    struct tm tm_info;
    gmtime_r(&now, &tm_info);
    int dlen = strftime(date_header, sizeof(date_header),
                        "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", &tm_info);
    if (dlen > 0) size += (size_t)dlen;

    // Custom headers
    for (int i = 0; i < resp->header_count; i++) {
        size += strlen(resp->headers[i].key) + strlen(resp->headers[i].value) + 4; // ": \r\n"
    }

    // Content-Length
    char cl_header[64];
    int clen = snprintf(cl_header, sizeof(cl_header),
                        "Content-Length: %zu\r\n", resp->body_length);
    if (clen < 0) return NULL;
    size += (size_t)clen;

    // Connection + blank line
    size += strlen("Connection: close\r\n\r\n");

    // Body
    size += resp->body_length;

    // Allocate and build
    char *buffer = (char *)malloc(size + 1);
    if (!buffer) {
        LOG_ERROR("Failed to allocate response buffer (%zu bytes)", size);
        return NULL;
    }

    char *ptr = buffer;
    char *end = buffer + size;

    ptr += snprintf(ptr, end - ptr, "%s", status_line);
    if (dlen > 0) ptr += snprintf(ptr, end - ptr, "%s", date_header);

    for (int i = 0; i < resp->header_count; i++) {
        ptr += snprintf(ptr, end - ptr, "%s: %s\r\n",
                        resp->headers[i].key, resp->headers[i].value);
    }

    ptr += snprintf(ptr, end - ptr, "%s", cl_header);
    ptr += snprintf(ptr, end - ptr, "Connection: close\r\n\r\n");

    if (resp->body && resp->body_length > 0) {
        memcpy(ptr, resp->body, resp->body_length);
        ptr += resp->body_length;
    }

    *ptr = '\0';
    return buffer;
}

void response_destroy(struct HTTPResponse *resp)
{
    if (resp->body) {
        free(resp->body);
        resp->body = NULL;
    }
    resp->body_length = 0;
    resp->header_count = 0;
}

char* response_make(int status_code, const char *status_text,
                    const char *content_type, const char *body)
{
    struct HTTPResponse resp;
    response_init(&resp);
    response_status(&resp, status_code, status_text);

    if (content_type) {
        response_header(&resp, "Content-Type", content_type);
    }

    if (body) {
        size_t len = strlen(body);
        char *body_copy = (char *)malloc(len + 1);
        if (!body_copy) {
            LOG_ERROR("Failed to allocate body copy (%zu bytes)", len);
            return NULL;
        }
        memcpy(body_copy, body, len + 1);
        response_body(&resp, body_copy, len);
    }

    char *serialized = response_serialize(&resp);
    response_destroy(&resp);
    return serialized;
}
