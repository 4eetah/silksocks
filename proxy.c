#include "common.h"

/* may be useful in case of many proxies types */
void proxy_start(void *arg)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
    int fd = (int)arg;
#pragma GCC diagnostic pop
    socks5_run(fd);
}
