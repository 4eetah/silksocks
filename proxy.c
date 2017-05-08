#include "common.h"

/* may be useful in case of many proxies types */
void proxy_start(void *arg)
{
    int fd = (int)arg;
    socks5_run(fd);
}
