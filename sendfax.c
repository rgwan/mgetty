#ident "$Id: sendfax.c,v 1.18 1993/06/21 15:30:46 gert Exp $ (c) Gert Doering"

/* sendfax.c
 *
 * Send a Fax using a class 2 faxmodem.
 * Calls routines in faxrec.c and faxlib.c
 *
 * The code is still quite rough, but it works.
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <termio.h>
#include <sys/ioctl.h>
#include <signal.h>

#include "mgetty.h"
#include "fax_lib.h"

/* I don't know *why*, but the ZyXEL wants all bytes reversed */
#define REVERSE 1

char * fac_tel_no;
boolean	verbose = FALSE;

void exit_usage( char * program )
{
    fprintf( stderr,
	     "Usage: %s [-p] [-h header] [-v] <fax-number> <page(s) in g3-format>\n",
	     program );
    exit(1);
}

struct termio fax_termio;

char	lockname[MAXPATH];
int fax_open_device( char * fax_tty )
{
char	device[MAXPATH];
int	fd;

    if ( verbose ) printf( "Trying fax device '/dev/%s'... ", fax_tty );

#ifdef SVR4
    if(!(lock=get_lock_name( lockname, fax_tty))){
      return -1;
    }
#else
      sprintf( lock = lockname, LOCK, fax_tty );
#endif

    if ( makelock( lock ) != SUCCESS )
    {
	if ( verbose ) printf( "locked!\n" );
	lprintf( L_MESG, "cannot lock %s", lock );
	return -1;
    }

    sprintf( device, "/dev/%s", fax_tty );

    if ( ( fd = open( device, O_RDWR | O_NDELAY ) ) == -1 )
    {
	lprintf( L_ERROR, "error opening %s", device );
	if ( verbose ) printf( "cannot open!\n" );
	rmlocks();
	return fd;
    }

    /* unset O_NDELAY (otherwise waiting for characters */
    /* would be "busy waiting", eating up all cpu) */

    if ( fcntl( fd, F_SETFL, O_RDWR ) == -1 )
    {
	lprintf( L_ERROR, "error in fcntl" );
	close( fd );
	if ( verbose ) printf( "cannot fcntl!\n" );
	rmlocks();
	return -1;
    }

    /* initialize baud rate, hardware handshake, ... */
    ioctl( fd, TCGETA, &fax_termio );

    /* welll... some modems do need XOFF/XON flow control,
     * but setting it here would interfere with waiting for an XON
     * later -> we do not set it here
     */
    fax_termio.c_iflag = IXANY;
    fax_termio.c_oflag = 0;

    fax_termio.c_cflag = FAX_SEND_BAUD | CS8 | CREAD | HUPCL | 
			 HARDWARE_HANDSHAKE;

    fax_termio.c_lflag = 0;
    fax_termio.c_line = 0;
    fax_termio.c_cc[VMIN] = 1;
    fax_termio.c_cc[VTIME] = 0;

    if ( ioctl( fd, TCSETAF, &fax_termio ) == -1 )
    {
	lprintf( L_ERROR, "error in ioctl" );
	close( fd );
	if ( verbose ) printf( "cannot ioctl!\n" );
	rmlocks();
	return -1;
    }

    /* reset parameters */
    fax_to_poll = FALSE;

    fax_remote_id[0] = 0;
    fax_param[0] = 0;

    lprintf( L_NOISE, "fax_open_device succeeded, %s -> %d", fax_tty, fd );
    if ( verbose ) printf( "OK.\n" );
    return fd;
}

/* fax_open: loop through all FAX_MODEM_TTYS until fax_open_device()
 * succeeds on one of them; then return file descriptor
 * return "-1" of no open succeeded
 */

int fax_open( void )
{
char fax_ttys[] = FAX_MODEM_TTYS;
char * p, * fax_tty;
int fd;

    p = fax_tty = fax_ttys;
    do
    {
	p = strchr( fax_tty, ':' );
	if ( p != NULL ) *p = 0;
	fd = fax_open_device( fax_tty );
	if ( p != NULL ) *p = ':';
	fax_tty = p+1;
    }
    while ( p != NULL && fd == -1 );
    return fd;
}

void fax_close( int fd )
{
    close( fd );
    rmlocks();
}

sig_t fax_send_timeout()
{
    lprintf( L_ERROR, "timeout!" );
}

char fax_end_of_page[] = { DLE, ETX };

/* fax_send_page - send one complete fax-G3-file to the modem
 *
 * modem has to be in sync, waiting for at+fdt
 * NO page punctuation is transmitted -> caller can concatenate
 * multiple parts onto one page
 */
int fax_send_page( char * g3_file, int fd )
{
int g3fd;
char ch;
char buf[256];
char wbuf[ sizeof(buf) * 2 ];

    lprintf( L_NOISE, "fax_send_page(\"%s\") started...", g3_file );

#ifdef FAX_SEND_USE_IXON
    /* disable output flow control! It would eat the XON otherwise! */
    fax_termio.c_iflag &= ~IXON;
    ioctl( fd, TCSETAW, &fax_termio );
#endif

    /* tell modem that we're ready to send - modem will answer
     * with a couple of "+F..." messages and finally CONNECT and XON
     */

    if ( fax_command( "AT+FDT", "CONNECT", fd ) == ERROR ||
	 fax_hangup_code != 0 )
    {
	lprintf( L_WARN, "AT+FDT -> some error, abort fax send!" );
	return ERROR;
    }

    /* when modem is ready to receive data, it will send us an XON
     * (20 seconds timeout)
     */

    lprintf( L_NOISE, "waiting for XON, got:" );

    signal( SIGALRM, fax_send_timeout );
    alarm( 20 );
    do
    {
	if ( read( fd, &ch, 1 ) != 1 )
	{
	    lprintf( L_ERROR, "waiting for XON" );
	    fprintf( stderr, "error waiting for XON!\n" );
	    fax_close( fd );
	    exit(11);
	}
	lputc( L_NOISE, ch );
    }
    while ( ch != XON );
    alarm(0);

    /* Since some faxmodems (ZyXELs!) do need XON/XOFF flow control
     * we have to enable it here
     */

#ifdef FAX_SEND_USE_IXON
    fax_termio.c_iflag |= IXON;
    ioctl( fd, TCSETAW, &fax_termio );
#endif

    /* send one page */
    lprintf( L_MESG, "sending %s...", g3_file );

    g3fd = open( g3_file, O_RDONLY );
    if ( g3fd == -1 )
    {
	lprintf( L_ERROR, "cannot open %s", g3_file );
	lprintf( L_WARN, "have to send empty page instead" );
    }
    else
    {
	int r, i, w;
	boolean first = TRUE;

	while ( ( r = read( g3fd, buf, 64 ) ) > 0 )
	{
	    i = 0;
	    /* skip over GhostScript / digifaxhigh header */

	    if ( first )
	    {
		first = FALSE;
		if ( r >= 64 && strcmp( buf+1,
					"PC Research, Inc" ) == 0 )
		{
		    lprintf( L_MESG, "skipping over GhostScript header" );
		    i = 64;
		}
	    }

	    for ( w = 0; i < r; i++ )
	    {
#if REVERSE
		wbuf[ w ] = swap_bits( buf[ i ] );
#else
		wbuf[ w ] = buf[ i ];
#endif
		if ( wbuf[ w++ ] == DLE ) wbuf[ w++ ] = DLE;
	    }

	    lprintf( L_JUNK, "read %d, write %d", r, w );

	    if ( write( fd, wbuf, w ) != w )
	    {
		lprintf( L_ERROR, "could not write all %d bytes", w );
	    }

	    /* drain output */
	    /* strange enough, some ISC versions do not like this (???) */
#ifndef ISC
	    ioctl( fd, TCSETAW, &fax_termio );
#endif

	    /* look if there's something to read
	     *
	     * normally there shouldn't be anything, but I have
	     * seen very old ZyXEL releases sending junk and then
	     * failing completely... so this may help when debugging
	     *
	     * Also, if you don't have defined FAX_SEND_USE_IXON,
	     * and your modem insists on xon/xoff flow control, you'll
	     * see these characters [0x11/0x13] here.
	     */

	    while ( check_for_input( fd ) )
	    {
		lprintf( L_NOISE, "input: got" );
		if ( read( fd, &ch, 1 ) != 1 )
		    lprintf( L_ERROR, "read failed" );
		else
		    lputc( L_NOISE, ch );
	    }
	}		/* end while (more g3 data to read) */
    }			/* end if (open file succeeded) */

    /* transmit end of page */
    lprintf( L_NOISE, "sending DLE ETX..." );
    write( fd, fax_end_of_page, sizeof( fax_end_of_page ));

    if ( fax_wait_for( "OK", fd ) == ERROR ) return ERROR;

    return NOERROR;
}

int main( int argc, char ** argv )
{
int argidx;
int fd;
char buf[1000];
char ch;
int i;
boolean fax_poll_req = FALSE;
char * 	fax_page_header = NULL;
char	poll_directory[MAXPATH] = ".";		/* FIXME: parameter */

    /* initialize logging */
    strcpy( log_path, FAX_LOG );
    log_level = L_NOISE;

    while ((ch = getopt(argc, argv, "vx:ph:")) != EOF) {
	switch (ch) {
	case 'v':
	    verbose = TRUE;
	    break;
	case 'x':
	    log_level = atoi(optarg);
	    break;
	case 'p':
	    fax_poll_req = TRUE;
	    break;
	case 'h':
	    fax_page_header = optarg;
	    lprintf( L_MESG, "page header: %s", fax_page_header );
	    break;
	case '?':
	    exit_usage(argv[0]);
	    break;
	}
    }

    argidx = optind;

    if ( argidx == argc )
    {
	exit_usage(argv[0]);
    }
    fac_tel_no = argv[ argidx++ ];

    lprintf( L_MESG, "sending fax to %s", fac_tel_no );

    if ( ! fax_poll_req && argidx == argc )
    {
	exit_usage(argv[0]);
    }

    /* check, if all the arguments passed are normal files and
     * readable
     */
    for ( i=argidx; i<argc; i++ )
    {
	lprintf( L_MESG, "checking %s", argv[i] );
	if ( access( argv[i], R_OK ) == -1 )
	{
	    lprintf( L_ERROR, "cannot access %s", argv[i] );
	    fprintf( stderr, "%s: cannot access %s\n", argv[0], argv[i]);
	    exit(1);
	}
    }

    fd = fax_open();

    if ( fd == -1 )
    {
	lprintf( L_WARN, "cannot open fax device(s)" );
	fprintf( stderr, "%s: cannot access fax device(s) (locked?)\n", argv[0] );
	exit(2);
    }

    if ( fax_command( "AT", "OK", fd ) == ERROR ||
         fax_command( "AT+FCLASS=2;+FLID=\""FAX_STATION_ID"\"", "OK", fd ) == ERROR )
    {
	lprintf( L_ERROR, "cannot initialize faxmodem" );
	fprintf( stderr, "%s: cannot initialize faxmodem\n", argv[0] );
	exit(3);
    }

#if REVERSE
    fax_command( "AT+FDCC=1,5,0,2,0,0,0,0;+FBOR=0", "OK", fd );
#else
    fax_command( "AT+FDCC=1,5,0,2,0,0,0,0;+FBOR=1", "OK", fd );
#endif

    /* tell the modem if we are willing to poll faxes
     */
    if ( fax_poll_req )
    {
	fax_command( "AT+FCIG=\""FAX_STATION_ID"\"", "OK", fd );
	if ( fax_command( "AT+FSPL=1", "OK", fd ) == ERROR )
	{
	    lprintf( L_WARN, "AT+FSPL=1: cannot enable polling" );
	    fprintf( stderr, "Warning: polling is not possible!" );
	    fax_poll_req = FALSE;
	}
    }

    /* set modem to hardware handshake (AT&H3), dial out
     */
    if ( verbose ) { printf( "Dialing %s... ", fac_tel_no ); fflush(stdout); }

    sprintf( buf, "AT&H3D%s", fac_tel_no );
    if ( fax_command( buf, "OK", fd ) == ERROR )
    {
	lprintf( L_WARN, "dial failed (hangup_code=%d)", fax_hangup_code );
	fprintf( stderr, "\n%s: dial %s failed (%s)\n", argv[0], fac_tel_no,
		 fax_hangup_code == FHUP_BUSY? "BUSY" : "ERROR / NO CARRIER");

	/* end program - return codes signals kind of dial failure */
	if ( fax_hangup_code == FHUP_BUSY ) exit(4);
	exit(10);
    }
    if ( verbose ) printf( "OK.\n" );

    /* process all files to send / abort, if Modem sent +FHNG result */

    while ( argidx < argc && ! fax_hangup )
    {
	/* send page header, if requested */
	if ( fax_page_header )
	{
#if 0
	    if ( fax_send_page( fax_page_header, fd ) == ERROR ) break;
#else
	    fprintf( stderr, "WARNING: no page header is transmitted. Does not work yet!\n" );
#endif
	    /* NO page punctuation, we want the next file on the same page
	     */
	}

	/* send page */
	if ( verbose ) printf( "sending '%s'...\n", argv[ argidx ] );
	if ( fax_send_page( argv[ argidx ], fd ) == ERROR ) break;

	argidx++;

        /* transmit page punctuation
	 * (three cases: more pages, last page but polling, last page at all)
	 */

	if ( argidx == argc )		/* was this the last page to send? */
	  if ( fax_poll_req && fax_to_poll )
	    fax_command( "AT+FET=1", "OK", fd );	/* end document */
	  else
	    fax_command( "AT+FET=2", "OK", fd );	/* end session */
	else
	    fax_command( "AT+FET=0", "OK", fd );	/* end page */
    }

    if ( argidx < argc || fax_hangup_code != 0 )
    {
	lprintf( L_WARN, "Failure transmitting %s: +FHNG:%2d",
		 argv[argidx-1], fax_hangup_code );
	fprintf( stderr,
		 "\n%s: FAILED transmitting '%s'.\n"
		 "Transmission error: +FHNG:%2d\n", 
		 argv[0], argv[argidx-1], fax_hangup_code );
	fax_close( fd );
	exit(12);
    }

    /* OK, handle (optional) fax polling now.
     * Fax polling will only be tried if user specified "-p" and the
     * faxmodem sent us a "+FPOLL" response
     */

    if ( fax_poll_req )
    {
    int pagenum = 0;

	if ( verbose ) printf( "starting fax poll\n" );

	if ( ! fax_to_poll )
	{
	    printf( "remote does not have document to poll!\n" );
	}
	else
	{
#ifdef FAX_SEND_USE_IXON
	    /* disable output flow control! It would eat XON/XOFF otherwise! */
	    fax_termio.c_iflag &= ~IXON;
	    ioctl( fd, TCSETAW, &fax_termio );
#endif
	    if ( fax_get_pages( fd, &pagenum, poll_directory ) == ERROR )
	    {
		fprintf( stderr, "warning: polling failed\n" );
		lprintf( L_WARN, "warning: polling failed!" );
	    }
	}
	if ( verbose ) printf( "%d pages successfully polled!\n", pagenum );
    }

    fax_close( fd );
    return 0;
}
