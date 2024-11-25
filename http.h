#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H




#define BUFFER_SIZE (1024*2)


const char *get_type(const char *path);


void process_request(int client_socket);


void serve_file(int client_socket, const char *file_path);


void serve_directory(int client_socket, const char *dir_path);


void execute_cgi(int client_socket, const char *cgi_path);


void send_error(int client_socket, int error_code);

#endif 
