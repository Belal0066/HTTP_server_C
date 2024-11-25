# HTTP_server_C

A lightweight HTTP server implementation in C that handles basic web requests, serves static files, generates directory listings, and supports CGI script execution.

## Features

- **HTTP Request Handling**: Processes GET and HEAD requests from web browsers
- **Static File Serving**: Serves various file types with appropriate types
- **Directory Listing**: Generates HTML listings for directory contents
- **CGI Support**: Executes and serves CGI scripts
- **Concurrent Access**: Handles multiple client connections using process forking
- **Error Handling**: Custom error pages with Arabic messages





## Configuration

Default settings:
- Port: 8080 (you can check if the port is in use by `lsof -i :8080`)
- Buffer Size: 2048 bytes
- Default page: index.html

## Building the Project

1. Clone the repository
2. build the project and run it

The server will start listening on port 8080 by default. (can be changed from `server.c`)

2. Access the server:
- Open a web browser and navigate to `http://localhost:8080` or any loopback ip with the choosen port.
- Or use cURL from your terminal: `curl http://localhost:8080`

## Project Structure

- `server.c`: Main server implementation (socket handling and connection management)
- `http.c`: HTTP protocol implementation (request parsing, response generation)
- `http.h`: Header file containing function declarations and constants

## Supported Features

### 1. Directory Listing
When accessing a directory, the server generates an HTML page listing all files and subdirectories.

### 2. File Serving
Supports various file types including:
- HTML (.html, .htm)
- CSS (.css)
- JavaScript (.js)
- Images (.png, .jpg, .jpeg)
- Text files (.txt)

### 3. CGI Execution (make sure the script is executable. if not use: `chmod +x [script.cgi]`)
- Files with `.cgi` extension are executed as CGI scripts
- Script output is captured and sent to the client

### 4. Error Handling
Custom error pages for:
- 400 Bad Request
- 403 Forbidden
- 404 Not Found
- 405 Method Not Allowed
- 500 Internal Server Error

  


