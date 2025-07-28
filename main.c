#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h> // 用于获取文件状态(大小)

#define PORT 8080

// 前置声明请求处理函数
void handle_request(int sock);

int main(int argc, char const *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // --- Socket创建、绑定、监听部分和阶段0完全一样 ---
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("--- Server is listening on port %d ---\n", PORT);

    // --- 核心改动：使用无限循环来持续接受连接 ---
    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            // 如果accept失败，不要退出，而是继续等待下一个连接
            continue; 
        }
        
        // 调用独立的函数来处理这个连接
        handle_request(new_socket);

        // 处理完后，关闭这个客户端的socket
        close(new_socket);
    }

    close(server_fd); // 理论上不会执行到这里
    return 0;
}

/**
 * @brief 处理单个HTTP请求
 * @param sock 连接的客户端socket
 */
void handle_request(int sock) {
    char buffer[2048] = {0};
    char method[16], uri[256];
    char filepath[512];
    
    // 读取HTTP请求
    read(sock, buffer, 2048);
    
    // 1. 解析请求方法和URI (这是一个简化的解析，仅用于实验)
    sscanf(buffer, "%s %s", method, uri);
    printf("Received request: Method=%s, URI=%s\n", method, uri);

    // 我们只关心GET请求
    if (strcmp(method, "GET") != 0) {
        char *response = "HTTP/1.1 501 Not Implemented\n\n";
        write(sock, response, strlen(response));
        return;
    }

    // 如果URI是根目录"/"，我们约定它指向index.html
    char* request_uri = uri;
    if (strcmp(request_uri, "/") == 0) {
        request_uri = "/index.html";
    }

    // 2. 构建本地文件路径
    sprintf(filepath, "htdocs%s", request_uri);

    // 3. 检查文件是否存在
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        // 文件不存在，返回404 Not Found
        printf("File not found: %s\n", filepath);
        char *response = "HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n<h1>404 Not Found</h1><p>The requested file could not be found.</p>";
        write(sock, response, strlen(response));
    } else {
        // 文件存在，准备发送文件内容
        printf("Serving file: %s\n", filepath);
        
        // a. 获取文件大小，用于设置Content-Length头部
        struct stat st;
        stat(filepath, &st);
        long file_size = st.st_size;

        // b. 读取文件内容到内存
        char *file_content = malloc(file_size);
        fread(file_content, 1, file_size, file);
        fclose(file);

        // c. 构建并发送HTTP响应头
        char response_header[256];
        sprintf(response_header, "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: %ld\n\n", file_size);
        write(sock, response_header, strlen(response_header));
        
        // d. 发送文件内容
        write(sock, file_content, file_size);
        
        // e. 释放内存
        free(file_content);
    }
}