#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define MAXLINE 256
#define SERV_PORT 6666

int main()
{
    /*
    // socket地址结构体
    struct sockaddr_in
    {
        sa_family_t sin_family; // 地址族：AF_inet
        u_int16_t sin_port;     // 网络字节序（大端）表示的端口号
        struct in_addr sin_addr;// ipv4地址结构体（32bit）
    };
    */
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cliaddr_len;
    // 监听、连接 文件描述符
    int listenfd, connfd;
    // 创建一个I/O缓冲区
    char buff[MAXLINE];
    // char str[INET_ADDRSTRLEN];
    // int i, n;

    // socket(底层协议族，服务类型，具体的协议)
    // 底层协议族：AF_INET,PF_INET,PF_INET6,PF_UNIX
    // 服务类型：SOCK_STREAM(TCP:流服务)，SOCK_UGRAM(UDP:数据包服务)
    // 具体的协议，一般选0，表示默认的协议
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == listenfd)
    {
        printf("Create socket error(%d): %s\n", errno, strerror(errno));
        return -1;
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    // htonl： host to net (l：长地址)，一般表示ip地址
    // INADDR_ANY 监听0.0.0.0地址，即监听所有地址
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // htons： host to net (s：短地址)，端口号
    servaddr.sin_port = htons(SERV_PORT);
    // bind(socket文件描述符，socket地址，socket地址长度)
    if (-1 == bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)))
    {
        printf("Bind error(%d): %s\n", errno, strerror(errno));
        return -1;
    }
    // listen(socket文件描述符，监听队列的最大长度)
    if (-1 == listen(listenfd, 2000))
    {
        printf("Listen error(%d): %s\n", errno, strerror(errno));
        return -1;
    }
    // 等待连接
    printf("Accepting connections ... \n");
    while (1)
    {
        cliaddr_len = sizeof(cliaddr);
        // 接受连接，绑定到连接文件描述符
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
        if (-1 == connfd)
        {
            printf("accept error(%d): %s\n", errno, strerror(errno));
            return -1;
        }
        
        // 读取socket上的数据
        bzero(buff, MAXLINE);
        recv(connfd, buff, MAXLINE - 1, 0);
        printf("%s \n", buff);
        // sleep(1);
        // 往socket上写数据
        send(connfd, buff, strlen(buff), 0);
        close(connfd);
    }
    return 0;
}