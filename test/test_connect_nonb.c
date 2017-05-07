#include "common.h"

/* the connection timeout may be tested on
 * non-routable ip e.g 10.255.255.1
 */
int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s host service connect_timeout(sec)\n", argv[0]);
        exit(1);
    }

    int nsec;
    int fd;
    struct addrinfo *addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        err_sys("socket");
    }

    addr = host_serv(argv[1], argv[2], AF_INET, SOCK_STREAM);
    if (addr == NULL) {
        err_quit("addrinfo");
    }

    nsec = atoi(argv[3]);
    if (connect_nonb(fd, addr->ai_addr, addr->ai_addrlen, nsec) == -1)
        err_sys("connect_nonb");
    printf("connected successfully within %d sec\n", nsec);
}
