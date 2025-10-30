#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "http/http_parser.h"

int main() {
	int connection_backlog = 10;
	char buffer[BUFFER_SIZE];
	pthread_t tid;
	int server_fd, client_addr_len;
	int* clientSocket;
	struct sockaddr_in client_addr;

	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	printf("HTTP Server started\n");
	
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
	
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}
	
	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);
	
	while (1) {
		clientSocket = malloc(sizeof(int));
        // Accept a client connection
		*clientSocket = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
		if (*clientSocket < 0) {
			perror("Error accepting connection");
			continue;
		}

        printf("Client connected from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		pthread_create(&tid, NULL, handle_client, clientSocket);
        pthread_detach(tid); // auto-cleanup
	}

	// Close the server socket
	close(server_fd);

	return 0;
}
