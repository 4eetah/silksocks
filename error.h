#ifndef ERROR_H
#define ERROR_H

#include "common.h"

#define MAXLINE 4096

#ifdef DEBUG
#define do_debug_errno(fmt, ...) err_ret("%s (%s): " fmt, gf_time(), __func__, ##__VA_ARGS__)
#define do_debug(fmt, ...) err_msg("%s (%s): " fmt, gf_time(), __func__, ##__VA_ARGS__)
#else
#define do_debug_errno(fmt, ...)
#define do_debug(fmt, ...)
#endif

void err_ret(const char *fmt, ...);
void err_sys(const char *fmt, ...);
void err_dump(const char *fmt, ...);
void err_msg(const char *fmt, ...);
void err_quit(const char *fmt, ...);

#endif // ERROR_H
