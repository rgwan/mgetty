#ident "$Id: faxrec.c,v 1.11 1993/08/25 12:51:37 gert Exp $ Gert Doering"

/* faxrec.c - part of the ZyXEL getty
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
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <malloc.h>
#include <termio.h>

#include "mgetty.h"
#include "fax_lib.h"

/* all stuff in here was programmed according to a description of the
 * class 2 standard as implemented in the SupraFAX Faxmodem
 */

void fax_notify_mail( int number_of_pages );
#ifdef FAX_NOTIFY_PROGRAM
void fax_notify_program( int number_of_pages );
#endif

void faxrec( char * spool_in )
{
int pagenum = 0;
struct termio termio;

    lprintf( L_NOISE, "fax receiver: entry" );

    /* Setup tty interface
     * Do not set c_cflag, assume that caller has set bit rate,
     * hardware handshake, ... properly
     */

    ioctl( STDIN, TCGETA, &termio );
#ifdef FAX_RECEIVE_USE_IXOFF
    termio.c_iflag = IXOFF;		/* xon/xoff flow control */
#else
    termio.c_iflag = 0;			/* do NOT process input! */
#endif
    termio.c_oflag = OPOST|ONLCR;	/* nl -> cr mapping for modem */
    termio.c_lflag = 0;			/* disable signals and echo! */
    termio.c_cc[VMIN] = 1;
    termio.c_cc[VTIME]= 0;
    ioctl( STDIN, TCSETA, &termio );

    /* read: +FTSI:, +FDCS, OK */

    fax_wait_for( "OK", 0 );

    fax_get_pages( 0, &pagenum, spool_in );

    lprintf( L_NOISE, "fax receiver: hangup & end" );

    /* send mail to MAIL_TO */
    fax_notify_mail( pagenum );

#ifdef FAX_NOTIFY_PROGRAM
    /* notify program */
    fax_notify_program( pagenum );
#endif
}

void fax_sig_hangup( )
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

void fax_sig_alarm( )
{
    signal( SIGALRM, fax_sig_alarm );
    lprintf( L_MESG, "timeout..." );
    fax_timeout = TRUE;
}

char *	fax_file_names = NULL;
int	fax_fn_size = 0;

int fax_get_page_data( int fd, int pagenum, char * directory )
{
char temp[MAXPATH];
FILE * fax_fp;
char c;
char WasDLE;
int ErrorCount = 0;
int ByteCount = 0;

    /* temp file is named: f-{n,f}iiiijjjjjj,
     * n/f means normal / fine, iiii is the current time, jjjjjj is
     * made by mktemp()
     */

    sprintf(temp, "%s/fax%c-%02d.XXXXXX", directory,
		 fax_par_d.vr == 0? 'n': 'f', pagenum );

    fax_fp = fopen( mktemp(temp), "w" );

    if ( fax_fp == NULL )
    {
	lprintf( L_ERROR, "opening %s failed!", temp );
	sprintf( temp, "/tmp/FAX%c-%02d", 
		       fax_par_d.vr == 0? 'n': 'f', pagenum );
	fax_fp = fopen( temp, "w" );
	if ( fax_fp == NULL )
	{
	    lprintf( L_ERROR, "opening of %s *also* failed!", temp );
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

	if ( c == DLE )
	{
	    if ( WasDLE ) fputc( c, fax_fp );	/* DLE DLE -> DLE */
	    WasDLE = 1;
	}
	else
	{
	    fputc( c, fax_fp );
	    if ( c != ETX ) WasDLE = 0;
	}
    }
    while ( ( c != ETX || ! WasDLE ) && !fax_timeout );

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
 * FIXME: evaluate unexpected error return codes
 */

int fax_get_pages( int fd, int * pagenum, char * directory )
{
static const char start_rcv = DC2;

    *pagenum = 0;

    /* allocate memory for fax page file names
     * (error checking is done in fax_get_page_data)
     */

    fax_file_names = malloc( fax_fn_size = MAXPATH * 4 );

    /* send command for start page receive
     * read: +FCFR:, [+FTSI, +FDCS:], CONNECT
     */

    if ( fax_command( "AT+FDR", "CONNECT", fd ) == ERROR )
    {
	lprintf( L_WARN, "fax_get_pages: cannot start page receive" );
	return ERROR;
    }

    do		/* page receive loop */
    {
	/* send command for start receive page data */
	lprintf( L_NOISE, "sending DC2" );
	write( fd, &start_rcv, 1);

	/* read page data (into temp file), change <DLE><DLE> to <DLE>,
	   wait for <DLE><ETX> for end of data */

	if ( fax_get_page_data( fd, ++(*pagenum), directory ) == ERROR )
	{
	    return ERROR;
	}

	/* read +FPTS:1 +FET 0 / 2 */

	if ( fax_wait_for( "OK", fd ) == ERROR ) return ERROR;

	/* send command to receive next page
	 * and to release post page response (+FPTS) to remote fax
	 */
	fax_send( "AT+FDR\n", fd );

	/* read: +FCFR, [+FDCS:], CONNECT */
	/* if it was the *last* page, modem will send +FHNG:0 ->
	 * fax_hangup will be set to TRUE
	 */

	if ( fax_wait_for( "CONNECT", fd ) == ERROR ) return ERROR;
    }
    while ( ! fax_hangup );

    return NOERROR;
}

void fax_notify_mail( int pagenum )
{
FILE  * pipe_fp;
char  * file_name, * p;
char	buf[256];
int	r;

    lprintf( L_NOISE, "fax_notify_mail: sending mail to: %s", MAIL_TO );

    sprintf( buf, "%s %s >/dev/null 2>&1", MAILER, MAIL_TO );

    pipe_fp = popen( buf, "w" );
    if ( pipe_fp == NULL )
    {
	lprintf( L_ERROR, "fax_notify_mail: cannot open pipe to %s", MAILER );
	return;
    }

#ifdef NEED_MAIL_HEADERS
    fprintf( pipe_fp, "Subject: incoming fax\n" );
    fprintf( pipe_fp, "To: %s\n", MAIL_TO );
    fprintf( pipe_fp, "From: root (Fax Getty)\n" );
    fprintf( pipe_fp, "\n" );
#endif

    fprintf( pipe_fp, "A fax has arrived:\n"
		      "Pages: %d\n"
		      "Sender ID: %s\n", pagenum, fax_remote_id );
    fprintf( pipe_fp, "Communication parameters: %s\n", fax_param );
    fprintf( pipe_fp, "    Resolution : %s\n"
		      "    Bit Rate   : %d\n"
		      "    Page Width : %d\n"
		      "    Page Length: %s\n"
		      "    Compression: %d\n"
		      "    Error Corr : %d (none)\n"
		      "    BFT        : %d (disabled)\n"
		      "    Scan Time  : %d\n\n",
		      fax_par_d.vr == 0? "normal" :"fine",
		      (fax_par_d.br+1 ) * 2400,
		      fax_par_d.wd,
		      fax_par_d.ln == 2? "unlimited":
			   fax_par_d.ln == 1? "B4 (364 mm)" : "A4 (297 mm)",
		      fax_par_d.df,
		      fax_par_d.ec,
		      fax_par_d.df,
		      fax_par_d.st );

    if ( fax_hangup_code != 0 )
    {
	fprintf( pipe_fp, "\nThe fax receive was *not* fully successful\n"
		 	  "The Modem returned +FHNG:%3d\n", fax_hangup_code );
    }

    /* list the spooled fax files (jcp/gd) */

    fprintf( pipe_fp, "\nSpooled G3 fax files:\n\n" );

    p = file_name = fax_file_names;
    do
    {
	p = strchr( file_name, ' ' );
	if ( p != NULL ) *p = 0;
	fprintf( pipe_fp, "  %s\n", file_name );
	if ( p != NULL ) *p = ' ';
	file_name = p+1;
    }
    while ( p != NULL );

    fprintf( pipe_fp, "\n\nregards, your modem subsystem.\n" );

    if ( ( r = pclose( pipe_fp ) ) != 0 )
    {
	lprintf( L_WARN, "fax_notify_mail: mailer exit status: %d\n", r );
    }
}

#ifdef FAX_NOTIFY_PROGRAM
void fax_notify_program( int pagenum )
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
    sprintf( line, "%s %d '%s' %d %s >%s 2>&1",
					 FAX_NOTIFY_PROGRAM,
					 fax_hangup_code,
					 fax_remote_id,
					 pagenum,
					 fax_file_names,
					 CONSOLE);

    lprintf( L_NOISE, "notify: '%s'", line );

    r = system( line );

    if ( r != 0 )
	lprintf( L_ERROR, "system() failed" );

    free( line );
}
#endif
