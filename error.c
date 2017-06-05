#include "error.h"
#include <syslog.h>

int	daemon_proc;		/* set nonzero by daemon_init() */
int silk_debug_level;
int silk_log_level;
__thread char log_buf[INET6_ADDRSTRLEN+PORTSTRLEN+1];

const char *syslog_lvl[] = {
    "emerg",
    "alert",
    "crit",
    "err",
    "warning",
    "notice",
    "info",
    "debug",
    "disabled",
};
const char *syslog_lvl2str(int level)
{
    if (level < 0 || level > LOG_DEBUG)
        return syslog_lvl[sizeof(syslog_lvl)-1];
    return syslog_lvl[level];
}

static void	err_doit(int, int, const char *, va_list);

const char *peer2logbuf(int sockfd, PEER peer)
{
    int len=0;
    struct sockaddr_storage addr;
    socklen_t addrlen;
    addrlen = sizeof(addr);

    if (getpeername(sockfd, (struct sockaddr*)&addr, &addrlen) == -1) {
        return NULL;
    }
    if (inet_ntop(sockFAMILY(&addr), sockADDR(&addr), log_buf, sizeof(log_buf)) == NULL) {
        return NULL;
    }
    len = strlen(log_buf);
    snprintf(log_buf+len, sizeof(log_buf)-len, ":%u", ntohs(*sockPORT(&addr)));
    return log_buf;
}

/* Nonfatal error related to system call
 * Print message and return */

void
err_ret(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, LOG_INFO, fmt, ap);
	va_end(ap);
	return;
}

/* Fatal error related to system call
 * Print message and terminate */

void
err_sys(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

/* Fatal error related to system call
 * Print message, dump core, and terminate */

void
err_dump(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(1, LOG_ERR, fmt, ap);
	va_end(ap);
	abort();		/* dump core and terminate */
	exit(1);		/* shouldn't get here */
}

/* Nonfatal error unrelated to system call
 * Print message and return */

void
err_msg(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(0, LOG_INFO, fmt, ap);
	va_end(ap);
	return;
}

/* Fatal error unrelated to system call
 * Print message and terminate */

void
err_quit(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	err_doit(0, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(1);
}

/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */

void log_doit(int errnoflag, int level, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(errnoflag, level, fmt, ap);
    va_end(ap);
    return;
}

static void
err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
	int		errno_save, n;
	char	buf[MAXLINE + 1];

	errno_save = errno;		/* value caller might want printed */
	vsnprintf(buf, MAXLINE, fmt, ap);	/* safe */
	n = strlen(buf);
	if (errnoflag)
		snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
	strcat(buf, "\n");

	if (daemon_proc) {
		syslog(level, "%s", buf);
	} else {
		fflush(stdout);		/* in case stdout and stderr are the same */
		fputs(buf, stderr);
		fflush(stderr);
	}
	return;
}
