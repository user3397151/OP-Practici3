#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 2048
#define MAX_PARAM_LENGTH 256
#define MAX_CLIENTS_QUEUE 5

void url_decode(char* output, const char* input) {
    char hex_high, hex_low;
    while (*input) {
        if (*input == '%' && 
            (hex_high = input[1]) && 
            (hex_low = input[2]) && 
            isxdigit(hex_high) && 
            isxdigit(hex_low)) {
            
            hex_high = hex_high >= 'a' ? (hex_high - 'a' + 10) : 
                     (hex_high >= 'A' ? (hex_high - 'A' + 10) : (hex_high - '0'));
            hex_low = hex_low >= 'a' ? (hex_low - 'a' + 10) :
                    (hex_low >= 'A' ? (hex_low - 'A' + 10) : (hex_low - '0'));
            
            *output++ = (hex_high << 4) | hex_low;
            input += 3;
        } else {
            *output++ = *input++;
        }
    }
    *output = '\0';
}

void handle_client(int client_fd) {
    char request[BUFFER_SIZE] = {0};
    ssize_t bytes_read = read(client_fd, request, BUFFER_SIZE - 1);
    
    if (bytes_read <= 0) {
        perror("Read error");
        close(client_fd);
        return;
    }

    char* param_start = strstr(request, "message=");
    char encoded_param[MAX_PARAM_LENGTH] = {0};
    char decoded_message[MAX_PARAM_LENGTH] = {0};

    if (param_start) {
        sscanf(param_start + 8, "%255[^& \n]", encoded_param);
        url_decode(decoded_message, encoded_param);
    }

    const char* html_template = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n\r\n"
        "<!DOCTYPE html><html>"
        "<head><title>Message Server</title><style>"
        "body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }"
        "</style></head>"
        "<body>"
        "<h2>Получено сообщение:</h2>"
        "<div style='border:1px solid #ccc; padding:20px; margin:20px auto; width:50%%;'>%s</div>"
        "<img src='https://www.mirea.ru/upload/medialibrary/c1a/MIREA_Gerb_Colour.jpg' width='200' alt='MIREA Logo'>"
        "</body></html>";
    
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, html_template, decoded_message);
    write(client_fd, response, strlen(response));
    close(client_fd);
}

void run_http_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS_QUEUE) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("HTTP server running on port %d\n", SERVER_PORT);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }
        handle_client(client_fd);
    }
}

int main() {
    run_http_server();
    return 0;
}
