#ifndef COMMON_H
#define COMMON_H

#include <curl/curl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include "urlqueue.h"

int myPCRE_href(const char *html, size_t htmlsize, struct URLqueue *q);

#endif
