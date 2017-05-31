#include "common.h"

int negotiate(int clientfd, int proxyfd)
{
	int			maxfdp1, clienteof;
	ssize_t		n, nwritten;
	fd_set		rset, wset;
	char		to[BUFSIZE], fr[BUFSIZE];
	char		*toiptr, *tooptr, *friptr, *froptr;

#ifdef DEBUG
    char buf[256];
    int len=0;
    struct sockaddr_storage addr;
    socklen_t addrlen;

    addrlen = sizeof(addr);
    if (getpeername(clientfd, (struct sockaddr*)&addr, &addrlen) == -1) {
        err_ret("(%d,%d): getpeername error on clientfd", clientfd, proxyfd);
        return 1;
    }
    if (inet_ntop(sockFAMILY(&addr), sockADDR(&addr), buf, sizeof(buf)) == NULL) {
        err_ret("(%d,%d): inet_ntop error on clientfd", clientfd, proxyfd);
        return 1;
    }

    len = strlen(buf);
    len += sprintf(buf+len, ":%u <--(%d)--(%d)--> ", *sockPORT(&addr), clientfd, proxyfd);

    addrlen = sizeof(addr);
    getpeername(proxyfd, (struct sockaddr*)&addr, &addrlen);
    inet_ntop(sockFAMILY(&addr), sockADDR(&addr), buf+len, sizeof(buf)-len);

    len = strlen(buf);
    sprintf(buf+len, ":%u", *sockPORT(&addr));

    do_debug("%s", buf);
#endif

    if (setnonblock(clientfd) == -1)
        return 1;
    if (setnonblock(proxyfd) == -1)
        return 1;

	toiptr = tooptr = to;
	friptr = froptr = fr;
	clienteof = 0;

	maxfdp1 = max(clientfd, proxyfd) + 1;
	for ( ; ; ) {
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		if (clienteof == 0 && toiptr < &to[BUFSIZE])
			FD_SET(clientfd, &rset);	        /* read from client socket */
		if (friptr < &fr[BUFSIZE])
			FD_SET(proxyfd, &rset);			/* read from proxy server socket */
		if (tooptr != toiptr)
			FD_SET(proxyfd, &wset);			/* data to write to proxy server socket */
		if (froptr != friptr)
			FD_SET(clientfd, &wset);	    /* data to write to client socket */

		if (select(maxfdp1, &rset, &wset, NULL, NULL) == -1)
            return 1;

		if (FD_ISSET(clientfd, &rset)) {
			if ( (n = read(clientfd, toiptr, &to[BUFSIZE] - toiptr)) < 0) {
				if (errno != EWOULDBLOCK) {
					do_debug_errno("(%d,%d): read error on clientfd", clientfd, proxyfd);
                    return 1;
                }

			} else if (n == 0) {
				do_debug("(%d,%d): EOF on clientfd", clientfd, proxyfd);

				clienteof = 1;			/* all done with client */
				if (tooptr == toiptr)
				    shutdown(proxyfd, SHUT_WR); /* send FIN */

			} else {
				do_debug("(%d,%d): read %ld bytes from clientfd", clientfd, proxyfd, n);

				toiptr += n;			/* # just read */
				FD_SET(proxyfd, &wset);	/* try and write to socket below */
			}
		}

		if (FD_ISSET(proxyfd, &rset)) {
			if ( (n = read(proxyfd, friptr, &fr[BUFSIZE] - friptr)) < 0) {
				if (errno != EWOULDBLOCK) {
					do_debug_errno("(%d,%d): read error on proxyfd", clientfd, proxyfd);
                    return 1;
                }

			} else if (n == 0) {
				do_debug("(%d,%d): EOF on proxy socket", clientfd, proxyfd);

				if (clienteof)
					return 0;		/* normal termination */
				else {
					do_debug("(%d,%d): proxy server terminated prematurely", clientfd, proxyfd);
                    return 1;
                }

			} else {
				do_debug("(%d,%d): read %ld bytes from proxy server socket", clientfd, proxyfd, n);

				friptr += n;		/* # just read */
				FD_SET(clientfd, &wset);	/* try and write below */
			}
		}

		if (FD_ISSET(clientfd, &wset) && ( (n = friptr - froptr) > 0)) {
			if ( (nwritten = write(clientfd, froptr, n)) < 0) {
				if (errno != EWOULDBLOCK) {
					do_debug_errno("(%d,%d): write error to clientfd", clientfd, proxyfd);
                    return 1;
                }

			} else {
				do_debug("(%d,%d): wrote %ld bytes to client socket", clientfd, proxyfd, nwritten);

				froptr += nwritten;		/* # just written */
				if (froptr == friptr)
					froptr = friptr = fr;	/* back to beginning of buffer */
			}
		}

		if (FD_ISSET(proxyfd, &wset) && ( (n = toiptr - tooptr) > 0)) {
			if ( (nwritten = write(proxyfd, tooptr, n)) < 0) {
				if (errno != EWOULDBLOCK) {
					do_debug_errno("(%d,%d): write error to proxyfd", clientfd, proxyfd);
                    return 1;
                }

			} else {
				do_debug("(%d,%d): wrote %ld bytes to proxy socket", clientfd, proxyfd, nwritten);

				tooptr += nwritten;	/* # just written */
				if (tooptr == toiptr) {
					toiptr = tooptr = to;	/* back to beginning of buffer */
					if (clienteof)
						shutdown(proxyfd, SHUT_WR);	/* send FIN */
				}
			}
		}
	}
}
