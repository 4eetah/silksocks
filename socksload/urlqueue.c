#include "urlqueue.h"

static struct URLnode* NEW(const char *url, size_t url_len, struct URLnode *next)
{
    struct URLnode *x = malloc(sizeof(*x));
    x->url = strndup(url, url_len);
    x->next = next;
    return x;
}

struct URLqueue *URLQinit()
{
    struct URLqueue *q = malloc(sizeof(*q));
    q->head = q->tail = NULL;
    q->size = 0;
    return q;
}

int URLQempty(struct URLqueue *q)
{
    return q->head == NULL;
}

char *URLQget(struct URLqueue *q)
{
    char *url = q->head->url;
    struct URLnode *t = q->head;
    q->head = q->head->next;
    free(t);
    q->size--;
    return url;
}

void URLQput(struct URLqueue *q, const char *url, size_t url_len)
{
    q->size++;
    if (q->head == NULL) {
        q->head = q->tail = NEW(url, url_len, q->head);
        return;
    }
    q->tail->next = NEW(url, url_len, q->tail->next);
    q->tail = q->tail->next;
}

int URLQsize(struct URLqueue *q)
{
    return q->size;
}
