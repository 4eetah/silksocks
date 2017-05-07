#include "common.h"

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s ip_uint32_t...\n", argv[0]);
        exit(1);
    }
   
    int i;
    char buf[16];
    uint32_t ipaddr;
    for (i = 1; i < argc; ++i) {
        ipaddr = (uint32_t)atol(argv[i]);
        sprintf(buf, "passwd%d", i);
        cache_put(ipaddr, buf);
    }

    char *val;
    for (i = 1; i < argc; ++i) {
        ipaddr = (uint32_t)atol(argv[i]);
        val = cache_get(ipaddr); 
        if (!val) {
            fprintf(stderr, "cache_get(%u) error\n", ipaddr);
            break;
        }
        printf("%u->%s\n", ipaddr, val); 
    }
}
