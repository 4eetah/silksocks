#include "common.h"

int negotiate(int clientfd, int proxyfd)
{
	int			maxfdp1, clienteof;
	ssize_t		n, nwritten;
	fd_set		rset, wset;
	char		to[BUFSIZE], fr[BUFSIZE];
	char		*toiptr, *tooptr, *friptr, *froptr;

    if (setnonblock(clientfd) == -1)
        return -1;
    if (setnonblock(proxyfd) == -1)
        return -1;

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
            return -1;

		if (FD_ISSET(clientfd, &rset)) {
			if ( (n = read(clientfd, toiptr, &to[BUFSIZE] - toiptr)) < 0) {
				if (errno != EWOULDBLOCK) {
					do_debug("read error on clientfd");
                    return -1;
                }

			} else if (n == 0) {
				do_debug("EOF on clientfd");

				clienteof = 1;			/* all done with client */
				if (tooptr == toiptr)
				    shutdown(proxyfd, SHUT_WR); /* send FIN */

			} else {
				do_debug("read %ld bytes from clientfd", n);

				toiptr += n;			/* # just read */
				FD_SET(proxyfd, &wset);	/* try and write to socket below */
			}
		}

		if (FD_ISSET(proxyfd, &rset)) {
			if ( (n = read(proxyfd, friptr, &fr[BUFSIZE] - friptr)) < 0) {
				if (errno != EWOULDBLOCK) {
					do_debug("read error on proxyfd");
                    return -1;
                }

			} else if (n == 0) {
				do_debug("EOF on proxy socket");

				if (clienteof)
					return 0;		/* normal termination */
				else {
					do_debug("proxy server terminated prematurely");
                    return -1;
                }

			} else {
				do_debug("read %ld bytes from proxy server socket", n);

				friptr += n;		/* # just read */
				FD_SET(clientfd, &wset);	/* try and write below */
			}
		}

		if (FD_ISSET(clientfd, &wset) && ( (n = friptr - froptr) > 0)) {
			if ( (nwritten = write(clientfd, froptr, n)) < 0) {
				if (errno != EWOULDBLOCK) {
					do_debug("write error to clientfd");
                    return -1;
                }

			} else {
				do_debug("wrote %ld bytes to client socket", nwritten);

				froptr += nwritten;		/* # just written */
				if (froptr == friptr)
					froptr = friptr = fr;	/* back to beginning of buffer */
			}
		}

		if (FD_ISSET(proxyfd, &wset) && ( (n = toiptr - tooptr) > 0)) {
			if ( (nwritten = write(proxyfd, tooptr, n)) < 0) {
				if (errno != EWOULDBLOCK) {
					do_debug("write error to proxyfd");
                    return -1;
                }

			} else {
				do_debug("wrote %ld bytes to proxy socket", nwritten);

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
