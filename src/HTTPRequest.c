//
// Created by mete on 13.04.2026.
//

#include "../include/HTTPRequest.h"
#include "../include/Logger.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>


// -------------------- HELPER: URL DECODE --------------------

static int hex_to_int(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

void url_decode(const char *src, char *dst, size_t dst_size)
{
    size_t di = 0;
    size_t si = 0;
    size_t src_len = strlen(src);

    while (si < src_len && di < dst_size - 1) {
        if (src[si] == '%' && si + 2 < src_len) {
            int hi = hex_to_int(src[si + 1]);
            int lo = hex_to_int(src[si + 2]);
            if (hi >= 0 && lo >= 0) {
                dst[di++] = (char)((hi << 4) | lo);
                si += 3;
                continue;
            }
        }
        if (src[si] == '+') {
            dst[di++] = ' ';
        } else {
            dst[di++] = src[si];
        }
        si++;
    }
    dst[di] = '\0';
}


// -------------------- METHOD PARSER --------------------

static HTTPMethod method_select(const char *method)
{
    if (strcasecmp(method, "GET") == 0)     return GET;
    if (strcasecmp(method, "POST") == 0)    return POST;
    if (strcasecmp(method, "PUT") == 0)     return PUT;
    if (strcasecmp(method, "HEAD") == 0)    return HEAD;
    if (strcasecmp(method, "PATCH") == 0)   return PATCH;
    if (strcasecmp(method, "DELETE") == 0)  return DELETE;
    if (strcasecmp(method, "CONNECT") == 0) return CONNECT;
    if (strcasecmp(method, "OPTIONS") == 0) return OPTIONS;
    if (strcasecmp(method, "TRACE") == 0)   return TRACE;
    return HTTP_INVALID;
}

const char* method_to_str(HTTPMethod method)
{
    switch (method) {
        case GET:     return "GET";
        case POST:    return "POST";
        case PUT:     return "PUT";
        case HEAD:    return "HEAD";
        case PATCH:   return "PATCH";
        case DELETE:  return "DELETE";
        case CONNECT: return "CONNECT";
        case OPTIONS: return "OPTIONS";
        case TRACE:   return "TRACE";
        default:      return "INVALID";
    }
}


// -------------------- MAIN PARSER --------------------

int HTTPRequest_constructor(struct HTTPRequest *req, char *buffer)
{
    // Zero-init
    memset(req, 0, sizeof(struct HTTPRequest));
    req->Method = HTTP_INVALID;
    req->body = NULL;

    if (!buffer || buffer[0] == '\0') {
        LOG_WARN("Empty or NULL request buffer");
        return -1;
    }

    // -------- REQUEST LINE --------
    char *saveptr = NULL;
    char *request_line = strtok_r(buffer, "\r\n", &saveptr);

    if (!request_line) {
        LOG_WARN("No request line found in buffer");
        return -1;
    }

    char method[MAX_METHOD_LENGTH];
    char uri[MAX_URI_LENGTH];
    char version_str[MAX_VERSION_LENGTH];

    if (sscanf(request_line, "%15s %2047s %31s", method, uri, version_str) != 3) {
        LOG_WARN("Invalid request line: %s", request_line);
        return -1;
    }

    req->Method = method_select(method);
    if (req->Method == HTTP_INVALID) {
        LOG_WARN("Unknown HTTP method: %s", method);
    }

    // URL-decode the URI
    req->URI = (char *)malloc(strlen(uri) + 1);
    if (!req->URI) {
        LOG_ERROR("malloc failed for URI");
        return -1;
    }
    url_decode(uri, req->URI, strlen(uri) + 1);

    // HTTP Version parse
    char *slash = strchr(version_str, '/');
    if (slash) {
        sscanf(slash + 1, "%d.%d", &req->version.major, &req->version.minor);
    }

    // -------- HEADERS --------
    char *header_line;

    while ((header_line = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
        if (strlen(header_line) == 0)
            break;

        char *colon = strchr(header_line, ':');
        if (!colon)
            continue;

        *colon = '\0';

        char *key = header_line;
        char *value = colon + 1;

        // Skip leading whitespace in value
        while (*value == ' ') value++;

        if (req->header_count < MAX_HEADERS) {
            req->headers[req->header_count].key = strdup(key);
            req->headers[req->header_count].value = strdup(value);

            if (!req->headers[req->header_count].key ||
                !req->headers[req->header_count].value) {
                LOG_ERROR("malloc failed for header");
                free(req->headers[req->header_count].key);
                free(req->headers[req->header_count].value);
                req->headers[req->header_count].key = NULL;
                req->headers[req->header_count].value = NULL;
                return -1;
            }

            req->header_count++;
        }
    }

    // -------- BODY --------
    // Check Content-Length header for accurate body parsing
    const char *content_length_str = get_header(req, "Content-Length");
    char *body_ptr = strtok_r(NULL, "", &saveptr);

    if (body_ptr && strlen(body_ptr) > 0) {
        if (content_length_str) {
            size_t content_length = (size_t)atol(content_length_str);
            size_t available = strlen(body_ptr);
            size_t body_len = (content_length < available) ? content_length : available;

            req->body = (char *)malloc(body_len + 1);
            if (!req->body) {
                LOG_ERROR("malloc failed for body (%zu bytes)", body_len);
                return -1;
            }
            memcpy(req->body, body_ptr, body_len);
            req->body[body_len] = '\0';
            req->body_length = body_len;
        } else {
            req->body = strdup(body_ptr);
            if (!req->body) {
                LOG_ERROR("malloc failed for body (strdup)");
                return -1;
            }
            req->body_length = strlen(body_ptr);
        }
    }

    LOG_DEBUG("Parsed request: %s %s HTTP/%d.%d (body=%zu)",
              method_to_str(req->Method), req->URI,
              req->version.major, req->version.minor, req->body_length);

    return 0;
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

void free_request(struct HTTPRequest *req)
{
    if (!req) return;

    if (req->URI) {
        free(req->URI);
        req->URI = NULL;
    }

    for (int i = 0; i < req->header_count; i++) {
        free(req->headers[i].key);
        free(req->headers[i].value);
    }
    req->header_count = 0;

    if (req->body) {
        free(req->body);
        req->body = NULL;
    }
    req->body_length = 0;
}
