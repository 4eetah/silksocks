#include "common.h"

int connect_nonb(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec)
{
	int				flags, n, error;
	socklen_t		len;
	fd_set			rset, wset;
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
        do_debug("connect completed immediately");
		goto done;
    }

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	wset = rset;
	tval.tv_sec = nsec;
	tval.tv_usec = 0;

	if ( (n = select(sockfd+1, &rset, &wset, NULL,
					 nsec ? &tval : NULL)) == 0) {
        do_debug("select, timeout expired");
		close(sockfd);
		errno = ETIMEDOUT;
		return(-1);
	} else if (n == -1) {
        error = errno;
        err_ret("connect_nonb: select error");
        goto done;
    }

	if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
		len = sizeof(error);
		if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
			return(-1);
        do_debug("successfully connected");
	} else {
		err_ret("connect_nonb: select error, sockfd not set");
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
