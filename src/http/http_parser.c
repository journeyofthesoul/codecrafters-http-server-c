#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
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

void* handle_client(void* arg) {
    int clientSocket = *(int*)arg;
    free(arg);
    char buffer[1024];
    int bytes;

    // Read the HTTP request
    ssize_t bytes_received = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate the received data
        printf("Received HTTP Request:\n%s\n", buffer);

        HttpRequest req;
        if (parse_http_request(buffer, &req) == 0) {
            printf("Parsed Request:\nMethod: %s\nPath: %s\nVersion: %s\nHeaders:\n",
                req.method, req.path, req.version);
            if (strcmp(req.path, "/") == 0){
                char response[128];  // writable buffer
                strcpy(response, OK); // copy the literal into writable memory
                strcat(response, CRLF); // safely append CRLF as the end of headers
                send(clientSocket, response, strlen(response), 0);
            } else if (strncmp(req.path, ECHO_PREFIX, strlen(ECHO_PREFIX)) == 0){
                char response[128];  // writable buffer
                strcpy(response, OK); // copy the literal into writable memory
                strcat(response, CONTENT_TYPE_TEXT_PLAIN);
                strcat(response, CONTENT_LENGTH_PREFIX);
                int bodyLength = strlen(req.path) - strlen(ECHO_PREFIX);
                char responseBody[bodyLength + 1];
                strncpy(responseBody, req.path + strlen(ECHO_PREFIX), bodyLength);
                char contentLength[20]; // Declare a character array to store the string
                sprintf(contentLength, "%d", bodyLength);
                strcat(response, contentLength);
                strcat(response, CRLF); // append CRLF as part of single header Content-Length
                strcat(response, CRLF); // safely append CRLF as the end of headers
                strcat(response, responseBody);
                send(clientSocket, response, strlen(response), 0);
            } else if (strncmp(req.path, USER_AGENT_PREFIX, strlen(USER_AGENT_PREFIX)) == 0){
                char response[512];  // writable buffer
                strcpy(response, OK); // copy the literal into writable memory
                strcat(response, CONTENT_TYPE_TEXT_PLAIN);
                const char *userAgent = "Unknown";
                for (int i = 0; i < req.header_count; i++) {
                    if (strcmp(req.headers[i].key, "User-Agent") == 0) {
                        userAgent = req.headers[i].value;
                        break;
                    }
                }
                strcat(response, CONTENT_LENGTH_PREFIX);
                int bodyLength = strlen(userAgent);
                char contentLength[20]; // Declare a character array to store the string
                sprintf(contentLength, "%d", bodyLength);
                strcat(response, contentLength);
                strcat(response, CRLF); // append CRLF as part of single header Content-Length
                strcat(response, CRLF); // safely append CRLF as the end of headers
                strcat(response, userAgent);
                send(clientSocket, response, strlen(response), 0);
            } else {
                char response[128];  // writable buffer
                strcpy(response, NOT_FOUND); // copy the literal into writable memory
                strcat(response, CRLF); // safely append CRLF as the end of headers
                send(clientSocket, response, strlen(response), 0);
            }
            free_http_request(&req);
        } else {
            fprintf(stderr, "Failed to parse HTTP request.\n");
        }
    } else if (bytes_received == 0) {
        printf("Client disconnected.\n");
    } else {
        perror("Error receiving data");
    }

    close(clientSocket);
    return NULL;
}
