#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#define MAX_HEADERS 50

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

#endif // HTTP_PARSER_H