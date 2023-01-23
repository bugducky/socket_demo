#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>

#define BUFFER_SIZE 64

const char* ip = "127.0.0.1";
const int port = 8818;

// 聊天室客户端程序

int main(int argc, char *argv[]){
    // if(argc !=2){
    //     fputs("usage: ./client message\n", stderr);
    //     exit(1);
    // }
    // 创建服务器端socket地址
    // 本机采用匿名socket地址，由系统自动分配
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    serv_addr.sin_port = htons(port);
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock_fd>0);

    if( connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        printf("connection failed\n");
        close(sock_fd);
        return 1;
    }

    struct pollfd fds[2];
    // 注册0（标准输入）和sockfd上的可读事件
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    fds[1].fd = sock_fd;
    // POLLRDHUP tcp连接被对方关闭、或对方关闭了写操作
    fds[1].events = POLLIN | POLLHUP;

    char read_buf[BUFFER_SIZE];
    int pipefd[2];
    int ret = pipe(pipefd);
    assert(ret!=-1);

    while(1){
        ret = poll(fds, 2, -1);
        if (ret < 0){
            printf("poll failed\n");
            break;
        }
        if (fds[1].revents & POLLHUP) {
            printf("server close connection.\n");
            break;
        }
        else if(fds[1].revents & POLLIN) {
            memset(read_buf, '\0', BUFFER_SIZE);
            recv(fds[1].fd, read_buf, BUFFER_SIZE-1, 0);
            printf("%s\n", read_buf);
        }

        if (fds[0].revents & POLLIN) {
            ret = splice(0, NULL, pipefd[1], NULL, 32768,
                            SPLICE_F_MORE | SPLICE_F_MOVE);
            ret = splice(pipefd[0], NULL, sock_fd, NULL, 32768,
                            SPLICE_F_MORE | SPLICE_F_MOVE);
        }
    }
    close(sock_fd);
    
    return 0;
}