#ident "$Id: faxrec.c,v 3.17 1996/12/15 16:45:42 gert Exp $ Copyright (c) Gert Doering"

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
#include "syslibs.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/stat.h>

#ifndef sunos4
#include <sys/ioctl.h>
#endif

#include "mgetty.h"
#include "tio.h"
#include "policy.h"
#include "fax_lib.h"

       time_t call_start;		/* set in mgetty.c */
static time_t call_done;

time_t	time _PROTO(( long * tloc ));

/* all stuff in here was programmed according to a description of the
 * class 2 standard as implemented in the SupraFAX Faxmodem
 */

void fax_notify_mail _PROTO(( int number_of_pages, char * mail_to ));
#ifdef FAX_NOTIFY_PROGRAM
void fax_notify_program _PROTO(( int number_of_pages ));
#endif
void faxpoll_send_pages _PROTO(( int fd, TIO * tio, char * pollfile));

char * faxpoll_server_file = NULL;

void faxrec _P6((spool_in, switchbd, uid, gid, mode, mail_to),
		char * spool_in, unsigned int switchbd,
		int uid, int gid, int mode, char * mail_to)
{
int pagenum = 0;
TIO tio;
extern  char * Device;

    lprintf( L_NOISE, "fax receiver: entry" );

    /* Setup tty interface
     * Do not set c_cflag, assume that caller has set bit rate,
     * hardware handshake, ... properly
     * For some modems, it's necessary to switch to 19200 bps.
     */

#ifdef FAX_USRobotics
    /* the ultra smart USR modems do it in yet another way... */
    fax_wait_for( "OK", 0 );
#endif

    tio_get( STDIN, &tio );

    /* switch bit rates, if necessary */
    if ( switchbd != 0 ) tio_set_speed( &tio, switchbd );

    tio_mode_raw( &tio );		/* no input or output post-*/
					/* processing, no signals */
    tio_set( STDIN, &tio );

    /* read: +FTSI:, +FDCS, OK */

#ifndef FAX_USRobotics
    fax_wait_for( "OK", 0 );
#endif

    /* if the "switchbd" flag is set wrongly, the fax_wait_for() command
     * will time out -> write a warning to the log file and give up
     */
    if ( fax_hangup_code == FHUP_TIMEOUT )
    {
	lprintf( L_WARN, ">> The problem seen above might be caused by a wrong value of the" );
	lprintf( L_WARN, ">> 'switchbd' option in 'mgetty.config' (currently set to '%d')", switchbd );

	if ( switchbd > 0 && switchbd != 19200 )
		lprintf( L_WARN, ">> try using 'switchbd 19200' or 'switchbd 0'");
	else if ( switchbd > 0 )
		lprintf( L_WARN, ">> try using 'switchbd 0'" );
	else    lprintf( L_WARN, ">> try using 'switchbd 19200'" );

	fax_hangup = 1;
    }

    /* *now* set flow control (we could have set it earlier, but on SunOS,
     * enabling CRTSCTS while DCD is low will make the port hang)
     */
    tio_set_flow_control( STDIN, &tio,
			 (FAXREC_FLOW) & (FLOW_HARD|FLOW_XON_IN) );
    tio_set( STDIN, &tio );

    /* tell modem about the flow control used (+FLO=...) */
    fax_set_flowcontrol( STDIN, (FAXREC_FLOW) & FLOW_HARD );

    fax_get_pages( 0, &pagenum, spool_in, uid, gid, mode );

    /* send polled documents (very simple yet) */
    if ( faxpoll_server_file != NULL && fax_poll_req )
    {
	lprintf( L_MESG, "starting fax poll send..." );
	
	faxpoll_send_pages( 0, &tio, faxpoll_server_file );
    }

    call_done = time(NULL);
	
    lprintf( L_NOISE, "fax receiver: hangup & end" );

    /* send mail to MAIL_TO */
    if ( mail_to != NULL && strlen(mail_to) != 0 )
        fax_notify_mail( pagenum, mail_to );

#ifdef FAX_NOTIFY_PROGRAM
    /* notify program */
    fax_notify_program( pagenum );
#endif

    call_done = call_done - call_start;
    /* write audit information and return (caller will exit() then) */
    lprintf( L_AUDIT,
"fax dev=%s, pid=%d, caller=%s, name='%s', id='%s', +FHNG=%03d, pages=%d, time=%02d:%02d:%02d\n",
	Device, getpid(), CallerId, CallName, fax_remote_id, fax_hangup_code, pagenum,
	call_done / 3600, (call_done / 60) % 60, call_done % 60);

    /* some modems require a manual hangup, with a pause before it. Notably
       this is the creatix fax/voice modem, which is quite widespread,
       unfortunately... */

    delay(1500);
    mdm_command( "ATH0", STDIN );
}

RETSIGTYPE fax_sig_hangup(SIG_HDLR_ARGS)
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

RETSIGTYPE fax_sig_alarm(SIG_HDLR_ARGS)
{
    signal( SIGALRM, fax_sig_alarm );
    lprintf( L_MESG, "timeout..." );
    fax_timeout = TRUE;
}

static	char *	fax_file_names = NULL;
static	int	fax_fn_size = 0;

int fax_get_page_data _P6((fd, pagenum, directory, uid, gid, file_mode),
			  int fd, int pagenum, char * directory,
			  int uid, int gid, int file_mode )
{
char	temp[MAXPATH];
FILE *	fax_fp;
char	c;
char	WasDLE;
int	ErrorCount = 0;
int	ByteCount = 0;
int i,j;
extern  char * Device;
char	DevId[3];

    /* call_start is only initialized if we're called from mgetty, not
     * when fax polling (sendfax) or from another getty (contrib/faxin).
     * So, eventually set it here
     */

    if ( call_start == 0L ) call_start = time( NULL );

    /* generate spool file name
     *
     * the format depends on the length of filenames allowed. If only
     * short filenames are allowed, it is f[nf]iiiiiii.jj, iii being
     * kind of a sequence number and jj the page number.
     * if long filenames are allowed, the filename will include the
     * fax id of the sending fax
     * the "iiiiii" part will start repeating after approx. 8 years
     */

    /* on some systems -- solaris2 -- the device name may look like
     * "/dev/cub/a", so use "ba" instead of "/a" for the device id
     */

    strcpy( DevId, &Device[strlen(Device)-2] );
    if ( DevId[0] == '/' ) DevId[0] = Device[strlen(Device)-3];
    if ( DevId[0] == '/' ) DevId[0] = '-';

#ifdef SHORT_FILENAMES
    sprintf(temp, "%s/f%c%07x%s.%02d", directory,
		 fax_par_d.vr == 0? 'n': 'f',
	         (int) call_start & 0xfffffff,
	         DevId, pagenum );
#else
    /* include sender's fax id - if present - into filename */
    sprintf(temp, "%s/f%c%07x%s-", directory,
		fax_par_d.vr == 0? 'n': 'f',
		(int) call_start & 0xfffffff,
		DevId );
    i = strlen(temp);

    /* filter out characters that will give problems in the shell */
    for ( j=0; fax_remote_id[j] != 0; j++ )
    {
	char c = fax_remote_id[j];
         if ( c == ' ' || c == '/' || c == '\\'|| c == '&' || c == ';' ||
	      c == '(' || c == ')' || c == '>' || c == '<' || c == '|' ||
	      c == '?' || c == '*' || c == '\''|| c == '"' || c == '`' )
	 {
	     if ( temp[i-1] != '-' ) temp[i++] = '-' ;
	 }
         else if ( c != '"' && c != '\'' ) temp[i++] = c;
    }
    if ( temp[i-1] == '-' ) i--;
    sprintf( &temp[i], ".%02d", pagenum );
#endif

    if ( checkspace(directory) )
	fax_fp = fopen( temp, "w" );
    else
    {
	lprintf( L_ERROR, "Not enough space on %s for fax reception", directory);
	fax_fp = NULL;
    }

    if ( fax_fp == NULL )
    {
	lprintf( L_ERROR, "opening %s failed", temp );
	sprintf( temp, "/tmp/FAX%c%04x.%02d",
		       fax_par_d.vr == 0? 'n': 'f',
		       (int) call_start & 0xffff, pagenum );

	if ( checkspace("/tmp") )
	    fax_fp = fopen( temp, "w" );
	else
	{
	    lprintf( L_ERROR, "Not enough space on /tmp for fax reception - dropping line");
	    return ERROR;
	}
	    
	if ( fax_fp == NULL )
	{
	    lprintf( L_ERROR, "opening of %s *also* failed - giving up", temp );
	    return ERROR;
	}
    }

    /* do permission and owner changes as soon as possible -- security */

    /* change file mode */
    if ( file_mode != -1 &&
	 chmod( temp, file_mode ) != 0 ) 
    {
	lprintf( L_ERROR, "fax_get_page_data: cannot change file mode" );
    }

    /* change file owner and group (jcp) */
    if ( uid != -1 &&
	 chown( temp, (uid_t) uid, (gid_t) gid ) != 0 )
    {
	lprintf( L_ERROR, "fax_get_page_data: cannot change owner, group" );
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

    WasDLE = 0;

    /* skip any leading garbage
     * it's reasonable to assume that a fax will start with a zero
     * byte (actually, T.4 requires it).
     * This has the additional benefit that we'll see error messages
     */

    lprintf( L_NOISE, "fax_get_page_data: wait for EOL, got: " );
    alarm( FAX_PAGE_TIMEOUT );

    while ( !fax_timeout )
    {
	if ( mdm_read_byte( fd, &c ) != 1 )
	{
	    lprintf( L_ERROR, "error waiting for page start" );
	    return ERROR;
	}
	lputc( L_NOISE, c );
	if ( c == 0 )   { fputc( c, fax_fp ); break; }
	if ( c == DLE ) { WasDLE = 1; break; }
    }

    lprintf( L_MESG, "fax_get_page_data: receiving %s...", temp );

    while ( !fax_timeout )
    {
	/* refresh alarm timer every 1024 bytes
	 * (to refresh it for every byte is far too expensive)
	 */
	if ( ( ByteCount & 0x3ff ) == 0 )
	{
	    alarm(FAX_PAGE_TIMEOUT);
	}

	if ( mdm_read_byte( fd, &c ) != 1 )
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
	      if ( c == SUB )				/* DLE SUB -> 2x DLE */
	    {						/* (class 2.0) */
		fputc( DLE, fax_fp ); fputc( DLE, fax_fp );
	    }
	    else
	      if ( c == ETX ) break;			/* DLE ETX -> end */
	    
	    WasDLE = 0;
	}
    }

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
 */

int fax_get_pages _P6( (fd, pagenum, directory, uid, gid, mode ),
		       int fd, int * pagenum, char * directory,
		       int uid, int gid, int mode )
{
static const char start_rcv = DC2;

    *pagenum = 0;

    /* allocate memory for fax page file names
     */

    fax_file_names = malloc( fax_fn_size = MAXPATH * 4 );
    if ( fax_file_names != NULL ) fax_file_names[0] = 0;

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

	if ( fax_get_page_data( fd, ++(*pagenum), directory,
			        uid, gid, mode ) == ERROR )
	{
	    fax_hangup_code = -1;
	    return ERROR;
	}

	/* read +FPTS:1 +FET 0 / 2 */

	if ( fax_wait_for( "OK", fd ) == ERROR ) return ERROR;

	/* check line count and bad line count. If page too short (less
	 * than 50 lines) or bad line count too high (> lc/5), reject
	 * page (+FPS=2, MPS, page bad - retrain requested)
	 */

	if ( fhs_details &&
	    ( fhs_lc < 50 || fhs_blc > (fhs_lc/10)+30 || fhs_blc > 500 ) )
	{
	    lprintf( L_WARN, "Page doesn't look good, request retrain (MPS)" );
	    fax_command( "AT+FPS=2", "OK", fd );
	}

	/* send command to receive next page
	 * and to release post page response (+FP[T]S) to remote fax
	 */
	fax_send( "AT+FDR", fd );

	/* read: +FCFR, [+FDCS:], CONNECT */
	/* if it was the *last* page, modem will send +FHNG:0 ->
	 * fax_hangup will be set to TRUE
	 */

	if ( fax_wait_for( "CONNECT", fd ) == ERROR ) return ERROR;
    }

    return NOERROR;
}

void fax_notify_mail _P2( (pagenum, mail_to),
			  int pagenum, char * mail_to )
{
FILE  * pipe_fp;
char  * file_name, * p;
char	buf[256];
int	r;
time_t	ti;
extern  char * Device;

    lprintf( L_NOISE, "fax_notify_mail: sending mail to: %s", mail_to );

    sprintf( buf, "%s %s >/dev/null 2>&1", MAILER, mail_to );

    pipe_fp = popen( buf, "w" );
    if ( pipe_fp == NULL )
    {
	lprintf( L_ERROR, "fax_notify_mail: cannot open pipe to %s", MAILER );
	return;
    }

#ifdef NEED_MAIL_HEADERS
    fprintf( pipe_fp, "Subject: fax from %s\n", fax_remote_id[0] ?
	               fax_remote_id: "(anonymous sender)" );
    fprintf( pipe_fp, "To: %s\n", mail_to );
    fprintf( pipe_fp, "From: root (Fax Getty)\n" );
    fprintf( pipe_fp, "\n" );
#endif

    if ( fax_hangup_code == 0 )
	if ( pagenum != 0 || !fax_poll_req )
	    fprintf( pipe_fp, "A fax was successfully received:\n" );
	else
	    fprintf( pipe_fp, "A to-be-polled fax was successfully sent:\n" );
    else
        fprintf( pipe_fp, "An incoming fax transmission failed (+FHNG:%3d):\n",
                 fax_hangup_code );

    fprintf( pipe_fp, "Sender ID: %s\n", fax_remote_id );
    fprintf( pipe_fp, "Pages received: %d\n", pagenum );
    if ( fax_poll_req )
    {
	fprintf( pipe_fp, "Pages sent    : %s\n", faxpoll_server_file );
    }

    fprintf( pipe_fp, "\nModem device: %s\n", Device );
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

    ti = call_done - call_start;	/* time spent */

    fprintf( pipe_fp, "Reception Time : %02d:%02d\n\n", (int) ti/60, (int) ti%60 );

    if ( fax_hangup_code != 0 )
    {
	fprintf( pipe_fp, "\nThe fax receive was *not* fully successful\n" );
	fprintf( pipe_fp, "The Modem returned +FHNG:%3d\n", fax_hangup_code );
	fprintf( pipe_fp, "\t\t   (%s)\n", fax_strerror( fax_hangup_code ) );
    }

    /* list the spooled fax files (jcp/gd) */

    if ( pagenum != 0 )
    {
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

    if ( fax_file_names == NULL ) fax_file_names="";

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
#if defined(BSD) || defined(sunos4)
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

void faxpoll_send_pages _P3( (fd, tio, pollfile),
			     int fd, TIO * tio, char * pollfile )
{
    FILE * fp;
    char buf[MAXPATH];
    char * file;
    char * fgetline _PROTO(( FILE * fp ));
    int    tries;

    fp = fopen( pollfile, "r" );
    if ( fp == NULL )
    {
	lprintf( L_ERROR, "can't open %s", pollfile ); return;
    }

    /* for historical reasons: if the file starts with "0x00",
       assume it's not a text file but a G3 file
     */

    if ( fread( buf, 1, 1, fp ) != 1 || buf[0] == 0 )
    {
	fclose( fp );
	
	lprintf( L_MESG, "fax poll: %s is (likely) G3 file", pollfile );

	/* send page, no more pages to follow */
	fax_send_page( faxpoll_server_file, NULL, tio, pp_eop, fd );

	return;
    }

    /* read line by line, send as separate pages.
     * comments and continuation lines allowed
     */
    rewind( fp );

    file = fgetline( fp );

    while ( file != NULL && !fax_hangup )
    {
	/* copy filename (we need to know *before* sending the file
	   whether it's the last one, and fgetline() uses a static buffer)
	 */
	
	strncpy( buf, file, sizeof(buf)-1 );
	buf[sizeof(buf)-1] = 0;

	file = fgetline( fp );

	lprintf( L_MESG, "fax poll: send %s...", buf );

	fax_page_tx_status = -1;
	tries = 0;

	/* send file, retransmit (once) if RTN received */
	do
	{
	    if ( file == NULL )		/* last page */
	        fax_send_page( buf, NULL, tio, pp_eop, fd );
	    else			/* not the very last */
	        fax_send_page( buf, NULL, tio, pp_mps, fd );
	    tries++;

	    if ( fax_hangup )
	    {
		lprintf( L_WARN, "fax poll failed: +FHNG:%d (%s)",
			 fax_hangup_code, fax_strerror(fax_hangup_code));
		break;
	    }
	    if ( fax_page_tx_status != 1 )
	        lprintf( L_WARN, "fax poll: +FPS: %d", fax_page_tx_status );
	}
	while( fax_page_tx_status == 2 && tries < 2 );
    }
    fclose( fp );
}
