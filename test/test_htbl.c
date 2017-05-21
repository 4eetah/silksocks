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

int main(int argc, char **argv)
{
    struct hostent *hent;
    uint32_t addr;

    hashtbl_init(&dns_table, 8, 4);
    hashtbl_print(&dns_table);  

    int i;
    for (i = 0; i < sizeof(hosts)/sizeof(*hosts); ++i) {
        if ((hent = gethostbyname(hosts[i])) != NULL) {
            addr = ((struct in_addr*)hent->h_addr)->s_addr;
            hashtbl_put(&dns_table, hosts[i], (unsigned char*)&addr, 0);
            hashtbl_print(&dns_table);
        }
    }

    printf("=========\n");

    for (i = 0; i < sizeof(hosts)/sizeof(*hosts); ++i) {
        addr = ((struct in_addr*)hent->h_addr)->s_addr;
        if (hashtbl_get(&dns_table, hosts[i], (unsigned char*)&addr)) {
            printf("Get: [%s] -> %s (success)\n", hosts[i], inet_ntoa(*(struct in_addr*)&addr));
        } else {
            printf("Get: [%s] -> null (fail)\n", hosts[i]);
        }
        hashtbl_print(&dns_table);
    }

}
