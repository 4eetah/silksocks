#ifndef ERROR_H
#define ERROR_H

#include "common.h"

#define MAXLINE 4096
#define PORTSTRLEN 6

extern int daemon_proc;
extern int silk_log_level;
extern int silk_debug_level;
extern __thread char log_buf[INET6_ADDRSTRLEN+PORTSTRLEN+1];

typedef enum {
    CLIENT,
    SERVER,
} PEER;
/* log_buf per thread storage */
#define addr2logbuf(sa) inet_ntop(sockFAMILY(sa), sockADDR(sa), log_buf, sizeof(log_buf))
const char *peer2logbuf(int sockfd, PEER p);
void log_doit(int errnoflag, int level, const char *fmt, ...);

/* There are 3 meaningfull debug levels 
* 0
* 1
* 2
*/
#ifdef DEBUG
#define SILK_DBG(dbg_level, fmt, ...) \
    do { \
        if (dbg_level <= silk_debug_level) \
            log_doit(0, LOG_DEBUG, "%s [%s] " fmt, gf_time(), __func__, ##__VA_ARGS__); \
    } while (0)

#define SILK_DBG_ERRNO(dbg_level, fmt, ...) \
    do { \
        if (dbg_level <= silk_debug_level) \
            log_doit(1, LOG_DEBUG, "%s [%s] " fmt, gf_time(), __func__, ##__VA_ARGS__); \
    } while (0)
#else
#define SILK_DBG(level, fmt, ...)
#define SILK_DBG_ERRNO(level, fmt, ...)
#endif

/* Standart syslog levels
* EMERG      system is unusable
* ALERT      action must be taken immediately
* CRIT       critical conditions
* ERR        error conditions
* WARNING    warning conditions
* NOTICE     normal, but significant, condition
* INFO       informational message
* For DEBUG level use SILK_DBG as it expands debug levels
* and doesn't interfere in case debug lvl is turned off
*/

#define SILK_LOG(LVL, fmt, ...) \
    do { \
        if (LOG_##LVL <= silk_log_level) \
            log_doit(0, LOG_##LVL, "%s [%s] " fmt, syslog_lvl2str(LOG_##LVL), __func__, ##__VA_ARGS__); \
    } while (0)

#define SILK_LOG_ERRNO(LVL, fmt, ...) \
    do { \
        if (LOG_##LVL <= silk_log_level) \
            log_doit(1, LOG_##LVL, "%s [%s] " fmt, syslog_lvl2str(LOG_##LVL), __func__, ##__VA_ARGS__); \
    } while (0)

void err_ret(const char *fmt, ...);
void err_sys(const char *fmt, ...);
void err_dump(const char *fmt, ...);
void err_msg(const char *fmt, ...);
void err_quit(const char *fmt, ...);

#endif // ERROR_H
