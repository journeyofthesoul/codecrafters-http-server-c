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
#define OK "HTTP/1.1 200 OK\r\n\r\n"
#define NOT_FOUND "HTTP/1.1 404 Not Found\r\n\r\n"

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
				if (strcmp(req.path, "/") == 0)
					send(clientSocket, OK, strlen(OK), 0);
				else
					send(clientSocket, NOT_FOUND, strlen(NOT_FOUND), 0);
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
