#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/wait.h> // for waitpid()
#include <signal.h>   // for signal()

#define PORT 8080

void handle_request(int sock);

void sigchld_handler(int sig)
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char const *argv[])
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // --- 注册信号处理器 ---
    // 当子进程状态改变(如退出)时，调用 sigchld_handler
    signal(SIGCHLD, sigchld_handler);

    // --- Socket创建、绑定、监听部分和阶段1完全一样 ---
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed"); exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt"); exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed"); exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen"); exit(EXIT_FAILURE);
    }

    printf("--- Server is listening on port %d (Multi-Process Mode) ---\n", PORT);

    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue; 
        }

        pid_t pid = fork();

        if (pid < 0)
        {
            perror("fork");
            close(new_socket);
            continue;   
        }

        if (pid == 0)
        {
            // 子进程
            close(server_fd);

            handle_request(new_socket);

            close(new_socket);

            printf("Child process PID: %d finished.\n", getpid());
            exit(0);
        }
        else
        {
            // 父
            close(new_socket);

            // 直接关闭，等待下一个连接
        }
    }

    close(server_fd);
    return 0;
}

void handle_request(int sock)
{
    printf("Child process PID: %d is handling a request.\n", getpid());
    
    char buffer[2048] = {0};
    char method[16], uri[256];
    
    read(sock, buffer, 2048);
    sscanf(buffer, "%s %s", method, uri);
    printf("PID: %d, Method: %s, URI: %s\n", getpid(), method, uri);

    // CGI 处理逻辑
    if (strncmp(uri, "/cgi-bin/", 9) == 0) // 确保是CGI请求
    { 
        char cgi_path[512];
        sprintf(cgi_path, ".%s", uri);

        // HTTP 状态行
        char *status_line = "HTTP/1.1 200 OK\n";
        write(sock, status_line, strlen(status_line));

        // stdout  ->  客户端 socket
        if (dup2(sock, STDOUT_FILENO) == -1)
        {
            perror("dup2 failed");
            exit(EXIT_FAILURE);
        }

        // execl 执行 CGI 程序
        if (execl(cgi_path, cgi_path, NULL) < 0)
        {
            perror("execl failed");
            exit(EXIT_FAILURE);
        }

    }
    else 
    {
        //  处理静态文件
        if (strcmp(method, "GET") != 0) 
        { 
            char *response = "HTTP/1.1 501 Not Implemented\n\n";
            write(sock, response, strlen(response));
            return; 
        }
        char* request_uri = uri;
        if (strcmp(request_uri, "/") == 0) { request_uri = "/index.html"; }
        
        char filepath[512];
        sprintf(filepath, "htdocs%s", request_uri);

        FILE *file = fopen(filepath, "r");
        if (file == NULL) 
        {
            printf("PID: %d, File not found: %s\n", getpid(), filepath);
            char *response = "HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n<h1>404 Not Found</h1>";
            write(sock, response, strlen(response));
        } 
        else 
        {
            printf("PID: %d, Serving file: %s\n", getpid(), filepath);
            struct stat st;
            stat(filepath, &st);
            long file_size = st.st_size;
            char *file_content = malloc(file_size);
            fread(file_content, 1, file_size, file);
            fclose(file);
            char response_header[256];
            sprintf(response_header, "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: %ld\n\n", file_size);
            write(sock, response_header, strlen(response_header));
            write(sock, file_content, file_size);
            free(file_content);
        }
    }
}
