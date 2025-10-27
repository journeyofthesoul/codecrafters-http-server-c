#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "http_parser.h"

// Trim leading and trailing whitespace
char *trim(char *str) {
    while (*str == ' ' || *str == '\t') str++;
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == '\r' || *end == '\n' || *end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }
    return str;
}

// Parse HTTP request string
int parse_http_request(const char *raw, HttpRequest *req) {
    char *request = strdup(raw);
    if (!request) return -1;

    char *line = strtok(request, "\r\n");
    if (!line) { free(request); return -1; }

    // Parse request line (method, path, version)
    sscanf(line, "%15s %255s %15s", req->method, req->path, req->version);

    req->header_count = 0;

    // Parse headers until blank line
    while ((line = strtok(NULL, "\r\n")) && strlen(line) > 0) {
        char *colon = strchr(line, ':');
        if (!colon) continue; // invalid header
        *colon = '\0';
        req->headers[req->header_count].key = strdup(trim(line));
        req->headers[req->header_count].value = strdup(trim(colon + 1));
        req->header_count++;
    }

    // Remaining data (if any) is body
    char *body_start = strtok(NULL, "");
    req->body = body_start ? strdup(body_start) : NULL;

    free(request);
    return 0;
}

void free_http_request(HttpRequest *req) {
    for (int i = 0; i < req->header_count; i++) {
        free(req->headers[i].key);
        free(req->headers[i].value);
    }
    free(req->body);
}
