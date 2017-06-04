#ifndef ERROR_H
#define ERROR_H

#include "common.h"

#define MAXLINE 4096

#if DEBUG_LVL > 0
#define do_debug_errno(fmt, ...) err_ret("%s (%16s): " fmt, gf_time(), __func__, ##__VA_ARGS__)
#define do_debug(fmt, ...) err_msg("%s (%16s): " fmt, gf_time(), __func__, ##__VA_ARGS__)
#else
#define do_debug_errno(fmt, ...)
#define do_debug(fmt, ...)
#endif

#if DEBUG_LVL > 1
#define do_debug2_errno(fmt, ...) do_debug_errno(fmt, ##__VA_ARGS__) 
#define do_debug2(fmt, ...) do_debug(fmt, ##__VA_ARGS__)
#else
#define do_debug2_errno(fmt, ...)
#define do_debug2(fmt, ...)
#endif

#if DEBUG_LVL > 2
#define do_debug3_errno(fmt, ...) do_debug_errno(fmt, ##__VA_ARGS__) 
#define do_debug3(fmt, ...) do_debug(fmt, ##__VA_ARGS__)
#else
#define do_debug3_errno(fmt, ...)
#define do_debug3(fmt, ...)
#endif

void err_ret(const char *fmt, ...);
void err_sys(const char *fmt, ...);
void err_dump(const char *fmt, ...);
void err_msg(const char *fmt, ...);
void err_quit(const char *fmt, ...);

#endif // ERROR_H
