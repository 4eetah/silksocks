/* simple ip->passwd mapping without collision resolution
 * TODO implement collision resolution
 * */

#include "common.h"

#define MAPIP_SIZE (1 << 15) // ip to credentials mapping
#define MAPAPP_SIZE (1 << 10) // app to passwd mapping

#define idx_ipmap(n) (((uint32_t)(n)) % MAPIP_SIZE)
#define idx_appmap(n) (((unsigned long)(n)) % MAPAPP_SIZE)

static struct ip2creds *map_ip2creds[MAPIP_SIZE];
static char *map_app2pass[MAPAPP_SIZE];

/* Robert Jenkins' 32 bit integer hash function */
uint32_t hash32(uint32_t a)
{
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^0xc761c23c) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^0xb55a4f09) ^ (a>>16);
    return a;
}

/* djb2 */
unsigned long hashStr(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c;

    return hash;
}

int cache_putip(uint32_t key, unsigned char *user, unsigned char *passwd)
{
    size_t id = idx_ipmap(hash32(key));
    struct ip2creds *val = malloc(sizeof(struct ip2creds));
    val->user = strdup(user);
    val->passwd = strdup(passwd);
    map_ip2creds[id] = val;
    SILK_LOG(INFO, "map_ip2creds[%lu] = {%s,%s}, key = %u", id, val->user, val->passwd, key);
    return 0;
}

struct ip2creds *cache_getip(uint32_t key)
{
    size_t id = idx_ipmap(hash32(key));
    SILK_LOG(INFO, "map_ip2creds[%lu] = {%s,%s}, key = %u", id, map_ip2creds[id] ? map_ip2creds[id]->user : NULL,
                                                map_ip2creds[id] ? map_ip2creds[id]->passwd : NULL, key);
    return map_ip2creds[id];
}

void cache_freeip()
{
    long i;
    for (i = 0; i < MAPIP_SIZE; ++i)
        if (map_ip2creds[i]) {
            free(map_ip2creds[i]->user);
            free(map_ip2creds[i]->passwd);
            free(map_ip2creds[i]);
        }
}

int cache_putapp(unsigned char *app, unsigned char *passwd)
{
    size_t id = idx_appmap(hashStr(app));
    map_app2pass[id] = strdup(passwd);
    SILK_LOG(INFO, "map_app2pass[%lu] = %s, key = %s", id, map_app2pass[id], app);
    return 0;
}

char *cache_getapp(unsigned char *app)
{
    size_t id = idx_appmap(hashStr(app));
    SILK_LOG(INFO, "map_app2pass[%lu] = %s, key = %s", id, map_app2pass[id], app);
    return map_app2pass[id];
}

void cache_freeapp()
{
    long i;
    for (i = 0; i < MAPAPP_SIZE; ++i)
        if (map_app2pass[i]) {
            free(map_app2pass[i]);
        }
}
