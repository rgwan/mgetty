#ident "$Id: ring.c,v 4.1 1998/04/15 20:01:40 gert Exp $ Copyright (c) Gert Doering"

/* ring.c
 *
 * This module handles incoming RINGs, distinctive RINGs (RING 1, RING 2,
 * etc.), and out-of-band messages (<DLE>v, "CONNECT", ...).
 *
 * Also, on ISDN "modems", multiple subscriber numbers (MSN) are mapped
 * to distinctive RING types. At least, if the ISDN device returns this
 * data.  It's known to work for ZyXEL and ELSA products.
 *
 * Works closely with "cnd.c" to grab CallerID for analog modems.
 */

#include <stdio.h>
#include "syslibs.h"
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>

#ifndef EINTR
#include <errno.h>
#endif

#include "mgetty.h"
#include "policy.h"
#include "tio.h"
#include "fax_lib.h"

static int ring_handle_ELSA _P2((string, msn_list),
				 char * string, char ** msn_list )
{
    lprintf( L_MESG, "ELSA: '%s'", string );
    return 0;			/* unspecified - FIXME */
}

static int ring_handle_ZyXEL _P2((string, msn_list),
				 char * string, char ** msn_list )
{
    lprintf( L_MESG, "ZyXEL: '%s'", string );
    return 0;			/* unspecified - FIXME */
}


static boolean chat_has_timeout;
static RETSIGTYPE chat_timeout(SIG_HDLR_ARGS)
{
    chat_has_timeout = TRUE;
}

extern boolean virtual_ring;

int wait_for_ring _P6((fd, msn_list, timeout, 
		       actions, action, dist_ring_number),
		int fd, char ** msn_list, int timeout, 
	        chat_action_t actions[], action_t * action,
		int * dist_ring_number )
{
#define BUFSIZE 500
char	buf[BUFSIZE], ch, *p;
int	i, w, r;
int	rc = SUCCESS;
boolean	got_dle;		/* for <DLE><char> events (voice mode) */

    lprintf( L_MESG, "wfr: waiting for ``RING''" );
    lprintf( L_NOISE, "got: ");

    w=0;
    got_dle = FALSE;

    signal( SIGALRM, chat_timeout );
    alarm( timeout );
    chat_has_timeout = FALSE;

    while(TRUE)
    {
	if ( virtual_ring )
	{
	    lputs( L_NOISE, "``found''" );
	    *dist_ring_number = 0;
	    break;
	}

	r = mdm_read_byte( fd, &ch );

	if ( r <= 0 )				/* unsuccessful */
	{
	    if ( chat_has_timeout )		/* timeout */
		lprintf( L_WARN, "wfr: timeout waiting for RING" );
	    else
		lprintf( L_ERROR, "wfr: error in read()" );

	    if ( action != NULL ) *action = A_TIMOUT;
	    rc = FAIL; break;
	}

	lputc( L_NOISE, ch );

	/* In voice mode, modems send <DLE><x> sequences to signal
	 * certain events, among them (IS-101) "RING".
	 */
	if ( got_dle )		/* last char was <DLE> */
	{
	    switch( ch )
	    {
		case 'h':
		case 'p':	/* handset on hook */
		case 'H':
		case 'P':	/* handset off hook */
		case 'r':	/* ringback detected */
		    *dist_ring_number = - (int)ch; break;
		case 'R':	/* RING detected */
		    *dist_ring_number = 0; break;
		default:
		    got_dle = FALSE;
	    }
	}
	else 
	    if ( ch == DLE ) got_dle = TRUE;

	/* line termination character? no -> add to buffer and go on */
	if ( ch != '\r' && ch != '\n' )
	{
	    /* add character to buffer */
	    if( w < BUFSIZ-2 )
		buf[w++] = ch;

	    /* check for "actions" */
	    if ( actions != NULL )
	      for( i=0; actions[i].expect != NULL; i++ )
	    {
		int len = strlen( actions[i].expect );
		if ( w >= len &&
		     memcmp( &buf[w-len], actions[i].expect, len ) == 0 )
		{
		    lprintf( L_MESG, "found action string: ``%s''",
				     actions[i].expect );
		    *action = actions[i].action;
		    rc = FAIL; break;
		}
	    }
	    if ( rc == FAIL ) break;		/* break out of while() */

	    /* go on */
	    continue;
	}

	/* got a full line */
	if ( w == 0 ) { continue; }		/* ignore empty lines */
	buf[w] = '\0';
	cndfind( buf );				/* grab caller ID */

	/* ZyXEL CallerID/MSN display? */
	if ( strncmp( buf, "FM:", 3 ) == 0 ||
	     strncmp( buf, "TO:", 3 ) == 0 )
	    { *dist_ring_number = ring_handle_ZyXEL( buf, msn_list ); break; }

	/* now check the different RING types */
	if ( strncmp( buf, "RING", 4 ) != 0 )	/* not RING<whatever> */
		continue;			/* get next line */

	p=&buf[4];
	while( isspace(*p) ) p++;

	if ( *p == '\0' )			/* "classic RING" */
	    { *dist_ring_number = 0; break; }

	if ( isdigit( *p ) )			/* RING 1 */
	    { *dist_ring_number = *p-'0'; break; }

	if ( isalpha( *p ) )			/* RING A */
	    { *dist_ring_number = tolower(*p)-'a' +1; break; }

	if ( *p == ';' )			/* ELSA type */
	    { *dist_ring_number = ring_handle_ELSA( p, msn_list ); break; }
    }

    alarm(0);
    lprintf( L_NOISE, "wfr: rc=%d, drn=%d", rc, *dist_ring_number );
    return rc;
}
