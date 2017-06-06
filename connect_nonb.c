#include "common.h"

#ifdef CONNB_BLOCK
int connect_nonb(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec, int nusec)
{
    if (connect(sockfd, saptr, salen) == -1)
        return -1;
    return 0;
}
#elif CONNB_SELECT
int connect_nonb(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec, int nusec)
{
	int				flags, n, error;
	socklen_t		len;
	fd_set			wset;
	struct timeval	tval;

	error = 0;

	if ((flags = fcntl(sockfd, F_GETFL, 0)) == -1)
        return -1;
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK))
        return -1;

	if ( (n = connect(sockfd, saptr, salen)) < 0)
		if (errno != EINPROGRESS)
			return(-1);

	if (n == 0) {
        SILK_LOG(INFO, "connect(%d) completed immediately", sockfd);
		goto done;
    }

	FD_ZERO(&wset);
	FD_SET(sockfd, &wset);
	tval.tv_sec = nsec;
	tval.tv_usec = nusec;

    /* on Linux select() modifies timeout to reflect the amount of  time  not  slept */
again:
    if ( (n = select(sockfd+1, NULL, &wset, NULL,
                     nsec ? &tval : NULL)) == 0) {
        SILK_LOG(INFO, "select(%d), timeout expired", sockfd);
        close(sockfd);
        errno = ETIMEDOUT;
        return(-1);
    } else if (n == -1) {
        if (errno == EINTR)
            goto again;
        error = errno;
        err_ret("connect_nonb(%d): select error", sockfd);
        goto done;
    }

    if (FD_ISSET(sockfd, &wset)) {
        len = sizeof(error);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
            return(-1);
        SILK_LOG(INFO"connect(%d) success", sockfd);
    } else {
        if (errno == EINPROGRESS)
            goto again;
        err_ret("connect_nonb(%d): select error, sockfd not set", sockfd);
        close(sockfd);
        return -1;
    }

done:
	if (fcntl(sockfd, F_SETFL, flags) == -1)
        return -1;

	if (error) {
		close(sockfd);
		errno = error;
		return(-1);
	}
	return(0);
}
#else
int connect_nonb(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec, int nusec)
{
    struct pollfd fds[1];
    int flags;
    int n;

	if ((flags = fcntl(sockfd, F_GETFL, 0)) == -1)
        return -1;
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK))
        return -1;

    if(connect(sockfd, saptr, salen) < 0)
        if(errno != EAGAIN && errno != EINPROGRESS) {
            SILK_LOG_ERRNO(NOTICE, "connect error");
            return -1;
        }

    memset(fds, 0, sizeof(fds));
    fds[0].fd = sockfd;
    fds[0].events = POLLOUT;

again:
    n = poll(fds, 1, nsec * 1000);
    if (n == -1) {
        if (errno == EINTR || errno == EAGAIN) {
            usleep(SLEEPTIME);
            goto again;
        }
        SILK_LOG_ERRNO(NOTICE, "poll error");
        return -1;
    }
    if (n == 0) {
        SILK_LOG(NOTICE, "poll error, timeout expired");
        return -1; 
    }

    fcntl(sockfd, F_SETFL, flags);

    return 0;
}
#endif
