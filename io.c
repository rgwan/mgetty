#ident "$Id: io.c,v 1.7 1993/10/19 11:40:27 gert Exp $ Copyright (c) Gert Doering";
/* io.c
 *
 * This module contains a few low-level I/O functions
 * (will be extended)
 */

#include <stdio.h>
#ifndef _NOSTDLIB_H
#include <stdlib.h>
#endif
#include <unistd.h>

#include "mgetty.h"

/* warning: these includes have to appear *after* "mgetty.h"! */

#ifdef USE_POLL
# include <poll.h>
int poll _PROTO(( struct pollfd fds[], unsigned long nfds, int timeout ));
#endif

#ifdef USE_SELECT
# include <string.h>
# if defined (linux) || defined (sun) || defined (SVR4) || defined (__hpux)
#  include <sys/time.h>
# else
#  include <sys/select.h>
# endif
#endif

void delay _P1( (waittime),
		int waittime )		/* wait waittime milliseconds */
{
#ifdef USE_POLL
struct pollfd sdummy;
    poll( &sdummy, 0, waittime );
#else
#ifdef USE_NAP
    nap( (long) waittime );
#else
#ifdef USE_SELECT
    struct timeval s;

    s.tv_sec = waittime / 1000;
    s.tv_usec = (waittime % 1000) * 1000;
    select( 0, (fd_set *) NULL, (fd_set *) NULL, (fd_set *) NULL, &s );

#else				/* neither poll nor nap nor select available */
    if ( waittime < 2000 ) waittime = 2000;	/* round up */
    sleep( waittime / 1000);			/* a sleep of 1 may not sleep at all */
#endif	/* use select */
#endif	/* use nap */
#endif	/* use poll */
}

/* check_for_input( open file deskriptor )
 *
 * returns TRUE if there's something to read on filedes, FALSE otherwise
 */

boolean	check_for_input _P1( (filedes),
			     int filedes )
{
#ifdef USE_SELECT
    fd_set	readfds;
    struct	timeval timeout;
#endif
#ifdef USE_POLL
    struct	pollfd fds;
#endif
    int ret;

#ifdef USE_SELECT

    FD_ZERO( &readfds );
    FD_SET( filedes, &readfds );
    timeout.tv_sec = timeout.tv_usec = 0;
    ret = select( FD_SETSIZE , &readfds, NULL, NULL, &timeout );

#else
# ifdef USE_POLL

    fds.fd = filedes;
    fds.events = POLLIN;
    fds.revents= 0;
    ret = poll( &fds, 1, 0 );

# else

    ret = 0;	/* CHEAT! */

# endif
#endif

    if ( ret < 0 ) lprintf( L_ERROR, "poll / select failed" );

    return ( ret > 0 );
}

/* wait until a character is available
 * where select() or poll() exists, no characters will be read,
 * if only read() can be used, at least one character will be dropped
 */
void wait_for_input _P1( (fd), int fd )
{
#ifdef USE_SELECT
    fd_set	readfds;
#endif
#ifdef USE_POLL
    struct	pollfd fds;
#endif
    int slct;

#ifdef USE_SELECT
    
    FD_ZERO( &readfds );
    FD_SET( fd, &readfds );
    slct = select( FD_SETSIZE, &readfds, NULL, NULL, NULL );
    lprintf( L_NOISE, "select returned %d", slct );

#else	/* use poll */
# ifdef USE_POLL

    fds.fd = fd;
    fds.events = POLLIN;
    fds.revents= 0;
    slct = poll( &fds, 1, -1 );
    lprintf( L_NOISE, "poll returned %d", slct );
# else
    {
	char t;
	read(1, &t, 1);
	lprintf(L_NOISE, "read returned: "); lputc(L_NOISE, c );
    }
# endif
#endif
}
