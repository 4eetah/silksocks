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

#include "error.h"
#include "thpool.h"

#define LISTENQ 1024
#define NR_THREADS 100
#define NEEDAUTH 1
#define BUFSIZE 8192

#define ODBCARG "megaindexodbc,root,den4ik"

#define max(a, b) (((a) > (b)) ? (a) : (b))

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
    LEN
} timeout;

extern size_t timeo[LEN];

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
char *sqlget_apppasswd(char *appuser);
int sqlget_proxycreds(unsigned char **puser, unsigned char **ppasswd, unsigned int ip, unsigned short port, unsigned int status);

#endif // COMMON_H
