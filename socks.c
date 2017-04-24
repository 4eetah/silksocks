#include "socks.h"
#include "error.h"

int check_socks5_initpkt_sanity(char *buf, size_t n)
{
    return (n >= 3) && (buf[0] == 0x05) && (n == buf[1]+2);
}

int check_socks5_authpkt_sanity(char *buf, size_t n)
{
    return (n >= 5) && (buf[0] == 0x01) && (buf[1]+4 <= n) && (buf[1]+buf[buf[1]+2]+3 == n);
}

int check_socks5_cmdpkt_sanity(char *buf, size_t n)
{
    return (buf[0] == 0x05) && (buf[1]==0x01 || buf[1]==0x02 || buf[1]==0x03) &&
            (buf[2] == 0x00) && (buf[3]=0x01 || buf[3]==0x03 || buf[3]==0x04);
}

void do_socks5(int client_fd)
{
    char buf[BUFSIZE];
    char login[255], passwd[255];
    int n;
    uint8_t i,j;
    int srvneedauth=1;
    int clihaveauth=0;
    struct sockaddr_in dst;
    int cmd;

    if ((n = recv(client_fd, buf, BUFSIZE, 0)) == -1) {
        err_ret("fd(%d),pid(%ld): recv failed", client_fd, pthread_self());
        return;
    }
    if (!check_socks5_initpkt_sanity(buf, n)) {
        do_debug("fd(%d),pid(%ld): malformed pkt", client_fd, pthread_self());
        close(client_fd);
        return;
    }
    /* TODO: check db for the type of a auth/noauth the given client client is configured to used */

    for (i=buf[1], j=2; j < n && i; j++, i--) {
        if (buf[j] == 0x02) {
            clihaveauth=1;
            break;
        }
    }
    buf[0] = 0x05;
    if (srvneedauth) {
        if (!clihaveauth)
            buf[1] = 0xFF;
        else
            buf[1] = 0x02;
    } else {
        buf[1] = 0x00;
    }
    
    if (send(client_fd, buf, 2) == -1) {
        err_ret("fd(%d),pid(%d): write failed", client_fd, pthread_self());
        return;
    }

    if (srvneedauth) {
        if ((n = recv(client_fd, buf, BUFSIZE, 0)) == -1) {
            err_ret("fd(%d),pid(%ld): recv authentication failed", client_fd, pthread_self());
            return;
        }
        if (!check_socks5_authpkt_sanity(buf, n)) {
            do_debug("fd(%d),pid(%ld): malformed pkt", client_fd, pthread_self());
            close(client_fd);
            return;
        }
        memcpy(login, buf+2, buf[1]);
        memcpy(passwd, buf+buf[1]+3, buf[buf[1]+2]);

        /* check login/passwd */
        int authfailure=0;
        if (1) {
            buf[0] = 0x01;
            buf[1] = 0x00;
        } else {
            authfailure=1;
            buf[0] = 0x01;
            buf[1] = 0xFF;
        }

        if ((n = send(client_fd, buf, 2, 0)) == -1) {
            err_ret("fd(%d),pid(%ld): send failed", client_fd, pthread_self());
            return;
        }

        if (authfailure) {
            do_debug("auth failure");
            close(client_fd);
            return;
        }
    }

    if ((n = recv(client_fd, buf, 4)) == -1) {
        err_ret("fd(%d),pid(%ld): recv cmd failed", client_fd, pthread_self());
        return;
    }

    if (!check_socks5_cmdpkt_sanity(buf, n)) {
        do_debug("fd(%d),pid(%ld): malformed pkt", client_fd, pthread_self());
        close(client_fd);
    }

    cmd = buf[1];
    memset(&dst, 0, sizeof(dst));
    switch (buf[3]) {
    case 0x01:
        dst.sin_family = AF_INET;
        if ((n=recv(client_fd, &dst.sin_addr.s_addr, 4)) == -1) {
            err_ret("fd(%d),pid(%ld): recv cmd pkt failed", client_fd, pthread_self());
            return;
        }
        if ((n=recv(client_fd, &dst.sin_port, 2)) == -1) {
            err_ret("fd(%d),pid(%ld): recv cmd pkt failed", client_fd, pthread_self());
            return;
        }

    case 0x03:
        if ((n=recv(client_fd, j, 1)) == -1) {
            err_ret("fd(%d),pid(%ld): recv cmd pkt failed", client_fd, pthread_self());
            return;
        }

    case 0x04:
    default:

    }

    return;

}
