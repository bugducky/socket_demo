#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 256
#define SERV_PORT 6666

int main(int argc, char *argv[]){
    // 创建服务器端socket地址
    // 本机采用匿名socket地址，由系统自动分配
    struct sockaddr_in servaddr;
    char buff[MAXLINE];
    int sockfd,n;
    char *str;
    if(argc !=2){
        fputs("usage: ./client message\n", stderr);
        exit(1);
    }
    str=argv[1];
    printf("send msg: %s \n", str);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    // 该函数将点分十进制地址转换为二进制
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
    // 设置服务端的端口号
    servaddr.sin_port = htons(SERV_PORT);

    connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    send(sockfd,str,strlen(str),0);
    bzero(buff, MAXLINE);
    recv(sockfd, buff, MAXLINE-1, 0);
    printf("Response from server %s\n", buff);
    // 关闭连接
    close(sockfd);
    return 0;
}