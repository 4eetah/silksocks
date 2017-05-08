#include "common.h"

/* init threadpoll, db, cache etc */
threadpool thpool;
void server_init()
{
    thpool = thpool_init(NR_THREADS);
    if (!thpool)
        err_quit("threadpool init failed");

    signal(SIGPIPE, SIG_IGN);
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clilen;
    struct sockaddr_storage cliaddr;

    server_init();
    listenfd = tcp_listen(NULL, "1080", NULL);
    
    for (;;) {
        clilen = sizeof(cliaddr);
        if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) == -1) {
            if (errno == EINTR)
                continue;
            else
                err_ret("accept");
        }

        thpool_add_work(thpool, &proxy_start, (void*)connfd);
    }

    return 0;
}
