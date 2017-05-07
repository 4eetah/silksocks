#include "socks.h"
#include "common.h"

/* Methods:
o  X'00' NO AUTHENTICATION REQUIRED
o  X'01' GSSAPI
o  X'02' USERNAME/PASSWORD
o  X'03' to X'7F' IANA ASSIGNED
o  X'80' to X'FE' RESERVED FOR PRIVATE METHODS
o  X'FF' NO ACCEPTABLE METHODS
*/

/* Reply field:
o  X'00' succeeded
o  X'01' general SOCKS server failure
o  X'02' connection not allowed by ruleset
o  X'03' Network unreachable
o  X'04' Host unreachable
o  X'05' Connection refused
o  X'06' TTL expired
o  X'07' Command not supported
o  X'08' Address type not supported
o  X'09' to X'FF' unassigned
*/

/* user-ip-port where ip is ipv4 in dotted format */
static int strip_redirectaddr(unsigned char *login, uint32_t *redip, unsigned short *redport)
{
    char ipbuf[16], portbuf[6];
    struct sockaddr_in addr;
    char err;

    if (!(login = strtok(login, "-")))
        return 0;
    if (!(login = strtok(NULL, "-")))
        return 0;
    strncpy(ipbuf, login, 15);
    ipbuf[15] = 0;

    if (!(login = strtok(NULL, "-")))
        return 0;
    strncpy(portbuf, login, 5);
    chainport[5] = 0;

    if (inet_pton(AF_INET, ipbuf, &addr.sin_addr) != 1)
        return 0;
    
    addr.sin_port = strtol(portbuf, &err, 10);
    if (*err)
        return 0;

    *redip = ntohl(addr.sin_addr.s_addr);
    *redport = ntohs(addr.sin_port);

    return 1;
}

static int socks5_choosemethod(int clientfd)
{
    uint8_t b, a;
    int n;
    int clipasswd = 0;

    /* version */
    if ((n = readn_timeo(clientfd, &b, 1, timeo[BYTE_L], 0)) <= 0)
        return -1;
    if (b != 0x05)
        return -1;

    /* methods */
    if ((n = readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0)) <= 0)
        return -1;
    while (b--) {
        if ((n = readn_timeo(clientfd, &a, 1, timeo[BYTE_S], 0)) <= 0)
            return -1;
        if (a == 0x02 && NEEDAUTH)
            clipasswd = 0x02;
    }
    if (NEEDAUTH && !clipasswd)
        clipasswd = 0xFF;

    return clipasswd;
}

/* get socks5 user/login from a client */
static int socks5_auth(int clientfd, unsigned char login[256], unsigned char passwd[256])
{
    int n;
    uint8_t b;
    char buf[2];

    /* version */
    if ((n = readn_timeo(clientfd, &b, 1, timeo[BYTE_L], 0)) <= 0)
        return -1;
    if (b != 0x01)
        return -1;

    /* user */ 
    if ((n = readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0)) <= 0)
        return -1;
    if ((n = readn_timeo(clientfd, login, b, timeo[STRING_S], 0)) <= 0) 
        return -1;
    login[n] = 0;

    /* passwd */
    if ((n = readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0)) <= 0)
        return -1;
    if ((n = readn_timeo(clientfd, passwd, b, timeo[STRING_S], 0)) <= 0)
        return -1;
    passwd[n] = 0;

    /* success */
    buf[0] = 1; buf[1] = 0;
    if ((n = writen_timeo(clientfd, buf, 2, timeo[STRING_S], 0)) != 2)
        return -1;

    return 0;
}

static int check_credentials(const char *user, const char *passwd)
{
    return 1;
}

int socks5_negotiate(int clientfd)
{
    int n;
    uint8_t b, cmd, atype;
    unsigned char buf[256];
    struct addrinfo *addrin;
    struct sockaddr_storage sstorage;

    if ((n = readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0)) <= 0)
        return 1;
    if (b != 0x05)
        return 1;
    
    /* cmd */
    if ((n = readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0)) <= 0)
        return 1;
    cmd = b;
    if (cmd < 0x01 || cmd > 0x03) 
        return 7;
   
    if ((n = readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0)) <= 0)
        return 1; 

    if ((n = readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0)) <= 0)
        return 1; 
    atype = b;

    switch (atype) {
    case (0x01):
        if ((n = readn_timeo(clientfd, buf, 4, timeo[BYTE_L], 0)) <= 0)
            return 1;
        ((struct sockaddr_in)sstorage).sin_family = AF_INET;
        ((struct sockaddr_in)sstorage).sin_addr.s_addr = *(uint32_t*)buf;

    case (0x03):
        if ((n = readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0)) <= 0)
            return 1;
        if ((n = readn_timeo(clientfd, buf, b, timeo[BYTE_L], 0)) <= 0)
            return 1;
        buf[n] = 0;
        if ((addrin = host_serv(buf, NULL, AF_INET, SOCK_STREAM)) == NULL) 
            return 4;
        
        ((struct sockaddr_in)sstorage).sin_family = addrin->ai_family;
        ((struct sockaddr_in)sstorage).sin_addr.s_addr = ((struct sockaddr_in*)addrin->ai_addr)->sin_addr.s_addr;
        freeaddrinfo(addrin);

    case (0x04):
        if ((n = readn_timeo(clientfd, buf, 16, timeo[BYTE_L], 0)) <= 0)
            return 1;
        ((struct sockaddr_in6)sstorage).sin6_family = AF_INET6;
        *(uint64_t*)(((struct sockaddr_in6)sstorage).sin6_addr.s6_addr) = *(uint64_t*)buf;
        *(uint64_t*)(((struct sockaddr_in6)sstorage).sin6_addr.s6_addr + 8) = *(uint64_t*)&buf[8];

    default:
        return 8;
    }
    
    if ((n = readn_timeo(clientfd, buf, 2, timeo[BYTE_S], 0)) <= 0)
        return 1;

    /* port... */
}

int socks5_run(int clientfd)
{
    unsigned char buf[BUFSIZE];
    unsigned char login[256], passwd[256];
    int n;
    uint8_t method;
    uint32_t redip;
    unsigned short redport;

    if ((method = socks5_choosemethod(clientfd)) == -1)
        goto error;

    buf[0] = 0x05;
    buf[1] = method;
    if ((n = writen_timeo(clientfd, buf, 2, timeo[STRING_S], 0)) != 2)
        goto error;

    /* authentication */
    if (method == 0x02) {
        if (socks5_auth(clientfd, login, passwd) == -1)
            goto error;
        if (!strip_redirectaddr(login, &redip, &redport))
            goto error;
    } else if (method && method != 0xFF) {
        do_debug("custom socks5 method? %d", method);
    }

    /* request */
    

error:
    close(clientfd);
    exit(1);
}
