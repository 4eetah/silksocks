#include "common.h"

/* init threadpoll, db, cache etc */
threadpool thpool;
char *sql_config;
void server_init()
{
    thpool = thpool_init(NR_THREADS);
    if (!thpool)
        err_quit("threadpool init failed");

    signal(SIGPIPE, SIG_IGN);

#ifdef USEDB
    char line[256];
    FILE *f = fopen(sql_config, "r");
    if (!f) {
        err_quit("The project was compiled with -DUSEDB option,\n"
                "Pass the config.txt file as an argument in the format:\n"
                "odbc_srcname,dbuser,dbpasswd");
    }
    
    if (fgets(line, sizeof(line), f) == NULL) {
        err_quit("Error reading %s\n", sql_config);
    }
    if (line[strlen(line)-1] == '\n')
        line[strlen(line)-1] = '\0';

    if (sqlinit(line) == -1)
        err_quit("can't initialize odbc DB");

    printf("ODBC: %s\n", line);
#endif
    
    if (hashtbl_init(&dns_table, DNSTBL_SIZE, 4) == -1)
        err_quit("Failed to initialize dns_table");

    /* no dns6 cache for now 
    if (hashtbl_init(&dns6_table, DNSTBL_SIZE, 16) == -1)
        err_quit("Failed to initialize dns6_table");
    */
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clilen;
    struct sockaddr_storage cliaddr;
    const char *host, *service;

    if (argc == 2)
        sql_config = argv[1];

    server_init();

    host = NULL;
    service = "1080";
    listenfd = tcp_listen(host, service, NULL);

    printf("Listening on:\t%s:%s\t%lu/%s\n", host ? host : "*", service, (unsigned long)getpid(), argv[0]);
    
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
