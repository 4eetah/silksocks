#include "common.h"

/* Auth Methods:
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

const char *socks5_strstatus[] = {
    "ok",
    "server failure",
    "connection not allowed by ruleset",
    "network unreachable",
    "host unreachable",
    "connection refused",
    "TTL expired",
    "command not supported",
    "address type not supported",
};

struct socks5_cli {
    struct sockaddr_storage request;
    unsigned char user[256], passwd[256];
    int clientfd, proxyfd;
    uint8_t cmd;
    uint32_t proxyip; // in host byte order
    unsigned short proxyport; // in host byte order
};

/* user-ip-port where ip is ipv4 in dotted format */
static int strip_redirectaddr(struct socks5_cli *client)
{
    char ipbuf[16], portbuf[6];
    unsigned char *login;
    struct sockaddr_in addr;
    char *err;

    login = client->user;

    if (!(login = strtok(login, "-")))
        return 1;
    if (!(login = strtok(NULL, "-")))
        return 1;
    strncpy(ipbuf, login, 15);
    ipbuf[15] = 0;

    if (!(login = strtok(NULL, "-")))
        return 1;
    strncpy(portbuf, login, 5);
    portbuf[5] = 0;

    if (inet_pton(AF_INET, ipbuf, &addr.sin_addr) != 1)
        return 1;
    
    addr.sin_port = strtol(portbuf, &err, 10);
    if (*err)
        return 1;

    client->proxyip = ntohl(addr.sin_addr.s_addr);
    client->proxyport = addr.sin_port;

    return 0;
}

static int socks5_choosemethod(int clientfd, uint8_t *climethod)
{
    uint8_t b, a;
    unsigned char buf[2];

    *climethod = 0;

    /* version */
    if (readn_timeo(clientfd, &b, 1, timeo[BYTE_L], 0) != 1)
        return 1;
    if (b != 0x05)
        return 1;

    /* methods */
    if (readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0) != 1)
        return 1;
    while (b--) {
        if (readn_timeo(clientfd, &a, 1, timeo[BYTE_S], 0) != 1)
            return 1;
        if (a == 0x02 && NEEDAUTH)
            *climethod = 0x02;
    }
    if (NEEDAUTH && !*climethod)
        *climethod = 0xFF;

    buf[0] = 0x05;
    buf[1] = *climethod;
    if (writen_timeo(clientfd, buf, 2, timeo[STRING_S], 0) != 2)
        return 1;

    return 0;
}

/* get socks5 user/login from a client */
static int socks5_readauth(struct socks5_cli *client)
{
    uint8_t b;
    char buf[2];
    int clientfd;

    clientfd = client->clientfd;

    /* version */
    if (readn_timeo(clientfd, &b, 1, timeo[BYTE_L], 0) != 1)
        return 1;
    if (b != 0x01)
        return 1;

    /* user */ 
    if (readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0) != 1)
        return 1;
    if (readn_timeo(clientfd, client->user, b, timeo[STRING_S], 0) != b)
        return 1;
    client->user[b] = 0;

    /* passwd */
    if (readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0) != 1)
        return 1;
    if (readn_timeo(clientfd, client->passwd, b, timeo[STRING_S], 0) != b)
        return 1;
    client->passwd[b] = 0;

    /* success */
    buf[0] = 1; buf[1] = 0;
    if (writen_timeo(clientfd, buf, 2, timeo[STRING_S], 0) != 2)
        return 1;

    return 0;
}

/* check app authentication from db */
static int socks5_doauth(struct socks5_cli *client)
{
    char *apppasswd;

#ifdef USEDB
    apppasswd = cache_getapp(client->user);
    if (!apppasswd) {
        if ((apppasswd = sqlget_apppasswd(client->user)) == NULL)
            return 5;
        cache_putapp(client->user, apppasswd);
    }
#else
    apppasswd = strdup("pwd");
#endif

    if (strcmp(client->passwd, apppasswd))
        return 5;

    return 0;
}

static int socks5_writerequest(struct socks5_cli *client)
{
    unsigned char buf[32];
    size_t n;
    
    buf[0] = 0x05;
    buf[1] = 0x01;
    buf[2] = 0x00;
    buf[3] = client->request.ss_family == AF_INET ? 1 : 4;

    n = sockADDRLEN(&client->request);
    memcpy(&buf[4], sockADDR(&client->request), n);
    memcpy(&buf[4 + n], sockPORT(&client->request), 2);

    n += 4 + 2;

    if (writen_timeo(client->proxyfd, buf, n, timeo[STRING_S], 0) != n)
        return 4;

    return 0;
}

int socks5_readreply(struct socks5_cli *client)
{
    unsigned char buf[256];
    uint8_t b;

    if (readn_timeo(client->proxyfd, buf, 4, timeo[BYTE_S], 0) != 4)
        return 4;
    if (buf[0] != 0x05 || buf[1] != 0x00)
        return buf[1];

    switch (buf[3]) {
    case (0x01):
        if (readn_timeo(client->proxyfd, buf, 6, timeo[BYTE_L], 0) != 6)
            return 2;
        break;
    
    case (0x03):
        if (readn_timeo(client->proxyfd, &b, 1, timeo[BYTE_S], 0) != 1)
            return 2;
        if (readn_timeo(client->proxyfd, buf, b+2, timeo[STRING_S], 0) != (b+2))
            return 2;
        break;
    case (0x04):
        if (readn_timeo(client->proxyfd, buf, 18, timeo[BYTE_L], 0) != 18)
            return 2;
        break;
    default:
        return 8;
    }
    
    return 0;
}

int socks5_readrequest(struct socks5_cli *client)
{
    int clientfd;
    uint8_t b;
    unsigned char buf[256];
    struct addrinfo *addrin;
    struct sockaddr_storage *sstorage;

    clientfd = client->clientfd;
    sstorage = &client->request;    

    if (readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0) != 1)
        return 1;
    if (b != 0x05)
        return 1;
    
    /* cmd */
    if (readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0) != 1)
        return 1;
    if (b < 0x01 || b > 0x03) 
        return 7;
    client->cmd = b;
   
    /* reserved */
    if (readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0) != 1)
        return 1; 

    /* atype */
    if (readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0) != 1)
        return 1; 

    switch (b) {
    case (0x01):
        if (readn_timeo(clientfd, buf, 4, timeo[BYTE_L], 0) != 4)
            return 1;
        ((struct sockaddr_in*)sstorage)->sin_family = AF_INET;
        ((struct sockaddr_in*)sstorage)->sin_addr.s_addr = *(uint32_t*)buf;
        if (readn_timeo(clientfd, buf, 2, timeo[BYTE_L], 0) != 2)
            return 1;
        ((struct sockaddr_in*)sstorage)->sin_port = *(unsigned short*)buf;
        break;

    case (0x03):
        if (readn_timeo(clientfd, &b, 1, timeo[BYTE_S], 0) != 1)
            return 1;
        if (readn_timeo(clientfd, buf, b, timeo[BYTE_L], 0) != b)
            return 1;
        buf[b] = 0;

        /* don't care of ipv6 for now */
        sstorage->ss_family = AF_INET;
        if (!hashtbl_get(&dns_table, buf, sockADDR(sstorage))) {
            do_debug("dns lookup [%s] -> null (fail)", buf);
            if ((addrin = host_serv(buf, NULL, AF_INET, SOCK_STREAM)) == NULL) 
                return 4;

            hashtbl_put(&dns_table, buf, sockADDR(addrin->ai_addr), 0);
            memcpy(sstorage, addrin->ai_addr, addrin->ai_addrlen); 
            freeaddrinfo(addrin);
        }
#ifdef DEBUG
        else {
            do_debug("dns lookup [%s] -> %s (success)", buf, inet_ntoa(*(struct in_addr*)sockADDR(sstorage)));
        }
#endif

        if (readn_timeo(clientfd, buf, 2, timeo[BYTE_L], 0) != 2)
            return 1;

        if (sstorage->ss_family == AF_INET)
            ((struct sockaddr_in*)sstorage)->sin_port = *(unsigned short*)buf;
        else if (sstorage->ss_family == AF_INET6)
            ((struct sockaddr_in6*)sstorage)->sin6_port = *(unsigned short*)buf;
        break; 

    case (0x04):
        if (readn_timeo(clientfd, buf, 16, timeo[BYTE_L], 0) != 16)
            return 1;
        ((struct sockaddr_in6*)sstorage)->sin6_family = AF_INET6;
        *(uint64_t*)(((struct sockaddr_in6*)sstorage)->sin6_addr.s6_addr) = *(uint64_t*)buf;
        *(uint64_t*)(((struct sockaddr_in6*)sstorage)->sin6_addr.s6_addr + 8) = *(uint64_t*)&buf[8];
        if (readn_timeo(clientfd, buf, 2, timeo[BYTE_L], 0) != 2)
            return 1;
        ((struct sockaddr_in6*)sstorage)->sin6_port = *(unsigned short*)buf;
        break;

    default:
        return 8;
    }

    return 0;
}


static int socks5_writereply(struct socks5_cli *client, uint8_t status)
{
    unsigned char buf[32];
    struct sockaddr_storage ss;
    socklen_t n;

    buf[0] = 0x05;
    buf[1] = status;
    buf[2] = 0;

    n = sizeof(ss);
    if (getsockname(client->proxyfd, (struct sockaddr*)&ss, &n) == -1)
        return 1;

    buf[3] = (sockFAMILY(&ss) == AF_INET) ? 1 : 4;
    n = sockADDRLEN(&ss);
    memcpy(&buf[4], sockADDR(&ss), n);
    memcpy(&buf[4 + n], sockPORT(&ss), 2);

    n += 4 + 2;

    if (writen_timeo(client->clientfd, buf, n, timeo[STRING_S], 0) != n)
        return 1;

    return 0;
}

static int socks5_connectproxy(struct socks5_cli *client)
{
    char buf[512];
    int n, status;
    struct sockaddr_in sin;
    unsigned char *proxyuser;
    unsigned char *proxypasswd;

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(client->proxyip);
    sin.sin_port = htons(client->proxyport);

    if ((client->proxyfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return 1;
    if (connect_nonb(client->proxyfd, (struct sockaddr*)&sin, sizeof(sin), timeo[CONNECT_S], 0) == -1)
        return 4;

    buf[0] = 0x05; 
    buf[1] = 1;
    buf[2] = 0x02;
    if (writen_timeo(client->proxyfd, buf, 3, timeo[BYTE_L], 0) != 3)
        return 4;

    if (readn_timeo(client->proxyfd, buf, 2, timeo[BYTE_L], 0) != 2)
        return 4;
    if (buf[0] != 5 || buf[1] != 0x02)
        return 4;

#ifdef USEDB
    struct ip2creds *val;
    val = cache_getip(client->proxyip);
    if (!val) {
        if (sqlget_proxycreds(&proxyuser, &proxypasswd, client->proxyip, client->proxyport, 1) == -1)
            return 4;
        cache_putip(client->proxyip, proxyuser, proxypasswd);
    } else {
        proxyuser = val->user;
        proxypasswd = val->passwd;
    }
#else
    proxyuser = strdup("usr");
    proxypasswd = strdup("pwd");
#endif

    /* proxy authenticate */
    n = 0;
    buf[n++] = 0x01;
    buf[n++] = strlen(proxyuser);
    memcpy(&buf[n], proxyuser, buf[n-1]);
    n += buf[1];
    buf[n++] = strlen(proxypasswd);
    memcpy(&buf[n], proxypasswd, buf[n-1]);
    n += buf[n-1];

    if (writen_timeo(client->proxyfd, buf, n, timeo[STRING_L], 0) != n)
        return 4;
    if (readn_timeo(client->proxyfd, buf, 2, timeo[BYTE_S], 0) != 2)
        return 4;
    if (buf[0] != 0x01 || buf[1] != 0x00)
        return 5;


    if ((status = socks5_writerequest(client)) != 0)
        return status;

    if ((status = socks5_readreply(client)) != 0)
        return status;

    return 0;
}


int socks5_run(int clientfd)
{
    struct socks5_cli client;
    int status;
    uint8_t method;

    client.clientfd = clientfd;

    if ((status = socks5_choosemethod(clientfd, &method)) != 0)
        goto done;

    /* authentication */
    if (method == 0x02) {
        if ((status = socks5_readauth(&client)) != 0)
            goto done;
        if ((status = strip_redirectaddr(&client)) != 0)
            goto done;
        if ((status = socks5_doauth(&client)) != 0)
            goto done;
    } else if (method && method != 0xFF) {
        do_debug("custom socks5 method? %d", method);
        status = 7;
        goto done;
    }

    /* request from client */
    if ((status = socks5_readrequest(&client)) != 0)
        goto done;

    switch (client.cmd) {
    case (0x01):
        status = socks5_connectproxy(&client);
        /* reply to client */
        if ((status = socks5_writereply(&client, status)) != 0)
            goto done;
        if ((status = negotiate(client.clientfd, client.proxyfd)) != 0)
            goto done;

        break;

    case (0x02): // bind not implemented yet
    case (0x03): // udp associate not implemented yet
    default:
        status = 7;
        goto done;
    }

done:

    close(client.clientfd);
    close(client.proxyfd);
    return status;
}
