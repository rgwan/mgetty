#ident "$Id: modem.c,v 4.8 2014/01/31 11:32:59 gert Exp $ Copyright (c) Gert Doering"

/* modem.c
 *
 * Module containing *very* basic modem functions
 *   - send a command
 *   - get a response back from the modem
 *   - send a command, wait for OK/ERROR response
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "mgetty.h"
#include "policy.h"
#include "tio.h"

/* get one line from the modem, only printable characters, terminated
 * by \r or \n. The termination character is *not* included
 */

char * mdm_get_line _P1( (fd), int fd )
{
    static char buffer[200];
    int bufferp;
    char c;
    
    bufferp = 0;
    lprintf( L_JUNK, "got:" );
    
    do
    {
	if( mdm_read_byte( fd, &c ) != 1 )
	{
	    lprintf( L_ERROR, "mdm_get_line: cannot read byte, return" );
	    return NULL;
	}

	lputc( L_JUNK, c );

	if ( isprint( c ) &&
	     bufferp < sizeof(buffer) )
	{
	    buffer[ bufferp++ ] = c;
	}
    }
    while ( bufferp == 0 || ( c != 0x0a && c != 0x0d ) );

    buffer[bufferp] = 0;

    return buffer;
}

/* wait for a given modem response string,
 * handle all the various class 2 / class 2.0 status responses
 */

boolean fwf_timeout = FALSE;

RETSIGTYPE fwf_sig_alarm(SIG_HDLR_ARGS)      	/* SIGALRM handler */
{
    signal( SIGALRM, fwf_sig_alarm );
    lprintf( L_WARN, "Warning: got alarm signal!" );
    fwf_timeout = TRUE;
}

/* send a command string to the modem, terminated with the
 * MODEM_CMD_SUFFIX character / string from policy.h
 */

int mdm_send _P2( (send, fd),
		  char * send, int fd )
{
#ifdef DO_CHAT_SEND_DELAY
    delay(DO_CHAT_SEND_DELAY);
#endif

    lprintf( L_MESG, "mdm_send: '%s'", send );

    if ( write( fd, send, strlen( send ) ) != strlen( send ) ||
	 write( fd, MODEM_CMD_SUFFIX, sizeof(MODEM_CMD_SUFFIX)-1 ) !=
	        ( sizeof(MODEM_CMD_SUFFIX)-1 ) )
    {
	lprintf( L_ERROR, "mdm_send: cannot write" );
	return ERROR;
    }

    return NOERROR;
}

/* simple send / expect sequence, for things that do not require
 * parsing of the modem responses, or where the side-effects are
 * unwanted.
 */

int mdm_command _P2( (send, fd), char * send, int fd )
{
    char * l;
    
    if ( mdm_send( send, fd ) == ERROR ) return ERROR;

    /* wait for OK or ERROR, *without* side effects (as fax_wait_for
     * would have)
     */
    signal( SIGALRM, fwf_sig_alarm ); alarm(10); fwf_timeout = FALSE;

    do
    {
	l = mdm_get_line( fd );
	if ( l == NULL ) break;
	lprintf( L_NOISE, "mdm_command: string '%s'", l );
    }
    while ( strcmp( l, "OK" ) != 0 && strcmp( l, "ERROR" ) != 0 );

    alarm(0); signal( SIGALRM, SIG_DFL );
    
    if ( l == NULL || strcmp( l, "ERROR" ) == 0 )
    {
	lputs( L_MESG, " -> ERROR" );
	return ERROR;
    }
    lputs( L_MESG, " -> OK" );
	
    return NOERROR;
}

/* mdm_read_byte
 * read one byte from "fd", with buffering
 * caveat: only one fd allowed (only one buffer), no way to flush buffers
 */

int mdm_read_byte _P2( (fd, c),
		       int fd, char * c )
{
static char frb_buf[512];
static int  frb_rp = 0;
static int  frb_len = 0;

    if ( frb_rp >= frb_len )
    {
	frb_len = read( fd, frb_buf, sizeof( frb_buf ) );
	if ( frb_len <= 0 )
	{
	    if ( frb_len == 0 ) errno = 0;	/* undefined otherwise */
	    lprintf( L_ERROR, "mdm_read_byte: read returned %d", frb_len );
	    return frb_len;
	}
	frb_rp = 0;
    }

    *c = frb_buf[ frb_rp++ ];
    return 1;
}

/* for modem identify (and maybe other nice purposes, who knows)
 * this function is handy:
 * - send some AT command, wait for OK/ERROR or 10 seconds timeout
 * - return a pointer to a static buffer holding the "nth" non-empty
 *   answer line from the modem (for multi-line responses), or the 
 *   last line if n==-1
 */
char * mdm_get_idstring _P3( (send, n, fd), char * send, int n, int fd )
{
    char * l; int i;
    static char rbuf[120];

    if ( mdm_send( send, fd ) == ERROR ) return "<ERROR>";

    /* wait for OK or ERROR, *without* side effects (as fax_wait_for
     * would have)
     */
    signal( SIGALRM, fwf_sig_alarm ); alarm(10); fwf_timeout = FALSE;

    i=0;
    rbuf[0] = '\0';

    while(1)
    {
	l = mdm_get_line( fd );

	if ( l == NULL ) break;				/* error */
	if ( strcmp( l, send ) == 0 ) continue;		/* command echo */

        if ( strcmp( l, "OK" ) == 0 ||			/* final string */
	     strcmp( l, "ERROR" ) == 0 ) break;

        i++;
	lprintf( L_NOISE, "mdm_gis: string %d: '%s'", i, l );

	if ( i==-1 || i==n )		/* copy string */
	    { strncpy( rbuf, l, sizeof(rbuf)-1); rbuf[sizeof(rbuf)-1]='\0'; }
    }

    alarm(0); signal( SIGALRM, SIG_DFL );
    
    if ( l == NULL ) return "<ERROR>";			/* error */

    return rbuf;
}


/* clean_line()
 *
 * wait for the line "fd" to be silent for "waittime" tenths of a second
 * if more than 500 bytes are read, stop logging them. We don't want to
 * have the log files fill up all of the hard disk.
 */

int clean_line _P2 ((fd, waittime), int fd, int waittime )
{
    char buffer[2];
    int	 bytes = 0;				/* bytes read */

#if defined(MEIBE) || defined(NEXTSGTTY) || defined(BROKEN_VTIME) ||\
	defined(FAX_SEND_SOCKETS)
    /* on some systems, the VMIN/VTIME mechanism is obviously totally
     * broken. So, use a select()/flush queue loop instead.
     */
    lprintf( L_NOISE, "waiting for line to clear (select/%d ms), read: ", waittime * 100 );

    while( wait_for_input( fd, waittime * 100 ) &&
	   read( fd, buffer, 1 ) > 0 &&
	   bytes < 10000 )
    {
	if ( ++bytes < 500 ) lputc( L_NOISE, buffer[0] );
    }
#else
TIO	tio, save_tio;

    lprintf( L_NOISE, "waiting for line to clear (VTIME=%d), read: ", waittime);

    /* set terminal timeout to "waittime" tenth of a second */
    tio_get( fd, &tio );
    save_tio = tio;				/*!! FIXME - sgtty?! */
    tio.c_lflag &= ~ICANON;
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = waittime;
    tio_set( fd, &tio );

    /* read everything that comes from modem until a timeout occurs */
    while ( read( fd, buffer, 1 ) > 0 &&
	    bytes < 10000 )
    {
        if ( ++bytes < 500 ) lputc( L_NOISE, buffer[0] );
    }

    /* reset terminal settings */
    tio_set( fd, &save_tio );
    
#endif

    if ( bytes > 500 )
        lprintf( L_WARN, "clean_line: only 500 of %d bytes logged", bytes );
    if ( bytes >= 10000 )
    {
	extern char * Device;
        lprintf( L_FATAL, "clean_line: got too much junk (dev=%s).", Device );
    }
    
    return 0;
}
