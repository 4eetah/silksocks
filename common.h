#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <pthread.h>

#include "error.h"
#include "thpool.h"

#define PROG "silksocks"
#define LISTENQ 8192
#define NR_THREADS 1024
#define NEEDAUTH 1
#define BUFSIZE (1<<20)
#define DNSTBL_SIZE (1<<23)
#define DNS_TTL 300 // sec

#define SLEEPTIME 1000

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

#define sockADDR(sa) (((struct sockaddr*)sa)->sa_family == AF_INET6 ? \
        ((unsigned char*)&((struct sockaddr_in6*)sa)->sin6_addr) : \
        ((unsigned char*)&((struct sockaddr_in*)sa)->sin_addr))

#define sockPORT(sa) (((struct sockaddr*)sa)->sa_family == AF_INET6 ? \
        (&((struct sockaddr_in6*)sa)->sin6_port) : \
        (&((struct sockaddr_in*)sa)->sin_port))

#define sockADDRLEN(sa) (((struct sockaddr*)sa)->sa_family == AF_INET6 ? 16 : 4)

#define sockSIZEOF(sa) (((struct sockaddr*)sa)->sa_family == AF_INET6 ? \
        sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in)

#define sockFAMILY(sa) (((struct sockaddr*)sa)->sa_family)

typedef enum {
    BYTE_S,
    BYTE_L,
    STRING_S,
    STRING_L,
    CONNECT_S,
    CONNECT_L,
    LEN,
} timeout;

extern size_t timeo[LEN];

struct socks5_cli {
    struct sockaddr_storage request;
    unsigned char user[256], passwd[256];
    int clientfd, proxyfd;
    struct sockaddr_in clipeer, srvpeer;
    uint8_t cmd;
};

struct hash_entry {
    unsigned long hash;
    time_t expires;
    struct hash_entry *next;
    unsigned char value[4];
};

struct hash_table {
    pthread_mutex_t hmutex;
    size_t size;
    size_t record_size;
    struct hash_entry **table;
    void *entries;
    struct hash_entry *empty_entry;
};

extern struct hash_table dns_table;
extern struct hash_table dns6_table;

struct ip2creds {
    unsigned char *user;
    unsigned char *passwd;
};

int negotiate(int clientfd, int proxyfd);
struct addrinfo *host_serv(const char *host, const char *serv, int family, int socktype);
int connect_nonb(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec, int nusec);
int setrcvtimeo(int fd, long sec, long usec);
int setsndtimeo(int fd, long sec, long usec);
ssize_t readn_timeo(int fd, void *vptr, size_t n, long sec, long usec);
ssize_t writen_timeo(int fd, const void *vptr, size_t n, long sec, long usec);
int tcp_listen(const char *host, const char *serv, socklen_t *addrlenp);
char *gf_time();
void proxy_start(void *arg);
int sqlinit(char *s);
void sqlclose();
char *sqlget_apppasswd(unsigned char *appuser);
int sqlget_proxycreds(unsigned char **puser, unsigned char **ppasswd, unsigned int ip, unsigned short port, unsigned int status);
int cache_putip(uint32_t key, unsigned char *user, unsigned char *passwd);
struct ip2creds *cache_getip(uint32_t key);
void cache_freeip();
int cache_putapp(unsigned char *app, unsigned char *passwd);
char *cache_getapp(unsigned char *app);
void cache_freeapp();
int hashtbl_init(struct hash_table *hashtbl, size_t size, size_t record_size);
int hashtbl_put(struct hash_table *hashtbl, unsigned char *key, unsigned char *val, time_t expires);
int hashtbl_get(struct hash_table *hashtbl, unsigned char *key, unsigned char *val);
int hashtbl_check(struct hash_table *hashtbl, unsigned char *key);
const char *syslog_lvl2str(int level);
const char *addr2logbuf(struct socks5_cli *cli);

#endif // COMMON_H
