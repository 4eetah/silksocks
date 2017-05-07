#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>

#include "error.h"
#include "thpool.h"

#define LISTENQ 1024
#define SRV_PORT 1080
#define NR_THREADS 100
#define NEEDAUTH 1


#endif // COMMON_H
