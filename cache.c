/* simple ip->passwd mapping without collision resolution
 * TODO implement collision resolution
 * */

#include "common.h"

#define MAP_SIZE (1 << 15) // ip to passwd mapping
#define idx(n) (((uint32_t)(n)) % MAP_SIZE)
static char *map[MAP_SIZE];

/* Robert Jenkins' 32 bit integer hash function */
static uint32_t hash32(uint32_t a)
{
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^0xc761c23c) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^0xb55a4f09) ^ (a>>16);
    return a;
}

int cache_put(uint32_t key, char *val)
{
    size_t id = idx(hash32(key));
    if (id < 0 || id >= MAP_SIZE)
        return 1;
    map[id] = strndup(val, 255);
    do_debug("map[%d] = %s, key = %u", id, val, key);
    return 0;
}

char *cache_get(uint32_t key)
{
    size_t id = idx(hash32(key));
    if (id < 0 || id >= MAP_SIZE)
        return NULL;
    do_debug("map[%d] = %s, key = %u", id, map[id], key);
    return map[id];
}

void cache_free()
{
    long i;
    for (i = 0; i < MAP_SIZE; ++i)
        if (map[i])
            free(map[i]);
}
