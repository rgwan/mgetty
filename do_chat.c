#ident "$Id: do_chat.c,v 1.22 1993/11/13 11:09:17 gert Exp $ Copyright (c) Gert Doering";
/* do_chat.c
 *
 * This module handles all the non-fax talk with the modem
 */

#include <stdio.h>
#ifndef _NOSTDLIB_H
#include <stdlib.h>
#endif
#include <unistd.h>
#include <signal.h>
#ifndef sun
#include <sys/ioctl.h>
#endif

#ifndef EINTR
#include <errno.h>
#endif

#include "mgetty.h"
#include "tio.h"

boolean chat_has_timeout;
static RETSIGTYPE chat_timeout()
{
    chat_has_timeout = TRUE;
}

extern volatile boolean virtual_ring;

int do_chat _P6((fd, expect_send, actions, action, chat_timeout_time,
		timeout_first ),
		int fd, char * expect_send[],
	        chat_action_t actions[], action_t * action,
                int chat_timeout_time, boolean timeout_first )
{
#define BUFFERSIZE 500
char	buffer[BUFFERSIZE];
int	i,cnt;
int	retcode = SUCCESS;
int	str;
char	*p;
int	h;
boolean	nocr;
TIO	tio, save_tio;

    tio_get( fd, &tio );
    save_tio = tio;
    tio_mode_raw( &tio );
    tio_set( fd, &tio );

    signal( SIGALRM, chat_timeout );

    if ( actions != NULL && action != NULL ) *action = A_FAIL;

    str=0;
    while ( expect_send[str] != NULL )
    {
	/* expect a string (expect_send[str] or abort[]) */
	i = 0;

	if ( strlen( expect_send[str] ) != 0 )
	{
	    lprintf( L_MESG, "waiting for ``%s''", expect_send[str] );

	    lprintf( L_NOISE, "got: " );

	    chat_has_timeout = FALSE;

	    /* set alarm timer. for the first string, the timer is only
	       set if the flag "timeout_first" is true */

	    if ( str != 0 || timeout_first )
		alarm( chat_timeout_time );
	    else
		alarm( 0 );

	    do
	    {
		if ( virtual_ring && strcmp( expect_send[str], "RING" ) == 0 )
		{
		    lputs( L_MESG, " ``found''" );
		    break;
		}
		
		cnt = read( fd, &buffer[i], 1 );

		if ( cnt < 0 )
		{
		    if ( errno == EINTR ) cnt = 0;
		    else
		        lprintf( L_ERROR, "do_chat: error in read()" );
		}

		if ( chat_has_timeout )		/* timeout */
		{
		    lprintf( L_ERROR,"timeout in chat skript, waiting for `%s'", expect_send[str] );
		    retcode = FAIL;
		    break;
		}

		if ( cnt > 0 ) lputc( L_NOISE, buffer[i] );

		i += cnt;
		if ( i>BUFFERSIZE-5 )
		{
		    memcpy( &buffer[0], &buffer[BUFFERSIZE/2], i-BUFFERSIZE/2+1 );
		    i-=BUFFERSIZE/2;
		}

		/* look for the "expect"-string */

		cnt = strlen( expect_send[str] );
		if ( i >= cnt &&
		     memcmp( &buffer[i-cnt], expect_send[str], cnt ) == 0 )
		{
		    lputs( L_MESG, " ** found **" );
		    break;
		}

		/* look for one of the "abort"-strings */
                if ( actions != NULL )
		  for ( h=0; actions[h].expect != NULL; h ++ )
		{
		    cnt = strlen( actions[h].expect );
		    if ( i>=cnt && 
			 memcmp( &buffer[i-cnt], actions[h].expect, cnt ) == 0 )
		    {
			lprintf( L_MESG,"found action string: ``%s''",
			                actions[h].expect );
			*action = actions[h].action;
			retcode = FAIL;
			break;
		    }
		}
	    }
	    while ( i<BUFFERSIZE && retcode != FAIL );

	    /* disable timeout alarm clock */
	    alarm(0);

	    /* found abort string or timeout? */
	    if ( retcode == FAIL ) break;

	}		/* end if (strlen(expect_send[str] != 0) */

	str++;
	/* end of list? */

	if ( expect_send[str] == NULL ) break;

	/* before sending, maybe we have to pause a little */
#ifdef DO_CHAT_SEND_DELAY
	delay(DO_CHAT_SEND_DELAY);
#endif
	
	/* send a string, translate esc-sequences */
        lprintf( L_MESG, "send: " );

	p = expect_send[str];
	nocr = FALSE;
        h = 0;
	while ( *p != 0 ) 
	{
	    if ( *p == '\\' )
	    {
		switch ( *(++p) )
		{
		    case 'd': lputs( L_MESG, "\\d"); delay(500); break;
		    case 'c': nocr = TRUE; break;
		    default:
		        write( fd, p, 1 );
                        lputc( L_MESG, *p );
		}
	    }
	    else
	    {
		write( fd, p, 1 );
		lputc( L_MESG, *p );
	    }
	    p++;
	}
	if ( ! nocr ) { write( fd, "\r\n", 2 ); lputs( L_MESG, "\\r\\n" ); }

	str++;

    }			/* end while ( expect_send[str] != NULL ) */

    /* reset terminal settings */
    tio_set( fd, &save_tio );

    return retcode;
}

int clean_line _P2 ((fd, waittime), int fd, int waittime )
{
TIO	tio, save_tio;
char	buffer[2];

    lprintf( L_NOISE, "waiting for line to clear, read: " );

    /* set terminal timeout to "waittime" tenth of a second */
    tio_get( fd, &tio );
    save_tio = tio;				/*!! FIXME - sgtty?! */
    tio.c_lflag &= ~ICANON;
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = waittime;
    tio_set( fd, &tio );

    /* read everything that comes from modem until a timeout occurs */
    while ( read( fd, buffer, 1 ) > 0 ) 
        lputc( L_NOISE, buffer[0] );

    /* reset terminal settings */
    tio_set( fd, &save_tio );
    
    return 0;
}
