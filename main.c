#include "common.h"

void daemon_init() {
    if (fork() > 0) {
        exit(0);
    }
    setsid();
    daemon_proc = 1;
    openlog(PROG, LOG_PID, 0);
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
    err_quit("Usage: %s [-Ddvh] socks.cfg\n"
            "\t-D\tdaemonize process\n"
            "\t-d\tenables debug output\n"
            "\t-v\tenables verbose output\n"
            "\t-h\tprint this help and exit\n"
            "\nlogging:\n"
            "\t-v\terror\n"
            "\t-vv\twarning\n"
            "\t-vvv\tnotice\n"
            "\t-vvvv\tinfo\n"
            "\ndebug:\nthe program should be compiled with -DDEBUG\n"
            "\t-d\t0 level\n"
            "\t-dd\t1 level\n"
            "\t-ddd\t2 level\n"
            , prog);
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clilen;
    struct sockaddr_storage cliaddr;
    const char *host, *service;
    int opt;
    int daemon = 0;
    int debug = -1, verbose = LOG_CRIT;

    while ((opt = getopt(argc, argv, "Ddvh")) != -1) {
        switch (opt) {
        case 'D':
            daemon = 1;
            break;
        case 'd':
            if (debug < 2)
                debug++;
            break;
        case 'v':
            if (verbose < LOG_INFO)
                verbose++;
            break;
        case 'h':
        default:
            usage(argv[0]);
        }
    }

    silk_log_level = verbose;
    silk_debug_level = debug;

    if (argc - optind > 1)
        usage(argv[0]);
    sql_config = argv[optind];

    if (daemon)
        daemon_init();

    server_init();

    host = NULL;
    service = "1080";
    listenfd = tcp_listen(host, service, NULL);

    printf("Listening on:\t%s:%s\t%lu/%s\n", host ? host : "*", service, (unsigned long)getpid(), argv[0]);
    printf("verbosity level: %s\ndebug level: (%s) %d level\n\n", syslog_lvl2str(verbose), debug == -1 ? "disabled" : "enabled", debug);

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
