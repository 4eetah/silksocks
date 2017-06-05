#include "common.h"

#define he_idx(i) \
    ((struct hash_entry*)((char*)hashtbl->entries + (i)*sizeof(struct hash_entry) + hashtbl->record_size-4))

int hashtbl_init(struct hash_table *hashtbl, size_t size, size_t record_size)
{
    pthread_mutex_init(&hashtbl->hmutex, NULL);

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

int hashtbl_check(struct hash_table *hashtbl, unsigned char *key)
{
    struct hash_entry **pphe, *phe;
    unsigned long hash;
    size_t idx;
    time_t now;

    hash = hashStr(key);
    idx = hash % hashtbl->size;
    now = time(NULL);

    for (pphe = hashtbl->table+idx; (phe = *pphe);) {
        if (phe->hash == hash && phe->expires > now)
            return 1;
        pphe = &phe->next;
    }
    return 0;
}

int hashtbl_put(struct hash_table *hashtbl, unsigned char *key, unsigned char *val, time_t expires)
{
    pthread_mutex_lock(&hashtbl->hmutex);
    if (!hashtbl->empty_entry) {
        pthread_mutex_unlock(&hashtbl->hmutex);
        return 0;
    }

    if (hashtbl_check(hashtbl, key)) {
        pthread_mutex_unlock(&hashtbl->hmutex);
        return 1;
    }

    struct hash_entry *hentry, *phe, **pphe;
    size_t idx;
    time_t now;

    hentry = hashtbl->empty_entry;
    hentry->hash = hashStr(key);
    hentry->expires = expires;
    hashtbl->empty_entry = hashtbl->empty_entry->next;
    hentry->next = NULL;
    memcpy(hentry->value, val, hashtbl->record_size);
   
    idx = hentry->hash % hashtbl->size;

#ifdef DEBUG
    char buf[INET6_ADDRSTRLEN];
    SILK_DBG(1, "hashtbl put: [%s] = %s, idx = %lu", key, (hashtbl->record_size == 4 ? \
            inet_ntop(AF_INET, (void*)val, buf, sizeof(buf)) : inet_ntop(AF_INET6, (void*)val, buf, sizeof(buf))), idx);
#endif            
    now = time(NULL);

    for (pphe = hashtbl->table+idx; (phe = *pphe);) {
        if (hentry->hash == phe->hash || phe->expires < now) {
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

    pthread_mutex_unlock(&hashtbl->hmutex);
    return 1;
}

int hashtbl_get(struct hash_table *hashtbl, unsigned char *key, unsigned char *val)
{
    struct hash_entry **pphe, *phe;
    unsigned long hash;
    size_t idx;
    time_t now;

    hash = hashStr(key);
    idx = hash % hashtbl->size;
    now = time(NULL);

    pthread_mutex_lock(&hashtbl->hmutex);
    for (pphe = hashtbl->table+idx; (phe = *pphe);) {
        if (phe->expires < now) {
            *pphe = phe->next;
            phe->expires = 0;
            phe->next = hashtbl->empty_entry;
            hashtbl->empty_entry = phe;
        } else if (phe->hash == hash) {
            memcpy(val, phe->value, hashtbl->record_size);
            pthread_mutex_unlock(&hashtbl->hmutex);
            return 1;
        } else {
            pphe = &phe->next;
        }
    }
    pthread_mutex_unlock(&hashtbl->hmutex);
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
