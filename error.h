#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXLINE 4096
#define DEBUG 1
#ifdef DEBUG
#define do_debug(...) err_msg(__VA_ARGS__)
#else
#define do_debug(...)
#endif

void err_ret(const char *fmt, ...);
void err_sys(const char *fmt, ...);
void err_dump(const char *fmt, ...);
void err_msg(const char *fmt, ...);
void err_quit(const char *fmt, ...);

#endif // ERROR_H
