#ifndef URLQUEUE_H
#define URLQUEUE_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct URLnode {
    char *url;
    struct URLnode *next;
};
struct URLqueue {
    struct URLnode *head, *tail;
    int size;
};

struct URLqueue *URLQinit();
int URLQempty(struct URLqueue *q);
void URLQput(struct URLqueue *q, const char *url, size_t len);
char *URLQget(struct URLqueue *q);
int URLQsize(struct URLqueue *q);

#endif
