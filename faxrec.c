#ident "$Id: faxrec.c,v 1.37 1994/01/05 04:48:53 gert Exp $ Copyright (c) Gert Doering"
;
/* faxrec.c - part of mgetty+sendfax
 *
 * this module is used when the modem sends a string that triggers the
 * action "A_FAX" - typically this should be "+FCON".
 *
 * The incoming fax is received, and stored to $FAX_SPOOL_IN (one file per
 * page). After completition, the result is mailed to $MAIL_TO.
 * If FAX_NOTIFY_PROGRAM is defined, this program is called with all
 * data about the fax as arguments (see policy.h for a description)
 */

#include <stdio.h>
#ifndef _NOSTDLIB_H
#include <stdlib.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <malloc.h>

#ifndef sun
#include <sys/ioctl.h>
#endif

#include "mgetty.h"
#include "tio.h"
#include "policy.h"
#include "fax_lib.h"

/* all stuff in here was programmed according to a description of the
 * class 2 standard as implemented in the SupraFAX Faxmodem
 */

void fax_notify_mail _PROTO(( int number_of_pages ));
#ifdef FAX_NOTIFY_PROGRAM
void fax_notify_program _PROTO(( int number_of_pages ));
#endif

static char * faxpoll_server_file = NULL;

void faxpoll_server_init( char * f )
{
    faxpoll_server_file = NULL;
    if ( access( f, R_OK ) != 0 )
    {
	lprintf( L_ERROR, "cannot access/read '%s'", f );
    }
    else if ( fax_command( "AT+FLPL=1", "OK", 0 ) == ERROR )
    {
	lprintf( L_WARN, "faxpoll_server_init: no polling available" );
	fax_hangup = 0;
    }
    else
    {
	lprintf( L_NOISE, "faxpoll_server_init: waiting for poll" );
	faxpoll_server_file = f;
    }
}

void faxrec _P1((spool_in), char * spool_in )
{
int pagenum = 0;
TIO tio;

    lprintf( L_NOISE, "fax receiver: entry" );

    /* Setup tty interface
     * Do not set c_cflag, assume that caller has set bit rate,
     * hardware handshake, ... properly
     * For some modems, it's necessary to switch to 19200 bps.
     */

    tio_get( STDIN, &tio );

#ifdef FAX_RECEIVE_USE_B19200
    tio_set_speed( &tio, B19200 );
#endif

    tio_mode_raw( &tio );		/* no input or output post-*/
					/* processing, no signals */
    tio_set_flow_control( STDIN, &tio,
			 (FAXREC_FLOW) & (FLOW_HARD|FLOW_XON_IN) );
    tio_set( STDIN, &tio );

    /* read: +FTSI:, +FDCS, OK */

    fax_wait_for( "OK", 0 );

    fax_get_pages( 0, &pagenum, spool_in );

    /* send polled documents (very simple yet) */
    if ( faxpoll_server_file != NULL && fax_poll_req )
    {
	lprintf( L_AUDIT, "fax poll: send %s...", faxpoll_server_file );
	/* send page */
	fax_send_page( faxpoll_server_file, &tio, 0 );
	/* no more pages */
	fax_command( "AT+FET=2", "OK", 0 );
    }
	
    lprintf( L_NOISE, "fax receiver: hangup & end" );

    /* send mail to MAIL_TO */
    fax_notify_mail( pagenum );

#ifdef FAX_NOTIFY_PROGRAM
    /* notify program */
    fax_notify_program( pagenum );
#endif

    /* write audit information and return (caller will exit() then) */
    lprintf( L_AUDIT, "fax received: id='%s', +FHNG:%03d",
	    fax_remote_id, fax_hangup_code );

}

RETSIGTYPE fax_sig_hangup( )
{
    signal( SIGHUP, fax_sig_hangup );
    /* exit if we have not read "+FHNG:xxx" yet (unexpected hangup) */
    if ( ! fax_hangup )
    {
	lprintf( L_WARN, "how rude, got hangup! exiting..." );
	exit(5);
    }
}

static boolean fax_timeout = FALSE;

RETSIGTYPE fax_sig_alarm( )
{
    signal( SIGALRM, fax_sig_alarm );
    lprintf( L_MESG, "timeout..." );
    fax_timeout = TRUE;
}

static	char *	fax_file_names = NULL;
static	int	fax_fn_size = 0;
static	time_t	faxrec_s_time = 0;

int fax_get_page_data _P3((fd, pagenum, directory), int fd,
			  int pagenum, char * directory )
{
char	temp[MAXPATH];
FILE *	fax_fp;
char	c;
char	WasDLE;
int	ErrorCount = 0;
int	ByteCount = 0;
int i,j;
extern  char * Device;

    /* generate spool file name
     *
     * the format depends on the length of filenames allowed. If only
     * short filenames are allowed, it is f[nf]iiiiiii.jj, iii being
     * kind of a sequence number and jj the page number.
     * if long filenames are allowed, the filename will include the
     * fax id of the sending fax
     * the "iiiiii" part will start repeating after approx. 8 years
     */

#ifdef SHORT_FILENAMES
    sprintf(temp, "%s/f%c%07x%s.%02d", directory,
		 fax_par_d.vr == 0? 'n': 'f',
	         (int) faxrec_s_time & 0xfffffff,
	         &Device[strlen(Device)-2], pagenum );
#else
    /* include sender's fax id - if present - into filename */
    sprintf(temp, "%s/f%c%07x%s-", directory,
		fax_par_d.vr == 0? 'n': 'f',
		(int) faxrec_s_time & 0xfffffff,
		&Device[strlen(Device)]-2 );
    i = strlen(temp);
		
    for ( j=0; fax_remote_id[j] != 0; j++ )
    {
         if ( fax_remote_id[j] == ' ' )
	 {
	     if ( temp[i-1] != '-' ) temp[i++] = '-' ;
	 }
         else if ( fax_remote_id[j] != '"' ) temp[i++] = fax_remote_id[j];
    }
    if ( temp[i-1] == '-' ) i--;
    sprintf( &temp[i], ".%02d", pagenum );
#endif

    fax_fp = fopen( temp, "w" );

    if ( fax_fp == NULL )
    {
	lprintf( L_ERROR, "opening %s failed", temp );
	sprintf( temp, "/tmp/FAX%c%04x.%02d",
		       fax_par_d.vr == 0? 'n': 'f',
		       (int) faxrec_s_time & 0xffff, pagenum );
	fax_fp = fopen( temp, "w" );
	if ( fax_fp == NULL )
	{
	    lprintf( L_ERROR, "opening of %s *also* failed", temp );
	    return ERROR;
	}
    }

    /* store file name in fax_file_names */

    if ( fax_file_names != NULL )
	if ( strlen( temp ) + strlen( fax_file_names ) + 2 > fax_fn_size )
	{
	    fax_fn_size += MAXPATH * 2;
	    fax_file_names = realloc( fax_file_names, fax_fn_size );
	}
    if ( fax_file_names != NULL )
    {
	strcat( fax_file_names, " " );
	strcat( fax_file_names, temp );
    }

    /* install signal handlers */
    signal( SIGALRM, fax_sig_alarm );
    signal( SIGHUP, fax_sig_hangup );
    fax_timeout = FALSE;

    lprintf( L_MESG, "fax_get_page_data: receiving %s...", temp );

    WasDLE = 0;
    do
    {
	/* refresh alarm timer every 1024 bytes
	 * (to refresh it for every byte is considered too expensive)
	 */
	if ( ( ByteCount & 0x3ff ) == 0 )
	{
	    alarm(FAX_PAGE_TIMEOUT);
	}

	if ( fax_read_byte( fd, &c ) != 1 )
	{
	    ErrorCount++;
	    lprintf( L_ERROR, "fax_get_page_data: cannot read from port (%d)!",
	                      ErrorCount );
	    if (ErrorCount > 10) return ERROR;
	}
	ByteCount++;

	if ( !WasDLE )
	{
	    if ( c == DLE ) WasDLE = 1;
	               else fputc( c, fax_fp );
	}
	else	/* WasDLE */
	{
	    if ( c == DLE ) fputc( c, fax_fp );		/* DLE DLE -> DLE */
	    else
	    if ( c == ETX ) break;			/* DLE ETX -> end */
	    
	    WasDLE = 0;
	}
    }
    while ( !fax_timeout );

    alarm(0);

    fclose( fax_fp );

    lprintf( L_MESG, "fax_get_page_data: page end, bytes received: %d", ByteCount);

    if ( fax_timeout )
    {
	lprintf( L_MESG, "fax_get_page_data: aborting receive, timeout!" );
	return ERROR;
    }

    /* change file owner and group (jcp) */
    if ( chown( temp, FAX_IN_OWNER, FAX_IN_GROUP ) != 0 )
    {
	lprintf( L_MESG, "fax_get_page_data: cannot change owner, group" );
    }

    return NOERROR;
}

/* receive fax pages
 * will return the number of received pages in *pagenum
 */

int fax_get_pages _P3( (fd, pagenum, directory),
		       int fd, int * pagenum, char * directory )
{
static const char start_rcv = DC2;

    *pagenum = 0;

    /* allocate memory for fax page file names
     */

    fax_file_names = malloc( fax_fn_size = MAXPATH * 4 );
    if ( fax_file_names != NULL ) fax_file_names[0] = 0;

    /* remember start time (for fax_notify_mail)
     */
    faxrec_s_time = time(NULL);

    if ( fax_poll_req || fax_hangup )
    {
	lprintf( L_MESG, "fax_get_pages: no pages to receive" );
	return NOERROR;
    }

    /* send command for start page receive
     * read: +FCFR:, [+FTSI, +FDCS:], CONNECT
     */

    if ( fax_command( "AT+FDR", "CONNECT", fd ) == ERROR )
    {
	lprintf( L_WARN, "fax_get_pages: cannot start page receive" );
	return ERROR;
    }

    while ( !fax_hangup && !fax_poll_req )	/* page receive loop */
    {
	/* send command for start receive page data */
	lprintf( L_NOISE, "sending DC2" );
	write( fd, &start_rcv, 1);

	/* read page data (into temp file), change <DLE><DLE> to <DLE>,
	   wait for <DLE><ETX> for end of data */

	if ( fax_get_page_data( fd, ++(*pagenum), directory ) == ERROR )
	{
	    fax_hangup_code = -1;
	    return ERROR;
	}

	/* read +FPTS:1 +FET 0 / 2 */

	if ( fax_wait_for( "OK", fd ) == ERROR ) return ERROR;

	/* send command to receive next page
	 * and to release post page response (+FPTS) to remote fax
	 */
	fax_send( "AT+FDR\r\n", fd );

	/* read: +FCFR, [+FDCS:], CONNECT */
	/* if it was the *last* page, modem will send +FHNG:0 ->
	 * fax_hangup will be set to TRUE
	 */

	if ( fax_wait_for( "CONNECT", fd ) == ERROR ) return ERROR;
    }

    return NOERROR;
}

void fax_notify_mail _P1( (pagenum),
			  int pagenum )
{
FILE  * pipe_fp;
char  * file_name, * p;
char	buf[256];
int	r;
time_t	ti;

    lprintf( L_NOISE, "fax_notify_mail: sending mail to: %s", MAIL_TO );

    sprintf( buf, "%s %s >/dev/null 2>&1", MAILER, MAIL_TO );

    pipe_fp = popen( buf, "w" );
    if ( pipe_fp == NULL )
    {
	lprintf( L_ERROR, "fax_notify_mail: cannot open pipe to %s", MAILER );
	return;
    }

#ifdef NEED_MAIL_HEADERS
    fprintf( pipe_fp, "Subject: fax from %s\n", fax_remote_id[0] ?
	               fax_remote_id: "(anonymous sender)" );
    fprintf( pipe_fp, "To: %s\n", MAIL_TO );
    fprintf( pipe_fp, "From: root (Fax Getty)\n" );
    fprintf( pipe_fp, "\n" );
#endif

    fprintf( pipe_fp, "A fax has arrived:\n" );
    fprintf( pipe_fp, "Sender ID: %s\n", fax_remote_id );
    fprintf( pipe_fp, "Pages received: %d\n", pagenum );
    if ( fax_poll_req )
    {
	fprintf( pipe_fp, "Pages sent    : %s\n", faxpoll_server_file );
    }

    fprintf( pipe_fp, "\nCommunication parameters: %s\n", fax_param );
    fprintf( pipe_fp, "    Resolution : %s\n",
		      fax_par_d.vr == 0? "normal" :"fine");
    fprintf( pipe_fp, "    Bit Rate   : %d\n", ( fax_par_d.br+1 ) * 2400 );
    fprintf( pipe_fp, "    Page Width : %d pixels\n", fax_par_d.wd == 0? 1728:
	              ( fax_par_d.wd == 1 ? 2048: 2432 ) );
    fprintf( pipe_fp, "    Page Length: %s\n",
		      fax_par_d.ln == 2? "unlimited":
			   fax_par_d.ln == 1? "B4 (364 mm)" : "A4 (297 mm)" );
    fprintf( pipe_fp, "    Compression: %d (%s)\n", fax_par_d.df, 
	              fax_par_d.df == 0 ? "1d mod Huffman":
	              (fax_par_d.df == 1 ? "2d mod READ": "2d uncompressed") );
    fprintf( pipe_fp, "    Error Corr.: %s\n", fax_par_d.ec? "ECM":"none" );
    fprintf( pipe_fp, "    Scan Time  : %d\n\n", fax_par_d.st );

    ti = time(NULL) - faxrec_s_time;
    fprintf( pipe_fp, "Reception Time : %d:%d\n\n", (int) ti/60, (int) ti%60 );

    if ( fax_hangup_code != 0 )
    {
	fprintf( pipe_fp, "\nThe fax receive was *not* fully successful\n" );
	fprintf( pipe_fp, "The Modem returned +FHNG:%3d\n", fax_hangup_code );
	fprintf( pipe_fp, "\t\t   (%s)\n", fax_strerror( fax_hangup_code ) );
    }

    /* list the spooled fax files (jcp/gd) */

    fprintf( pipe_fp, "\nSpooled G3 fax files:\n\n" );

    p = file_name = fax_file_names;
    
    while ( p != NULL )
    {
	p = strchr( file_name, ' ' );
	if ( p != NULL ) *p = 0;
	fprintf( pipe_fp, "  %s\n", file_name );
	if ( p != NULL ) *p = ' ';
	file_name = p+1;
    }

    fprintf( pipe_fp, "\n\nregards, your modem subsystem.\n" );

    if ( ( r = pclose( pipe_fp ) ) != 0 )
    {
	lprintf( L_WARN, "fax_notify_mail: mailer exit status: %d\n", r );
    }
}

#ifdef FAX_NOTIFY_PROGRAM
void fax_notify_program _P1( (pagenum),
			     int pagenum )
{
int	r;
char *	line;

    line = malloc( fax_fn_size + sizeof( FAX_NOTIFY_PROGRAM) + 100 );
    if ( line == NULL )
    {
	lprintf( L_ERROR, "fax_notify_program: cannot malloc" );
	return;
    }

    /* build command line
     * note: stdout / stderr redirected to console, we don't
     *       want the program talking to the modem
     */
    sprintf( line, "%s %d '%s' %d %s >%s 2>&1 </dev/null",
					 FAX_NOTIFY_PROGRAM,
					 fax_hangup_code,
					 fax_remote_id,
					 pagenum,
					 fax_file_names,
					 CONSOLE);

    lprintf( L_NOISE, "notify: '%.320s'", line );

    switch( fork() )
    {
	case 0:		/* child */
	    /* detach from controlling tty -> no SIGHUP */
	    close( 0 ); close( 1 ); close( 2 );
#ifdef BSD
	    setpgrp( 0, getpid() );
	    if ( ( r = open( "/dev/tty", O_RDWR ) ) >= 0 )
	    {
		ioctl( r, TIOCNOTTY, NULL );
		close( r );
	    }
#else
	    setpgrp();
#endif
	    r = system( line );

	    if ( r != 0 )
		lprintf( L_ERROR, "system() failed" );
	    exit(0);
	case -1:
	    lprintf( L_ERROR, "fork() failed" );
	    break;
    }
    free( line );
}
#endif
