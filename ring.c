#ident "$Id: ring.c,v 4.5 1998/05/02 18:58:29 gert Exp $ Copyright (c) Gert Doering"

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

/* strdup variant that returns "<null>" in case of out-of-memory */
static char * safedup( char * in )
{
    char * p = strdup( in );
    return ( p == NULL ) ? "<null>" : p;
}

/* find number given in msn_list, return index */
static int find_msn _P2((string, msn_list),
			 char * string, char ** msn_list )
{
int i, len, len2;

    lprintf( L_NOISE, "MSN: '%s'", string );
    if ( msn_list == NULL ) return 0;

    CalledNr = safedup(string);			/* save away */

    len=strlen(string);
    for( i=0; msn_list[i] != NULL; i++ )
    {
	lprintf( L_JUNK, "match: '%s'", msn_list[i] );
	len2=strlen( msn_list[i] );
	if ( len2 <= len && 
	     strcmp( msn_list[i], &string[len-len2] ) == 0 )
		{ return i+1; }
    }
    return 0;				/* not found -> unspecified */
}

/* ELSA CallerID data comes in as "RING;<from>[;<to>]" */
static int ring_handle_ELSA _P2((string, msn_list),
				 char * string, char ** msn_list )
{
char * p;
    lprintf( L_MESG, "ELSA: '%s'", string );

    string++;
    p=string;
    while( isdigit(*p) ) p++;

    if ( *p == '\0' )		/* no MSN listed */
    {
	CallerId = safedup( string );
	return 0;
    }
    else			/* MSN listed -> terminate string, get MSN */
    {
	*p = '\0';
	CallerId = safedup( string );
	return find_msn( p+1, msn_list );
    }
}

/* ZyXEL CallerID data comes in as "FM:<from> [TO:<to>]" or "TO:<to>" */
static int ring_handle_ZyXEL _P2((string, msn_list),
				 char * string, char ** msn_list )
{
char * p, ch;
    lprintf( L_MESG, "ZyXEL: '%s'", string );

    if ( strncmp( string, "FM:", 3 ) == 0 )		/* caller ID */
    {
	string+=3;
	p=string;
	while( isdigit(*p) ) p++;
	ch = *p; *p = '\0';
	CallerId = safedup(string);
	*p = ch;
	while( isspace(*p) ) p++;
	string = p;
    }
    if ( strncmp( string, "TO:", 3 ) == 0 )		/* target msn */
    {
	return find_msn( string+3, msn_list );
    }
    return 0;			/* unspecified */
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

	if ( *p == ';' )			/* ELSA type */
	    { *dist_ring_number = ring_handle_ELSA( p, msn_list ); break; }

	if ( strlen(p) > 1 )			/* USR type B: "RING 1234" */
	    { CallerId = safedup(p); *dist_ring_number = 0; break; }

	if ( isdigit( *p ) )			/* RING 1 */
	    { *dist_ring_number = *p-'0'; break; }

	if ( isalpha( *p ) )			/* RING A */
	    { *dist_ring_number = tolower(*p)-'a' +1; break; }
    }

    alarm(0);
    lprintf( L_NOISE, "wfr: rc=%d, drn=%d", rc, *dist_ring_number );
    return rc;
}
