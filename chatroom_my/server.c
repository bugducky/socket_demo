#define _GNU_SOURCE 1
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define USER_LIMIT 5
#define BUFFER_SIZE 64
#define FD_LIMIT 65535

// ipv4地址和监听端口号
const char *ip = "127.0.0.1";
const int port = 8818;

struct client_data
{
  struct sockaddr_in address;
  char *write_buf;
  char buf[BUFFER_SIZE];
};

// 将文件描述符设置为非阻塞
int setnoblocking(int fd)
{
  int old_opt = fcntl(fd, F_GETFL);
  int new_opt = old_opt | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_opt);
  return old_opt;
}

int main(int argc, char **argv)
{
  // if(argc<=2) {
  //     printf("usage: ip:%s\n", basename(argv[0]));
  //     return 1;
  // }
  // const char* ip = argv[1];
  // int port = atoi(argv[2]);

  int ret = 0;
  struct sockaddr_in address;
  bzero(&address, sizeof(address));
  address.sin_family = AF_INET;
  // 将点分十位的ip地址转换为网络字节序
  inet_pton(AF_INET, ip, &address.sin_addr);
  address.sin_port = htons(port);

  printf("server ip: %s port: %d \n", ip, port);
  // 创建socket socket(协议族，传输模式：流式传世或数据报，协议：一般为0)
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  printf("listen fd created\n");
  assert(listenfd >= 0);
  // 将ip地址绑定到socketfd
  ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
  assert(ret != -1);
  // 创建监听队列，监听客户端的连接，USER_LIMIT指定了监听队列长度
  ret = listen(listenfd, USER_LIMIT);
  assert(ret != -1);

  // 创建客户端资源
  struct client_data *users = (struct client_data *)malloc(sizeof(struct client_data) * FD_LIMIT);

  
  struct pollfd fds[USER_LIMIT + 1];
  int user_counter = 0;
  for (int i = 1; i <= USER_LIMIT; ++i)
  {
    fds[i].fd = -1;
    fds[i].events = 0;
  }
  fds[0].fd = listenfd;
  fds[0].events = POLLIN | POLLERR;
  fds[0].revents = 0;

  while (1)
  {
    ret = poll(fds, user_counter + 1, -1);

    if (ret < 0)
    {
      printf("poll failed\n");
      break;
    }

    for (int i = 0; i < user_counter + 1; ++i)
    {
      // 服务端socket监听到连接，分配客户端资源
      if ((fds[i].fd == listenfd) && (fds[i].revents & POLLIN))
      {
        struct sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);
        int connfd = accept(listenfd, (struct sockaddr *)&client_address,
                            &client_address_length);
        if (connfd < 0)
        {
          printf("errno: %d", errno);
          continue;
        }
        // 如果用户超出限制，
        if (user_counter >= USER_LIMIT)
        {
          const char *info = "too many users.\n";
          printf("%s", info);
          send(connfd, info, strlen(info), 0);
          close(connfd);
          continue;
        }
        // 接受新的连接
        user_counter++;
        users[connfd].address = client_address;
        setnoblocking(connfd);
        fds[user_counter].fd = connfd;
        fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;
        fds[user_counter].revents = 0;
        printf("comes a new user, welcome!\nnow we have %d users\n", user_counter);
      }
      else if (fds[i].revents & POLLERR)
      {
        printf("get an err from %d\n", fds[i].fd);
        char errors[100];
        memset(errors, '\0', 100);
        socklen_t length = sizeof(errors);
        if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &length) < 0)
        {
          printf("get socket option failed\n");
        }
        continue;
      }
      else if (fds[i].revents & POLLRDHUP)
      {
        users[fds[i].fd] = users[fds[user_counter].fd];
        close(fds[i].fd);
        fds[i] = fds[user_counter];
        i--;
        user_counter--;
        printf("a client left\n");
      }
      else if (fds[i].revents & POLLIN)
      { // 处理客户端事件
        int connfd = fds[i].fd;
        memset(users[connfd].buf, '\0', BUFFER_SIZE);
        ret = recv(connfd, users[connfd].buf, BUFFER_SIZE - 1, 0);
        printf("get %d bytes of client data [%s from %d \n", ret,
               users[connfd].buf, connfd);
        if (ret < 0)
        {
          if (errno != EAGAIN)
          {
            close(connfd);
            users[fds[i].fd] = users[fds[user_counter].fd];
            fds[i] = fds[user_counter];
            i--;
            user_counter--;
          }
        }
        else if (ret == 0)
        {
          printf("空数据");
          // 空数据
          // fds[i].events |= ~POLLOUT;
          // fds[i].events |= POLLIN;
        }
        else
        {
          // 向其他客户端发送接收的数据
          for (int j = 1; j <= user_counter; ++j)
          {
            if (fds[j].fd == connfd)
            {
              continue;
            }
            fds[j].events |= ~POLLIN;
            fds[j].events |= POLLOUT;
            users[fds[j].fd].write_buf = users[connfd].buf;
          }
        }
      }
      else if (fds[i].revents & POLLOUT)
      {
        int connfd = fds[i].fd;
        if (!users[connfd].write_buf)
        {
          continue;
        }
        // printf("send msg: %s\n", users[connfd].write_buf);
        ret = send(connfd, users[connfd].write_buf,
                   strlen(users[connfd].write_buf), 0);

        // 写完数据后需要重新注册可读事件
        users[connfd].write_buf = NULL;
        fds[i].events |= ~POLLOUT;
        fds[i].events |= POLLIN;
      }
    }
  }

  free(users);
  close(listenfd);
  return 0;
}