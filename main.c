#include "common.h"

/* init threadpoll, db, cache etc */
threadpool thpool;
void server_init()
{
    thpool = thpool_init(NR_THREADS);
    if (!thpool)
        err_quit("threadpool init failed");

    signal(SIGPIPE, SIG_IGN);

#ifdef USEDB
    if (sqlinit(ODBCARG) == -1)
        err_quit("can't initialize odbc DB");
#endif
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

/* nasty cast warning =) */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
        thpool_add_work(thpool, &proxy_start, (void*)connfd);
#pragma GCC diagnostic pop
    }

    return 0;
}
