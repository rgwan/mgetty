#ident "$Id: io.c,v 1.3 1993/09/01 22:49:12 gert Exp $ (c) Gert Doering";
/* io.c
 *
 * This module contains a few low-level I/O functions
 * (will be extended)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mgetty.h"

/* warning: these includes have to appear *after* "mgetty.h"! */

#ifdef USE_POLL
#include <poll.h>
int poll( struct pollfd fds[], unsigned long nfds, int timeout );
#else
#ifdef USE_SELECT
# if defined (linux) || defined (sun)
# include <sys/time.h>
# else
# include <sys/select.h>
# endif
#endif
#endif

void delay( int waittime )		/* wait waittime milliseconds */
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
    if ( waittime < 1000 ) waittime = 1000;	/* round up */
    sleep( waittime / 1000);
#endif	/* use select */
#endif	/* use nap */
#endif	/* use poll */
}

/* check_for_input( open file deskriptor )
 *
 * returns TRUE if there's something to read on filedes, FALSE otherwise
 */

boolean	check_for_input( int filedes )
{
#ifdef USE_SELECT
    fd_set	readfds;
    struct	timeval timeout;
#else	/* use poll */
    struct	pollfd fds;
#endif
    int ret;

#ifdef USE_SELECT

    FD_ZERO( &readfds );
    FD_SET( filedes, &readfds );
    timeout.tv_sec = timeout.tv_usec = 0;
    ret = select( FD_SETSIZE , &readfds, NULL, NULL, &timeout );

#else	/* use poll */

    fds.fd = filedes;
    fds.events = POLLIN;
    fds.revents= 0;
    ret = poll( &fds, 1, 0 );
#endif

    if ( ret < 0 ) lprintf( L_ERROR, "poll / select failed" );

    return ( ret > 0 );
}
