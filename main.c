#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

// 定义服务器监听的端口
#define PORT 8080

int main(int argc, char const *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // 1. 创建Socket文件描述符
    // AF_INET: 指定使用IPv4协议
    // SOCK_STREAM: 指定使用TCP协议
    // 0: 使用IP协议
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed"); // 如果失败，打印错误信息
        exit(EXIT_FAILURE);     // 退出程序
    }

    // 3. 绑定Socket到IP地址和端口
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // 接受来自任何IP地址的连接
    address.sin_port = htons(PORT);       // 将端口号从主机字节序转换到网络字节序

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 4. 开始监听进入的连接
    // backlog设置为10，表示内核为此Socket维护的已完成连接队列的最大长度
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("--- Server is listening on port %d ---\n", PORT);

    // 5. 接受一个客户端连接，这是一个阻塞调用
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // 6. 向连接的客户端发送一个简单的HTTP响应
    char *hello_response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 13\n\nHello, world!";
    write(new_socket, hello_response, strlen(hello_response));
    printf("--- 'Hello, world!' response sent ---\n");

    // 7. 关闭与客户端的连接和服务器的监听Socket
    close(new_socket);
    close(server_fd);

    return 0;
}