#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "http.h"

#define PORT 8080


int main() {
    int server_fd, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 10) == -1) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d...\n", PORT);

    // Accept
    while (1) {
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("Connection acceptance failed");
            continue;	
        }

        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd); 
            process_request(client_socket);
            close(client_socket);
            exit(0);
        } else if (pid > 0) {
            close(client_socket);
        } else {
            perror("Fork failed");
        }
    }

    close(server_fd);
    return 0;
}

