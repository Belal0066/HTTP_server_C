#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include "http.h"


const char *get_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".txt") == 0) return "text/plain";
    return "application/octet-stream";
}

void process_request(int client_socket) {
    char request[BUFFER_SIZE];
    memset(request, 0, BUFFER_SIZE);

    // read request
    ssize_t bytes_received = read(client_socket, request, BUFFER_SIZE - 1);
    if (bytes_received <= 0) {
        send_error(client_socket, 400);  //Bad Request
        return;
    }

    // parse
    char method[16], path[256];
    if (sscanf(request, "%15s %255s", method, path) != 2) {
        send_error(client_socket, 400);  
        return;
    }

    if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0) {
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response),
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Allow: GET, HEAD\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "\r\n");
        send(client_socket, response, strlen(response), 0);
        send_error(client_socket, 405);
        return;
    }

    if (strstr(path, "..") != NULL) {
        send_error(client_socket, 403);  //Forbidden
        return;
    }

    if (path[0] == '/') {
        memmove(path, path + 1, strlen(path));
    }
    if (strlen(path) == 0) {
        strcpy(path, "index.html"); //Default file
    }

    struct stat path_stat;
    if (stat(path, &path_stat) == -1) {
        send_error(client_socket, 404);
        return;
    }

    if (S_ISDIR(path_stat.st_mode)) {
        serve_directory(client_socket, path);
    } else if (S_ISREG(path_stat.st_mode)) {
        if (strstr(path, ".cgi") != NULL) {
            execute_cgi(client_socket, path);
        } else {
            serve_file(client_socket, path);
        }
    } else {
        send_error(client_socket, 404);
    }
}


void serve_file(int client_socket, const char *file_path) {
    char buffer[BUFFER_SIZE];
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd == -1) {
        send_error(client_socket, 500);
        return;
    }

    const char *mime_type = get_type(file_path);
    snprintf(buffer, sizeof(buffer),
             "HTTP/1.1 200 OK\r\n" 
             "Content-Type: %s\r\n\r\n", mime_type);
    send(client_socket, buffer, strlen(buffer), 0);

    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    close(file_fd);
}

void serve_directory(int client_socket, const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        send_error(client_socket, 500);
        return;
    }

    char body[BUFFER_SIZE * 10] = "<html><body><ul>";
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        strcat(body, "<li>");
        strcat(body, entry->d_name);
        strcat(body, "</li>");
    }
    strcat(body, "</ul></body></html>");
    closedir(dir);

    char header[BUFFER_SIZE];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %zu\r\n\r\n",
             strlen(body));
    send(client_socket, header, strlen(header), 0);
    send(client_socket, body, strlen(body), 0);
}

void execute_cgi(int client_socket, const char *cgi_path) {
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        send_error(client_socket, 500);
        return;
    }

    const char *initial_header = "HTTP/1.1 200 OK\r\n";
    send(client_socket, initial_header, strlen(initial_header), 0);

    pid_t pid = fork();
    if (pid == 0) {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO); 
        
        setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
        setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
        setenv("REQUEST_METHOD", "GET", 1);
        setenv("SCRIPT_NAME", cgi_path, 1);
        
        execl(cgi_path, cgi_path, NULL); 
        exit(1);
    } else if (pid > 0) {

        close(pipe_fd[1]);
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;

        while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer))) > 0) {
            send(client_socket, buffer, bytes_read, 0);
        }
        close(pipe_fd[0]);
        waitpid(pid, NULL, 0);
    } else {

        send_error(client_socket, 500);
    }
}


void send_error(int client_socket, int error_code) {
    const char *status;
    const char *title;
    const char *description;
    
    switch (error_code) {
        case 400:
            status = "400";
            title = "400 Bad Request";
            description = "ايه دا يا غالي؟";
            break;
        case 403:
            status = "403";
            title = "403 Forbidden";
            description = "انت تبع مين؟";
            break;
        case 404:
            status = "404";
            title = "404 Not Found";
            description = "هو قالك فين؟";
            break;
        default:
            status = "500";
            title = "500 Internal Server Error";
            description = "عندي انا دي معلش";
            break;
    }

    // html for err
    char body[BUFFER_SIZE];
    snprintf(body, sizeof(body),
    "<!DOCTYPE HTML>\n"
    "<html>\n"
    "<head>\n"
    "    <title>%s</title>\n"
    "    <style>\n"
    "        body { \n"
    "            background-color: #121212; \n"
    "            color: #e0e0e0; \n"
    "            font-family: Arial, sans-serif; \n"
    "            padding: 2em; \n"
    "            margin: 0; \n"
    "        }\n"
    "        h1 { \n"
    "            font-size: 3em; \n"
    "            color: #ff0000; \n"
    "        }\n"
    "        .error-container { \n"
    "            border: 1px solid #444; \n"
    "            padding: 2em; \n"
    "            background-color: #1d1d1d; \n"
    "            border-radius: 10px; \n"
    "            box-shadow: 0 0 10px rgba(255, 0, 0, 0.6); \n"
    "        }\n"
    "        p { \n"
    "            color: #aaa; \n"
    "        }\n"
    "        hr { \n"
    "            border: 0; \n"
    "            border-top: 2px solid #444; \n"
    "            margin: 2em 0; \n"
    "            opacity: 0.5; \n"
    "        }\n"
    "        small { \n"
    "            font-size: 0.8em; \n"
    "            color: #bbb; \n"
    "        }\n"
    "    </style>\n"
    "</head>\n"
    "<body>\n"
    "    <div class='error-container'>\n"
    "        <h1>%s</h1>\n"
    "        <p>%s</p>\n"
    "        <hr>\n"
    "        <small>- Belal HTTP Server</small>\n"
    "    </div>\n"
    "</body>\n"
    "</html>",
    title, title, description);

    
    char response[BUFFER_SIZE * 2];
    snprintf(response, sizeof(response),
        "HTTP/1.1 %s\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "Server: Belal-HTTP-Server/1.0\r\n"
        "\r\n"
        "%s",
        status, strlen(body), body);

    send(client_socket, response, strlen(response), 0);
}