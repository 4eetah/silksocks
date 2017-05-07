#include "common.h"

int setrcvtimeo(int fd, long sec, long usec)
{
    struct timeval timer;
    timer.tv_sec = sec;
    timer.tv_usec = usec;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timer, sizeof(timer)) == -1)
        return -1;
    return 0;
}

int setsndtimeo(int fd, long sec, long usec)
{
    struct timeval timer;
    timer.tv_sec = sec;
    timer.tv_usec = usec;

    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timer, sizeof(timer)) == -1)
        return -1;
    return 0;
}


ssize_t readn_timeo(int fd, void *vptr, size_t n, long sec, long usec)
{
	size_t	nleft;
	ssize_t	nread;
	char	*ptr;

    if (setrcvtimeo(fd, sec, usec) == -1)
        return -1;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0;
			else
				return(-1);
		} else if (nread == 0)
			break;

		nleft -= nread;
		ptr   += nread;
	}
	return(n - nleft);
}

ssize_t writen_timeo(int fd, const void *vptr, size_t n, long sec, long usec)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

    if (setsndtimeo(fd, sec, usec) == -1)
        return -1;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;
			else
				return(-1);
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}
