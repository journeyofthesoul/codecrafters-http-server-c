#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#define MAX_HEADERS 50
#define BUFFER_SIZE 1024
#define OK "HTTP/1.1 200 OK\r\n"
#define NOT_FOUND "HTTP/1.1 404 Not Found\r\n"
#define CRLF "\r\n"
#define ECHO_PREFIX "/echo/"
#define USER_AGENT_PREFIX "/user-agent"
#define CONTENT_TYPE_TEXT_PLAIN "Content-Type: text/plain\r\n"
#define CONTENT_LENGTH_PREFIX "Content-Length: "

typedef struct {
    char *key;
    char *value;
} Header;

typedef struct {
    char method[16];
    char path[256];
    char version[16];
    Header headers[MAX_HEADERS];
    int header_count;
    char *body;
} HttpRequest;

char *trim(char *str);
int parse_http_request(const char *raw, HttpRequest *req);
void free_http_request(HttpRequest *req);
void* handle_client(void* arg);

#endif // HTTP_PARSER_H