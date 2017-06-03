#include "common.h"

unsigned char *hosts[] = {
    "google.com",
    "google.com",
    "amazon.com",
    "facebook.com",
    "yandex.ru",
    "yandex.ru",
    "github.com",
    "stackoverflow.com",
    "coursera.org",
    "wikipedia.org",
    "youtube.com",
    "bash.org",
    "yahoo.com",
    "news.ycombinator.com",
    "linuxjournal.com",
};

#define idx(entry, size) (hashStr(entry) % size)
int main(int argc, char **argv)
{
    struct hostent *hent;
    uint32_t addr;
    time_t expires;

    hashtbl_init(&dns_table, 8, 4);
    hashtbl_print(&dns_table);  

    int i;
    
    expires = time(NULL)+1;
    printf("Expires at: %lu\n", expires+1);
    for (i = 0; i < sizeof(hosts)/sizeof(*hosts); ++i) {
        if ((hent = gethostbyname(hosts[i])) != NULL) {
            addr = ((struct in_addr*)hent->h_addr)->s_addr;
            hashtbl_put(&dns_table, hosts[i], (unsigned char*)&addr, time(NULL)+1);
            printf("Time now: %lu\n", time(NULL));
            printf("Put: %s , table[%ld], entry[%p]\n", hosts[i], idx(hosts[i], dns_table.size), dns_table.empty_entry);
            hashtbl_print(&dns_table);
        }
    }

    printf("=========\n");

    for (i = 0; i < sizeof(hosts)/sizeof(*hosts); ++i) {
        addr = ((struct in_addr*)hent->h_addr)->s_addr;
        printf("Time now: %lu\n", time(NULL));
        if (hashtbl_get(&dns_table, hosts[i], (unsigned char*)&addr)) {
            printf("Get[%ld]: [%s] -> %s (success)\n", idx(hosts[i], dns_table.size), hosts[i], inet_ntoa(*(struct in_addr*)&addr));
        } else {
            printf("Get[%ld]: [%s] -> null (fail)\n", idx(hosts[i], dns_table.size), hosts[i]);
        }
        hashtbl_print(&dns_table);
        // let some entries expire
        if (i == (sizeof(hosts)/(sizeof(*hosts)*2)))
            sleep(2);
    }

}
