#ident "$Id: sendfax.c,v 1.2 1993/03/14 15:38:04 gert Exp $ (c) Gert Doering"

/* sendfax.c
 *
 * Send a Fax using a class 2 faxmodem.
 * Calls routines in faxrec.c and faxlib.c
 *
 * The code is still quite rough, but it works.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <termio.h>
#include <sys/ioctl.h>

#include "mgetty.h"
#include "fax_lib.h"

/* I don't know *why*, but the ZyXEL wants all bytes reversed */
#define REVERSE 1

char * fac_tel_no;

void exit_usage( char * program )
{
    fprintf( stderr,
	     "%s: usage is\n%s [-p] <fax-number> <page(s) in g3-format>\n",
	     program, program );
    exit(1);
}

/* FIXME: handle multiple faxmodems here! */

struct termio fax_termio;

char	lockname[MAXPATH];
int fax_open( void )
{
char *	fax_tty = FAX_MODEM_TTY;
char	device[MAXPATH];
int	fd;

    sprintf( lock = lockname, LOCK, fax_tty );
    if ( makelock( lock ) != SUCCESS )
    {
	lprintf( L_MESG, "cannot lock %s", lock );
	return -1;
    }

    sprintf( device, "/dev/%s", fax_tty );

    if ( ( fd = open( device, O_RDWR | O_NDELAY ) ) == -1 )
    {
	lprintf( L_ERROR, "error opening %s", device );
	return fd;
    }

    /* unset O_NDELAY (otherwise waiting for characters */
    /* would be "busy waiting", eating up all cpu) */

    if ( fcntl( fd, F_SETFL, O_RDWR ) == -1 )
    {
	lprintf( L_ERROR, "error in fcntl" );
	close( fd );
	return -1;
    }

    /* initialize baud rate, hardware handshake, ... */
    ioctl( fd, TCGETA, &fax_termio );

    fax_termio.c_iflag = IXANY;
    fax_termio.c_oflag = 0;
#ifdef linux
    fax_termio.c_cflag = FAX_SEND_BAUD | CS8 | CREAD | HUPCL | CLOCAL |
			 CRTSCTS;
#else
    fax_termio.c_cflag = FAX_SEND_BAUD | CS8 | CREAD | HUPCL | CLOCAL |
			 RTSFLOW | CTSFLOW;
#endif
    fax_termio.c_lflag = 0;
    fax_termio.c_line = 0;
    fax_termio.c_cc[VMIN] = 1;
    fax_termio.c_cc[VTIME] = 0;

    if ( ioctl( fd, TCSETAF, &fax_termio ) == -1 )
    {
	lprintf( L_ERROR, "error in ioctl" );
	close( fd );
	return -1;
    }

    /* reset parameters */
    fax_to_poll = FALSE;

    fax_remote_id[0] = 0;
    fax_param[0] = 0;

    lprintf( L_NOISE, "fax_open succeeded, %s -> %d", fax_tty, fd );
    return fd;
}

void fax_close( int fd )
{
    close( fd );
    rmlocks();
}

char fax_end_of_page[] = { DLE, ETX };

int main( int argc, char ** argv )
{
int argidx;
int fd, rfd;
char buf[1000];
char wbuf[sizeof(buf)*2];
char ch;
int i;
boolean fax_poll_req = FALSE;
char	poll_directory[MAXPATH] = ".";		/* FIXME: parameter */

    /* initialize logging */
    strcpy( log_path, FAX_LOG );
    log_level = L_NOISE;

    while ((ch = getopt(argc, argv, "x:p")) != EOF) {
	switch (ch) {
	case 'x':
	    log_level = atoi(optarg);
	    break;
	case 'p':
	    fax_poll_req = TRUE;
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
	lprintf( L_WARN, "cannot open fax device" );
	fprintf( stderr, "%s: cannot access fax device (locked?)\n", argv[0] );
	exit(2);
    }

    /* FIXMEEEEE: return codes! */

    if ( fax_command( "AT", "OK", fd ) == ERROR ||
         fax_command( "AT+FCLASS=2;+FLID=\""FAX_STATION_ID"\"", "OK", fd ) == ERROR )
    {
	lprintf( L_ERROR, "cannot initialize faxmodem" );
	fprintf( stderr, "%s: cannot initialize faxmodem\n", argv[0] );
	exit(2);
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

    sprintf( buf, "AT&K4D%s", fac_tel_no );
    if ( fax_command( buf, "OK", fd ) == ERROR )
    {
	lprintf( L_WARN, "dial failed" );
	fprintf( stderr, "%s: dial %s failed\n", argv[0], fac_tel_no );
	exit(3);
    }

    /* process all files to send / abort, if Modem sent +FHNG result */

    while ( argidx < argc && ! fax_hangup )
    {
	/* tell modem that we're ready to send - modem will answer
	 * with a couple of "+F..." messages and finally CONNECT
	 */

	if ( fax_command( "AT+FDT", "CONNECT", fd ) == ERROR )
	{
	    lprintf( L_WARN, "AT+FDT -> some error, abort fax send!" );
	    break;
	}

	/* when modem is ready to receive data, it will send us an XON
	 */

	lprintf( L_NOISE, "waiting for XON, got:" );

	do
	{
	    if ( read( fd, &ch, 1 ) != 1 )
	    {
		lprintf( L_ERROR, "waiting for XON" );
		fax_close( fd );
		exit(10);
	    }
	    lputc( L_NOISE, ch );
	}
	while ( ch != XON );

	/* enable polling */
	fax_termio.c_cc[VMIN] = 0;
	ioctl( fd, TCSETAW, &fax_termio );

	/* send one page */
	lprintf( L_MESG, "sending %s...", argv[argidx] );

	rfd = open( argv[ argidx ], O_RDONLY );
	if ( rfd == -1 )
	{
	    lprintf( L_ERROR, "cannot open %s", argv[ argidx ] );
	}
	else
	{
	    int r, i, w;
	    boolean first = TRUE;

	    while ( ( r = read( rfd, buf, 64 ) ) > 0 )
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

		lprintf(L_NOISE,"read %d, write %d", r, w );

		if ( write( fd, wbuf, w ) != w )
		{
		    lprintf( L_ERROR, "did not write %d bytes", w );
		}

		/* drain output */
		ioctl( fd, TCSETAW, &fax_termio );

		if ( read( fd, &ch, 1 ) == 1 )
		{
		    lprintf( L_NOISE, "input: read" );
		    lputc( L_NOISE, ch );
		    if ( ch == XOFF )
		    {
			lprintf( L_NOISE, "got XOFF, wait 2 seconds" );
			sleep(2);
		    }
		}
	    }

	    /* disable polling */
	    fax_termio.c_cc[VMIN] = 1;
	    ioctl( fd, TCSETAW, &fax_termio );

	    /* end of page */
	    lprintf( L_NOISE, "sending DLE ETX..." );
	    write( fd, fax_end_of_page, sizeof( fax_end_of_page ));
	}

	fax_wait_for( "OK", fd );
	argidx++;

        /* transmit page punctuation
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
		 "%s: FAILED transmitting '%s'.\n"
		 "Transmission error: +FHNG:%2d\n", 
		 argv[0], argv[argidx-1], fax_hangup_code );
	fax_close( fd );
	exit(10);
    }

    /* OK, handle (optional) fax polling now.
     * Fax polling will only be tried if user specified "-p" and the
     * faxmodem sent us a "+FPOLL" response
     */

    if ( fax_poll_req )
    {
    int pagenum;

	if ( ! fax_to_poll )
	{
	    printf( "remote does not have document to poll!\n" );
	}

	/* FIXME: this should go to an else branch */
	/* try anyway - The ZyXEL seems to never send a +FPOLL */

	if ( fax_get_pages( fd, &pagenum, poll_directory ) == ERROR )
	{
	    fprintf( stderr, "warning: polling failed!" );
	    lprintf( L_WARN, "warning: polling failed!" );
	}
	printf( "%d pages successfully polled!\n", pagenum );
    }

    fax_close( fd );
    return 0;
}
