#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "http/http_parser.h"

#define BUFFER_SIZE 1024
#define OK "HTTP/1.1 200 OK\r\n"
#define NOT_FOUND "HTTP/1.1 404 Not Found\r\n"
#define CRLF "\r\n"
#define ECHO_PREFIX "/echo/"
#define USER_AGENT_PREFIX "/user-agent"
#define CONTENT_TYPE_TEXT_PLAIN "Content-Type: text/plain\r\n"
#define CONTENT_LENGTH_PREFIX "Content-Length: "

int main() {
	int connection_backlog = 5;
	char buffer[BUFFER_SIZE];

	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	printf("HTTP Server started\n");
	
	int server_fd, client_addr_len, clientSocket;
	struct sockaddr_in client_addr;
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	
	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}
	
	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
									 .sin_port = htons(4221),
									 .sin_addr = { htonl(INADDR_ANY) },
									};
	
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	
	
	while (1) {
		if (listen(server_fd, connection_backlog) != 0) {
			printf("Listen failed: %s \n", strerror(errno));
			return 1;
		}
		
		printf("Waiting for a client to connect...\n");
		client_addr_len = sizeof(client_addr);

        // Accept a client connection
		clientSocket = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
		if (clientSocket < 0) {
			perror("Error accepting connection");
			continue;
		}

        printf("Client connected from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

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

		// Close the client socket
		close(clientSocket);
	}

	// Close the server socket
	close(server_fd);

	return 0;
}
