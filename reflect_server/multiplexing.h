#pragma once 

#include <unistd.h>
#include <cstdio>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <arpa/inet.h>

const int MAXLINE = 1024;

class Multiplexing {
protected:
    // 默认最大文件描述符 
    static const int DEFAULT_IO_MAX = 10;
    static const int INFTIM = -1;
    int io_max;
    int listenfd;
public:
    Multiplexing() : io_max(DEFAULT_IO_MAX) { }

    Multiplexing(int _max, int _listenfd) : 
        io_max(_max), listenfd(_listenfd) { }
    
    virtual void server_do_multiplexing() = 0;
    virtual void client_do_multiplexing() = 0;
    virtual void handle_client_msg() = 0;
    virtual bool accept_client_proc() = 0;
    virtual bool add_event(int confd, int event) = 0;
    virtual int wait_event() = 0;
};

typedef struct epoll_event Epoll_event;
class MyEpoll : public Multiplexing {
private:
    int epollfd;
    Epoll_event *events;
    int nready;
public:
    MyEpoll() : Multiplexing(), events(nullptr), epollfd(-1) { }
    MyEpoll(int _max, int _listenfd);
    ~MyEpoll() {
        if(events){
            delete events;
            events = nullptr;
        }
    } 

    void server_do_multiplexing();
    void client_do_multiplexing();
    void handle_client_msg();
    bool accept_client_proc();
    bool add_event(int confd, int event);
    int wait_event();
    bool delete_event(int confd, int event);

};