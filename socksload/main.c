#include "common.h"
#include "urlqueue.h"
#include "prv.h"

#define STARTPAGE "www.yandex.ru"
#define QBACKLOG 1024
#define PROXYMGR prvMGRIPPORT
#define PROXYMGRUSR prvMGRUSR
#define PROXYMGRPWD prvMGRPWD

static int NR_proxy;
static char proxylist[1024][16];

static size_t cb(char *d, size_t n, size_t l, void *p)
{
    size_t len = n*l;
    struct URLqueue *Q = (struct URLqueue*)p;
    if (URLQsize(Q) < QBACKLOG) {
        //printf("%.*s\n", (int)len, d);
        myPCRE_href(d, len, Q);
    }
    return len;
}

static void init(CURLM *cm, struct URLqueue *Queue)
{
    CURL *eh = curl_easy_init();
    char *url;

    if (URLQempty(Queue))
        return;
    url = URLQget(Queue);

    static unsigned int i = 0;
    char prxuser[strlen(PROXYMGRUSR)+32];
    strcpy(prxuser, PROXYMGRUSR);
    strcat(prxuser, proxylist[i%NR_proxy]);
    strcat(prxuser, "-1080");
    i++;


    curl_easy_setopt(eh, CURLOPT_PROXY, "socks5://" PROXYMGR);
    curl_easy_setopt(eh, CURLOPT_PROXYUSERNAME, prxuser);
    curl_easy_setopt(eh, CURLOPT_PROXYPASSWORD, PROXYMGRPWD);
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(eh, CURLOPT_WRITEDATA, Queue);
    curl_easy_setopt(eh, CURLOPT_HEADER, 0L);
    curl_easy_setopt(eh, CURLOPT_URL, url);
    curl_easy_setopt(eh, CURLOPT_PRIVATE, url);
    curl_easy_setopt(eh, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(eh, CURLOPT_FOLLOWLOCATION, 1L);

    curl_multi_add_handle(cm, eh);
}

void curl_multi_fire(void *arg)
{
    CURLM *cm;
    CURLMsg *msg;
    long L;
    unsigned int C=0;
    int M, Q, U = -1;
    fd_set R, W, E;
    struct timeval T;
    struct URLqueue *Queue;
    int max_conn = (int)arg;

    curl_global_init(CURL_GLOBAL_ALL);

    cm = curl_multi_init();

    curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, (long)max_conn);

    Queue = URLQinit();
    URLQput(Queue, STARTPAGE, strlen(STARTPAGE));
    init(cm, Queue);

    while(U) {
        curl_multi_perform(cm, &U);
        if(U) {
          FD_ZERO(&R);
          FD_ZERO(&W);
          FD_ZERO(&E);

          if(curl_multi_fdset(cm, &R, &W, &E, &M)) {
            fprintf(stderr, "E: curl_multi_fdset\n");
            return;
          }

          if(curl_multi_timeout(cm, &L)) {
            fprintf(stderr, "E: curl_multi_timeout\n");
            return;
          }
          if(L == -1)
            L = 100;

          if(M == -1) {
            sleep((unsigned int)L / 1000);
          }
          else {
            T.tv_sec = L/1000;
            T.tv_usec = (L%1000)*1000;

            if(0 > select(M+1, &R, &W, &E, &T)) {
              fprintf(stderr, "E: select(%i,,,,%li): %i: %s\n",
                  M+1, L, errno, strerror(errno));
              return;
            }
          }
        }

        while((msg = curl_multi_info_read(cm, &Q))) {
          if(msg->msg == CURLMSG_DONE) {
            char *url;
            CURL *e = msg->easy_handle;
            //curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &url);
            //fprintf(stderr, "R: %d - %s <%s>\n",
            //        msg->data.result, curl_easy_strerror(msg->data.result), url);
            curl_multi_remove_handle(cm, e);
            curl_easy_cleanup(e);
          }
          else {
            fprintf(stderr, "E: CURLMsg (%d)\n", msg->msg);
          }
          while (U < max_conn) {
              init(cm, Queue);
              U++;
          }
        }
    }

    curl_multi_cleanup(cm);
    curl_global_cleanup();
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s <max_conn> <proxylist.txt> [<max_threads>]\n", argv[0]);
        exit(1);
    }
    int i;
    int max_conn = atoi(argv[1]);
    FILE *proxyfile = fopen(argv[2], "r");

    if (!proxyfile) {
        fprintf(stderr, "Unable to open %s\n", argv[2]);
        exit(1);
    }

    int len;
    while (fgets(proxylist[NR_proxy], 16, proxyfile) != NULL) {
        len = strlen(proxylist[NR_proxy]);
        if (proxylist[NR_proxy][len-1] == '\n')
            proxylist[NR_proxy][len-1] = 0;
        NR_proxy++;
    }

    pthread_t *threads;
    int max_threads = 1;
    if (argc == 4)
        max_threads = atoi(argv[3]);

    threads = calloc(max_threads, sizeof(pthread_t));

    for (i = 0; i < max_threads; ++i) {
        pthread_create(&threads[i], NULL, &curl_multi_fire, (void*)max_conn);
    }

    for (i = 0; i < max_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    return EXIT_SUCCESS;
}
