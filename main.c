#include "common.h"

/* init threadpoll, db, cache etc */
void server_init()
{
    //threadpool thpool;
    //thpool = thpool_init(NR_THREADS);
    //if (!thpool)
    //    err_sys("threadpool init failed");

}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clilen;
    struct sockaddr_storage cliaddr;

    server_init();
    listenfd = tcp_listen(NULL, SRV_PORT, NULL);
    
    for (;;) {
        clilen = sizeof(cliaddr);
        if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) == -1) {
            if (errno == EINTR)
                continue;
            else
                err_sys("accept");
        }
        if (setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, &timer, sizeof(timer)) == -1) {
            err_ret("setsockopt(%d, SO_RCVTIMEO)", connfd);
            close(connfd);
            continue;
        }

        // run new thread from threadpoll here
        socks5_run(connfd);

    }

    return 0;
}
