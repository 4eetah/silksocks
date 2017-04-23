#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include "thpool.h"

#define SRV_PORT 1080
#define LISTENQ 1024
#define MAX_EVENTS 10
#define MAX_LINES 4096

#define DEBUG 1
#ifdef DEBUG
#define do_debug(...) err_msg(__VA_ARGS__)
#else
#define do_debug(...)
#endif

void str_echo(int fd);
void do_accept(int fd);
int set_nonblock(int fd);
int bind_and_listen();

/* epoll fd handle */
static int epollfd;

void str_echo(int fd)
{
    ssize_t n;
    char buf[MAX_LINES];

again:
    while ((n = recv(fd, buf, MAX_LINES, 0)) > 0)
        if (send(fd, buf, n, 0) == -1)
            break;
    if (n < 0 && errno == EINTR)
        goto again;
    else if (n < 0 && errno == EAGAIN)
        return;
    else if (n == 0) {
        do_debug("client fd(%d) closed connection", fd);
        close(fd);
        return;
    }

    err_sys("str_echo failed");
}

void do_accept(int listen_fd)
{
    int client_fd;
    struct sockaddr_in addr;
    int addrlen;
    struct epoll_event ev;

    while (1) {

        addrlen = sizeof(addr);
        client_fd = accept(listen_fd, (struct sockaddr*)&addr, &addrlen);
        if (client_fd == -1) {
            if (errno == EAGAIN)
                break; /* proceeded with all clients */
            else
                err_ret("accept on new connection failed");
        }
        do_debug("new client fd(%d) %s", client_fd, inet_ntoa(addr.sin_addr));

        set_nonblock(client_fd);
        ev.data.fd = client_fd;
        ev.events = EPOLLIN;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
            err_sys("do_accept: epoll_ctl_add failed");
    }
}

int set_nonblock(int fd)
{    
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1)
        return -1;

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        return -1;

    return 0; 
}

int bind_and_listen()
{
    struct sockaddr_in sin;
    int listen_fd;

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        err_sys("failed to create listen fd");

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(SRV_PORT);

    if (bind(listen_fd, (struct sockaddr*)&sin, sizeof(sin)) == -1)
        err_sys("failed to bind listen fd");

    if (listen(listen_fd, LISTENQ) == -1)
        err_sys("failed to listen on listen fd");

    return listen_fd;
}

int main(int argc, char **argv)
{

    int ready, i;
    int nr_cpu_online;
    threadpool thpool;
    int listen_fd;
    struct epoll_event ev, evlist[MAX_EVENTS];

    nr_cpu_online = sysconf(_SC_NPROCESSORS_ONLN);
    thpool = thpool_init(nr_cpu_online+1);
    if (!thpool)
        err_sys("threadpool init failed");

    listen_fd = bind_and_listen();
    set_nonblock(listen_fd);

    epollfd = epoll_create1(0);
    if (epollfd == -1)
        err_sys("epoll_create1");

    ev.events = EPOLLIN; /* EPOLLHUP | EPOLLERR */
    ev.data.fd = listen_fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_fd, &ev) == -1)
        err_sys("epoll_ctl: listen_fd");

    while (1) {
        
        do_debug("About to epoll_wait");

        ready = epoll_wait(epollfd, evlist, MAX_EVENTS, -1);
        if (ready == -1) {
            if (errno == EINTR)
                continue; /* restart if interrupted by signal */
            else
                err_sys("epoll_wait failed");
        }

        do_debug("Ready %d", ready);

        for (i = 0; i < ready; ++i) {
            
            do_debug("  fd=%d; events: %s%s%s", evlist[i].data.fd,
                    (evlist[i].events & EPOLLIN) ? "EPOLLIN " : "",
                    (evlist[i].events & EPOLLHUP) ? "EPOLLHUP " : "",
                    (evlist[i].events & EPOLLERR) ? "EPOLLERR " : "");

            if (evlist[i].data.fd == listen_fd) {
                do_accept(listen_fd);
            } else if (evlist[i].events & EPOLLIN) {
                str_echo(evlist[i].data.fd);
            } else if (evlist[i].events & (EPOLLERR | EPOLLHUP)) {
                do_debug("closing fd %d", evlist[i].data.fd);
                if (close(evlist[i].data.fd) == -1)
                    err_sys("close failed");
            } else {
                err_msg("what can I do?");
            }
        }
    }
    
    free(evlist);
    close(listen_fd);

    return 0;
}
