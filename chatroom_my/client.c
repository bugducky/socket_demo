#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
    // 本机socket地址由系统自动分配
    // 创建服务器端socket地址
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    serv_addr.sin_port = htons(port);
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock_fd>=0);

    if(connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
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
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents = 0;

    char read_buf[BUFFER_SIZE];

    // 创建管道，用于收发消息
    int pipefd[2];
    int ret = pipe(pipefd);
    assert(ret!=-1);

    while(1){
        // 轮询用户的IO和网络IO
        // timeout=-1 表示poll将阻塞，直到内核通知事件发生
        ret = poll(fds, 2, -1);
        if (ret < 0){
            printf("poll failed\n");
            break;
        }
        // 内核返回POLLHUP事件，提示服务器关闭，程序结束
        if (fds[1].revents & POLLRDHUP) {
            printf("server close connection.\n");
            break;
        } 
        // 内核返回网络中的POLLIN事件，消息到写read_buf
        else if(fds[1].revents & POLLIN) {
            memset(read_buf, '\0', BUFFER_SIZE);
            recv(fds[1].fd, read_buf, BUFFER_SIZE-1, 0);
            printf("%s\n", read_buf);
        }
        // 内核返回标准输入的POLLIN事件
        if (fds[0].revents & POLLIN) {
            // 通过零拷贝输出到管道中，pipefd[0]只读数据，pipefd[1]只写数据
            // 管道同时读写需要创建两个管道
            ret = splice(0, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
            // 从pipefd[0]读数据，写入sock_fd中，即发往服务端程序
            ret = splice(pipefd[0], NULL, sock_fd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        }
    }
    close(sock_fd);
    return 0;
}