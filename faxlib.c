#ident "$Id: faxlib.c,v 1.9 1993/09/13 21:02:59 gert Exp $ Gert Doering"

/* faxlib.c
 *
 * Module containing generic faxmodem functions (as: send a command, wait
 * for modem responses, parse modem responses)
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>

#include "mgetty.h"
#include "fax_lib.h"

char	fax_remote_id[1000];		/* remote FAX id +FTSI */
char	fax_param[1000];		/* transm. parameters +FDCS */
fax_param_t	fax_par_d;		/* fax params detailed */
char	fax_hangup = 0;
int	fax_hangup_code = 0;		/* hangup cause +FHNG:<xxx> */
int	fax_page_tx_status = 0;		/* +FPTS:<ppm> */
boolean	fax_to_poll = FALSE;		/* there's something to poll */

static boolean fwf_timeout = FALSE;

static void fwf_sig_alarm()		/* SIGALRM handler */
{
    signal( SIGALRM, fwf_sig_alarm );
    lprintf( L_WARN, "Warning: got alarm signal!" );
    fwf_timeout = TRUE;
}

int fax_wait_for( char * s, int fd )
{
char buffer[1000];
char c;
int bufferp;

    lprintf( L_MESG, "fax_wait_for(%s)", s );

    if ( fax_hangup )
    {
	lputs( L_MESG, ": already hangup!" );
	return ERROR;
    }

    fwf_timeout = FALSE;
    signal( SIGALRM, fwf_sig_alarm );

    do
    {
	alarm( FAX_RESPONSE_TIMEOUT );

	bufferp = 0;
	lprintf( L_JUNK, "got:" );

	/* get one string, not empty, printable chars only, ended by '\n' */
	do
	{
	    if( fax_read_byte( fd, &c ) != 1 )
	    {
		lprintf( L_ERROR, "fax_wait_for: cannot read byte, return" );
		alarm( 0 ); signal( SIGALRM, SIG_DFL );
		return ERROR;
	    }
	    lputc( L_JUNK, c );
	    if ( isprint( c ) ) buffer[ bufferp++ ] = c;
	}
	while ( bufferp == 0 || c != 0x0a );
	buffer[bufferp] = 0;

	lprintf( L_NOISE, "fax_wait_for: string '%s'", buffer );

	if ( strncmp( buffer, "+FTSI:", 6 ) == 0 ||
	     strncmp( buffer, "+FCSI:", 6 ) == 0 )
	{
	    lprintf( L_MESG, "fax_id: '%s'", buffer );
	    strcpy( fax_remote_id, &buffer[6] );
	}

	else if ( strncmp( buffer, "+FDCS:", 6 ) == 0 )
	{
	    lprintf( L_MESG, "transmission par.: '%s'", buffer );
	    strcpy( fax_param, buffer );
	    if ( sscanf( &fax_param[6], "%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd",
			 &fax_par_d.vr, &fax_par_d.br, &fax_par_d.wd,
			 &fax_par_d.ln, &fax_par_d.df, &fax_par_d.ec,
			 &fax_par_d.bf, &fax_par_d.st ) != 8 )
	    {
		lprintf( L_WARN, "cannot evaluate +FDCS-Code!" );
		fax_par_d.vr = 0;
	    }
	}

	else if ( strncmp( buffer, "+FHNG:", 6 ) == 0 )
	{
	    /* hangup. First, set flag to indicate +FHNG: was read.
	     * The SIGHUP signal catcher will check this, and not exit.
	     * Next, reset the action for SIGHUP, to be ignore, so we
	     * (and child processes) are not interrupted while we cleanup.
	     * If the wait_for string is not "OK", return immediately,
	     * since that is all that the modem can send after +FHNG
	     */
	    fax_hangup = 1; /* set this as soon as possible */
	    /* change action for SIGHUP signals to be ignore */
	    if ( signal( SIGHUP, SIG_IGN ) == SIG_ERR )
	    {
		lprintf( L_WARN, "fax_wait_for: cannot reset SIGHUP handler." );
	    }
	    lprintf( L_MESG, "connection hangup: '%s'", buffer );
	    sscanf( &buffer[6], "%d", &fax_hangup_code );

	    if ( strcmp( s, "OK" ) != 0 ) break;
	}

	else if ( strncmp( buffer, "+FPTS:", 6 ) == 0 )
	{
	    /* page transmit status
	     * store into global variable (read by sendfax.c)
	     */
	    lprintf( L_MESG, "page status: %s", buffer );
	    sscanf( &buffer[6], "%d", &fax_page_tx_status );
	}

	else if ( strcmp( buffer, "+FPOLL" ) == 0 )
	{
	    /* the other side is telling us that it has a document that
	     * we can poll (with AT+FDR)
	     */
	    lprintf( L_MESG, "got +FPOLL -> will do polling" );
	    fax_to_poll = TRUE;
	}

	else
	if ( strcmp( buffer, "ERROR" ) == 0 ||
	     strcmp( buffer, "NO CARRIER" ) == 0 ||
	     strcmp( buffer, "BUSY" ) == 0 ||
	     strcmp( buffer, "NO DIALTONE" ) == 0 )
	{
	    lprintf( L_MESG, "ABORTING: buffer='%s'", buffer );
	    fax_hangup = 1;
	    fax_hangup_code = (strcmp( buffer, "BUSY" ) == 0) ? FHUP_BUSY:
								FHUP_ERROR;
	    alarm( 0 ); signal( SIGALRM, SIG_DFL );
	    return ERROR;
	}

    }
    while ( strncmp( s, buffer, strlen(s) ) != 0 );
    lputs( L_MESG, "** found **" );

    alarm( 0 );
    signal( SIGALRM, SIG_DFL );

    if ( fax_hangup && fax_hangup_code != 0 ) return ERROR;

    return NOERROR;
}

int fax_send( char * s, int fd )
{
    lprintf( L_NOISE, "fax_send: '%s'", s );
    return write( fd, s, strlen( s ) );
}

int fax_command( char * send, char * expect, int fd )
{
    lprintf( L_MESG, "fax_command: send '%s'", send );
    if ( write( fd, send, strlen( send ) ) != strlen( send ) ||
	 write( fd, "\r\n", 2 ) != 2 )
    {
	lprintf( L_ERROR, "fax_command: cannot write" );
	return ERROR;
    }
    return fax_wait_for( expect, fd );
}

/*
  Builds table to swap the low order 4 bits with the high order.

  (Part of the GNU NetFax system! gfax-3.2.1/lib/libfax/swap.c)
*/

static void init_swaptable(swaptable)
     unsigned char *swaptable;
{
    int i, j;

    for (i = 0; i < 256; i++) {
	j = ( ((i & 0x01) << 7) |
	     ((i & 0x02) << 5) |
	     ((i & 0x04) << 3) |
	     ((i & 0x08) << 1) |
	     ((i & 0x10) >> 1) |
	     ((i & 0x20) >> 3) |
	     ((i & 0x40) >> 5) |
	     ((i & 0x80) >> 7) );
	swaptable[i] = j;
    }
}

/* 
  Reverses the low order 8 bits of a byte
*/
unsigned char swap_bits(unsigned char c)
{
    static unsigned char swaptable[256];
    static int swaptable_init = FALSE;

    if (!swaptable_init) {
	init_swaptable(swaptable);
	swaptable_init = TRUE;
    }

    return(swaptable[c]);
}

/* fax_read_byte
 * read one byte from "fd", with buffering
 * caveat: only one fd allowed (only one buffer), no way to flush buffers
 */

int fax_read_byte( int fd, char * c )
{
static char frb_buf[512];
static int  frb_rp = 0;
static int  frb_len = 0;
extern int  errno;			/* don't want to include errno.h */

    if ( frb_rp >= frb_len )
    {
	frb_len = read( fd, frb_buf, sizeof( frb_buf ) );
	if ( frb_len <= 0 )
	{
	    if ( frb_len == 0 ) errno = 0;	/* undefined otherwise */
	    lprintf( L_ERROR, "fax_read_byte: read returned %d", frb_len );
	    return frb_len;
	}
/*!!	lprintf( L_JUNK, "fax_read_byte: read %d bytes", frb_len );*/
	frb_rp = 0;
    }

    *c = frb_buf[ frb_rp++ ];
    return 1;
}
