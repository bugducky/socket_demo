
#include "multiplexing.h"

MyEpoll::MyEpoll(int _max, int _listenfd) : Multiplexing(_max, _listenfd)
{
    this->events = new Epoll_event[this->io_max];
    this->epollfd = epoll_create(this->io_max);
}

void MyEpoll::server_do_multiplexing()
{
    int i, fd;
    int nready;
    char buf[MAXLINE];
    memset(buf, 0, MAXLINE);

    // 添加监听描述符事件
    if (!add_event(this->listenfd, EPOLLIN))
    {
        perror("add event error.");
        exit(1);
    }

    while (1)
    {
        nready = wait_event();
        this->nready = nready;
        if (-1 == nready)
            return;
        if (0 == nready)
            continue;

        // 进行遍历
        /**这里和poll、select都不同，因为并不能直接判断监听的事件是否产生，
        所以需要一个for循环遍历，
        这个for循环+判断类似于poll中
        if (FD_ISSET(this->listenfd, this->allfds))、
        select中的if (this->clientfds[0].revents & POLLIN)
        这里只是尽量写的跟poll、select中的结构类似，
        但是实际代码中，不应该这么写，这么写多加了一个for循环**/

        for (int i = 0; i < nready; i++)
        {
            fd = events[i].data.fd;
            if ((fd == this->listenfd) && (events[i].events & EPOLLIN))
            {
                if (!accept_client_proc())
                {
                    continue;
                }
                if (--nready <= 0)
                {
                    continue;
                }
            }
            handle_client_msg();
        }
    }
    close(epollfd);
}

bool MyEpoll::accept_client_proc()
{
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen;
    cliaddrlen = sizeof(cliaddr);
    int connfd;
    // 接受新的连接
    if ((connfd = accept(
        this->listenfd, 
        (struct sockaddr *)&cliaddr, 
        &cliaddrlen)) == -1) {
            if (errno == EINTR)
                return false;
            else
            {
                perror("accept error:.\n");
                exit(1);
            }
        }
        fprintf(stdout, "accpet a new client: %s:%d\n",
                inet_ntoa(cliaddr.sin_addr),  cliaddr.sin_port);

    return add_event(connfd, EPOLLIN);
}

void MyEpoll::client_do_multiplexing()
{
    char sendline[MAXLINE], recvline[MAXLINE];
    int n;
    int nready = -1;
    int fd;
    if (this->io_max < 2)
    {
        perror("please increase the max number of io.");
        exit(1);
    }

    if (!add_event(this->listenfd, EPOLLIN))
    {
        perror("add event error");
        exit(1);
    }

    if (!add_event(STDIN_FILENO, EPOLLIN)){
        perror("add event error.\n");
        exit(1);
    }

    while (1)
    {
        nready = wait_event();
        if (-1 == nready)
            return;
        if (0 == nready)
            return;
        for (int i = 0; i < nready; i++)
        {
            fd = events[i].data.fd;
            if ((fd == this->listenfd) && (events[i].events & EPOLLIN))
            {
                n = read(this->listenfd, recvline, MAXLINE);
                if (n <= 0)
                {
                    fprintf(stderr, "client: server is closed.\n");
                    close(this->listenfd);
                    return;
                }
                write(STDOUT_FILENO, recvline, n);
            }
            else
            {
                n = read(STDIN_FILENO, sendline, MAXLINE);
                if (n <= 0)
                {
                    shutdown(this->listenfd, SHUT_WR);
                    continue;
                }
                write(this->listenfd, sendline, n);
            }
        }
    }
}

bool MyEpoll::add_event(int connfd, int event)
{
    Epoll_event ev;
    ev.events = event;
    ev.data.fd = connfd;
    return epoll_ctl(this->epollfd, EPOLL_CTL_ADD, connfd, &ev) ==0;
}

void MyEpoll::handle_client_msg()
{
    int fd;
    char buf[MAXLINE];
    memset(buf, 0, MAXLINE);
    for (int i = 0; i < this->nready; i++)
    {
        fd = this->events[i].data.fd;
        if (fd == this->listenfd)
            continue;
        if (events[i].events & EPOLLIN)
        {
            int n = read(fd, buf, MAXLINE);
            if (n <= 0)
            {
                perror("read error.");
                close(fd);
                delete_event(fd, EPOLLIN);
            }
            else
            {
                write(STDOUT_FILENO, buf, n);
                write(fd, buf, strlen(buf));
            }
        }
    }
}

int MyEpoll::wait_event() {
    int nready = epoll_wait(this->epollfd, this->events, this->io_max, INFTIM);
    if (nready==-1) fprintf(stderr, "poll error:%s.\n", strerror(errno));
        
    if(nready==0) fprintf(stdout, "epoll_wait timeout\n");
    return nready;
}


bool MyEpoll::delete_event(int fd, int state) {
    Epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    return epoll_ctl(this->epollfd, EPOLL_CTL_DEL, fd, &ev) == 0;
}