#include "common.h"

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s ip_uint32_t...\n", argv[0]);
        exit(1);
    }
   
    int i;
    char user[16];
    char passwd[16];
    uint32_t ipaddr;
    struct ip2creds ip2c;
    for (i = 1; i < argc; ++i) {
        ipaddr = (uint32_t)atol(argv[i]);
        sprintf(user, "user%d", i);
        sprintf(passwd, "passwd%d", i);
        cache_putip(ipaddr, user, passwd);
        cache_putapp(user, passwd);
    }

    struct ip2creds *val;
    for (i = 1; i < argc; ++i) {
        ipaddr = (uint32_t)atol(argv[i]);
        val = cache_getip(ipaddr); 
        if (!val) {
            fprintf(stderr, "cache_get(%u) error\n", ipaddr);
            break;
        }
        printf("ip2creds: %u->{%s,%s}\n", ipaddr, val->user, val->passwd); 
        sprintf(user, "user%d", i);
        printf("app2pass: %s->%s\n", user, cache_getapp(user));
    }

    ipaddr = 2130706433; // loopback
    val = cache_getip(ipaddr);
    printf("ip2creds: %u->{%s,%s}\n", ipaddr, val ? val->user : NULL , val ? val->passwd : NULL); 
    printf("app2pass: %s->%s\n", "foo", cache_getapp("foo"));
}
