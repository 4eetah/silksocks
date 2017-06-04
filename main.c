#include "common.h"

void daemon_init(const char *pname, int facility)
{
    int i;
    pid_t pid;

    if ((pid = fork()) < 0) {
        perror("fork1");
        exit(1);
    }
    if (pid != 0)
        exit(0);

    setsid();
    signal(SIGHUP, SIG_IGN);

    if ((pid = fork()) < 0) {
        perror("fork2");
        exit(1);
    }
    if (pid != 0)
        exit(0);

    chdir("/");
    umask(0);
    
    for (i = 0; i < 65535; ++i)
        close(i);

    daemon_proc = 1; // switch to syslog error.c
    openlog(pname, LOG_PID, facility);
}

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

void usage(char *prog) {
    err_quit("Usage: %s [-d] socks.cfg\n\t-d daemonize process", prog);
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clilen;
    struct sockaddr_storage cliaddr;
    const char *host, *service;
    int opt;
    int daemon = 0;

    while ((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
        case 'd':
            daemon = 1;
            break;
        default:
            usage(argv[0]);
        }
    }
    if (argc - optind > 1)
        usage(argv[0]);
    sql_config = argv[optind];

    server_init();

    if (daemon)
        daemon_init(argv[0], 0);

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
