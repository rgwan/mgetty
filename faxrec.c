#ident "$Id: faxrec.c,v 1.2 1993/03/23 16:44:50 gert Exp $ Gert Doering"

/* faxrec.c - part of the ZyXEL getty
 *
 * this module is used when the modem sends a string that triggers the
 * action "A_FAX" - typically this should be "+FCON".
 *
 * The incoming fax is received, and stored to $FAX_SPOOL_IN (one file per
 * page). After completition, the result is mailed to $MAIL_TO.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>

#include "mgetty.h"
#include "fax_lib.h"

/* all stuff in here was programmed according to a description of the
 * class 2 standard as implemented in the SupraFAX Faxmodem
 */

void fax_notify_mail( int number_of_pages );

void faxrec( void )
{
int pagenum = 0;

    log_level = L_NOISE;	/* log all */

    lprintf( L_NOISE, "fax receiver: entry" );

    /* read: +FTSI:, +FDCS, OK */

    fax_wait_for( "OK", 0 );

    fax_get_pages( 0, &pagenum, FAX_SPOOL_IN );

    lprintf( L_NOISE, "fax receiver: hangup & end" );

    /* send mail to MAIL_TO */
    fax_notify_mail( pagenum );
}

void fax_sig_hangup( void )
{
    signal( SIGHUP, fax_sig_hangup );
    lprintf( L_ERROR, "got hangup! what's this? exiting..." );
    exit(5);
}

static boolean fax_timeout = FALSE;

void fax_sig_alarm( void )
{
    signal( SIGALRM, fax_sig_alarm );
    lprintf( L_MESG, "timeout..." );
    fax_timeout = TRUE;
}

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
	fax_fp = fopen( "/tmp/FAXxxx", "w" );
	if ( fax_fp == NULL )
	{
	    lprintf( L_ERROR, "opening of /tmp/FAXxxx *also* failed!" );
	    return ERROR;
	}
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

	if ( read( fd, &c, 1 ) != 1 )
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

    return NOERROR;
}

/* receive fax pages
 * will return the number of received pages in *pagenum
 * FIXME: evaluate unexpected error return codes
 */

int fax_get_pages( int fd, int * pagenum, char * directory )
{
    *pagenum = 0;

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
	fputc( DC2, stdout );

	/* read page data (into temp file), change <DLE><DLE> to <DLE>,
	   wait for <DLE><ETX> for end of data */

	fax_get_page_data( fd, ++(*pagenum), directory );

	/* read +FPTS:1 +FET 0 / 2 */
	fax_wait_for( "OK", fd );

	/* send command to receive next page
	 * and to release post page response (+FPTS) to remote fax
	 */
	fax_send( "AT+FDR\n", fd );

	/* read: +FCFR, [+FDCS:], CONNECT */
	/* if it was the *last* page, modem will send +FHNG:0 ->
	 * fax_hangup will be set to TRUE
	 */

	fax_wait_for( "CONNECT", fd );
    }
    while ( ! fax_hangup );

    return NOERROR;
}

void fax_notify_mail( int pagenum )
{
FILE  * pipe_fp;
char	buf[100];

    sprintf( buf, "%s %s", MAILER, MAIL_TO );
    pipe_fp = popen( buf, "w" );
    if ( pipe_fp == NULL )
    {
	lprintf( L_ERROR, "cannot open pipe to %s", MAILER );
	return;
    }
    fprintf( pipe_fp, "Subject: incoming fax\n" );
    fprintf( pipe_fp, "To: %s\n", MAIL_TO );
    fprintf( pipe_fp, "From: root (Fax Getty)\n" );
    fprintf( pipe_fp, "\n"
		      "A fax has arrived:\n"
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

    fprintf( pipe_fp, "\n\n... spooled to %s\n", FAX_SPOOL_IN );
    fprintf( pipe_fp, "\n\nregards, your modem subsystem.\n" );
    pclose( pipe_fp );
}

