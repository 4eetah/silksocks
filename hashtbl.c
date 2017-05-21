#include "common.h"

#define he_idx(i) \
    ((struct hash_entry*)((char*)hashtbl->entries + (i)*sizeof(struct hash_entry) + hashtbl->record_size-4))

static time_t currtime = 0;
pthread_mutex_t hashtbl_mutex = PTHREAD_MUTEX_INITIALIZER;

int hashtbl_init(struct hash_table *hashtbl, size_t size, size_t record_size)
{
    hashtbl->size = size;
    hashtbl->record_size = record_size;

    if ((hashtbl->table = calloc(size, sizeof(struct hash_entry*))) == NULL)
        return -1;

    if ((hashtbl->entries = calloc(size, sizeof(struct hash_entry)+record_size-4)) == NULL)
        return -1;

    hashtbl->empty_entry = hashtbl->entries;

    int i;
    for (i = 0; i < size-1; ++i) {
        he_idx(i)->next = he_idx(i+1);
    }

    return 0;
}

int hashtbl_put(struct hash_table *hashtbl, unsigned char *key, unsigned char *val, time_t expires)
{
    pthread_mutex_lock(&hashtbl_mutex);
    if (!hashtbl->empty_entry) {
        pthread_mutex_unlock(&hashtbl_mutex);
        return 0;
    }

    struct hash_entry *hentry, *phe, **pphe;
    size_t idx;

    hentry = hashtbl->empty_entry;
    hentry->hash = hashStr(key);
    hentry->expires = expires;
    hashtbl->empty_entry = hashtbl->empty_entry->next;
    hentry->next = NULL;
    memcpy(hentry->value, val, hashtbl->record_size);
   
    idx = hentry->hash % hashtbl->size;

#ifdef DEBUG
    char buf[INET6_ADDRSTRLEN];
    do_debug("hashtbl put: [%s] = %s, idx = %lu", key, (hashtbl->record_size==4 ? \
            inet_ntop(AF_INET, (void*)val, buf, sizeof(buf)) : inet_ntop(AF_INET6, (void*)val, buf, sizeof(buf))), idx);
#endif            

    for (pphe = hashtbl->table+idx; (phe = *pphe);) {
        if (hentry->hash == phe->hash || phe->expires < currtime ) {
            *pphe = phe->next;
            phe->expires = 0;
            phe->next = hashtbl->empty_entry;
            hashtbl->empty_entry = phe;
        } else {
            pphe = &phe->next;
        }
    }
    hentry->next = hashtbl->table[idx];
    hashtbl->table[idx] = hentry;

    pthread_mutex_unlock(&hashtbl_mutex);
    return 1;
}

int hashtbl_get(struct hash_table *hashtbl, unsigned char *key, unsigned char *val)
{
    struct hash_entry **pphe, *phe;
    unsigned long hash;
    size_t idx;

    hash = hashStr(key);
    idx = hash % hashtbl->size;

    for (pphe = hashtbl->table+idx; (phe = *pphe);) {
        if (phe->expires < currtime) {
            *pphe = phe->next;
            phe->expires = 0;
            phe->next = hashtbl->empty_entry;
            hashtbl->empty_entry = phe;
        } else if (phe->hash == hash) {
            memcpy(val, phe->value, hashtbl->record_size);
            return 1;
        } else {
            pphe = &phe->next;
        }
    }
    return 0;
}

void hashtbl_print(struct hash_table *hashtbl)
{
    int i;

    printf("table size: %lu\n", hashtbl->size);
    printf("record size: %lu\n", hashtbl->record_size);
    printf("empty entry: %p\n", hashtbl->empty_entry);

    printf("\ntable:\n");
    for (i = 0; i < hashtbl->size; ++i)
        printf("%4d: %p\n", i, hashtbl->table[i]);
    printf("\n");

    printf("entries:\n");
    for (i = 0; i < hashtbl->size; ++i)
        printf("%4d: curr: %p, next: %p\n", i, he_idx(i), he_idx(i)->next);
    printf("------\n");
    printf("\n\n");
}

struct hash_table dns_table;
struct hash_table dns6_table;

#undef he_idx
