#ident "$Id: sendfax.c,v 1.49 1993/12/18 19:35:21 gert Exp $ Copyright (c) Gert Doering"
;
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
#ifndef _NOSTDLIB_H
#include <stdlib.h>
#endif
#ifndef sun
#include <sys/ioctl.h>
#endif
#include <signal.h>

#include "mgetty.h"
#include "tio.h"
#include "policy.h"
#include "fax_lib.h"

/* I don't know *why*, but the ZyXEL wants all bytes reversed */
#define REVERSE 1

char * fac_tel_no;
boolean	verbose = FALSE;

void exit_usage _P1( (program),
		     char * program )
{
    fprintf( stderr,
	     "usage: %s [options] <fax-number> <page(s) in g3-format>\n", program);
    fprintf( stderr,
	     "\tvalid options: -p, -h <header>, -v, -l <device(s)>, -x <debug>, -n\n");
    exit(1);
}

TIO fax_tio;
char * Device;

int fax_open_device _P1( (fax_tty),
			 char * fax_tty )
{
char	device[MAXPATH];
int	fd;

    if ( verbose ) printf( "Trying fax device '/dev/%s'... ", fax_tty );

    if ( makelock( fax_tty ) != SUCCESS )
    {
	if ( verbose ) printf( "locked!\n" );
	lprintf( L_MESG, "cannot lock %s", fax_tty );
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

    /* make device name externally visible (faxrec()) */
    Device = fax_tty;

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
    tio_get( fd, &fax_tio );

    /* even if we use a modem that requires Xon/Xoff flow control,
     * do *not* enable it here - it would interefere with the Xon
     * received at the top of a page.
     */
    tio_mode_sane( &fax_tio, TRUE );
    tio_set_speed( &fax_tio, FAX_SEND_BAUD );
    tio_mode_raw( &fax_tio );
#ifdef sun
    /* sunos does not rx with RTSCTS unless carrier present */
    tio_set_flow_control( fd, &fax_tio, FLOW_NONE );
#else
    tio_set_flow_control( fd, &fax_tio, (FAXSEND_FLOW) & FLOW_HARD );
#endif
    
    if ( tio_set( fd, &fax_tio ) == ERROR )
    {
	lprintf( L_ERROR, "error in tio_set" );
	close( fd );
	if ( verbose ) printf( "cannot set termio values!\n" );
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

/* fax_open: loop through all devices in fax_ttys until fax_open_device()
 * succeeds on one of them; then return file descriptor
 * return "-1" of no open succeeded (all locked, permission denied, ...)
 */

int fax_open _P1( (fax_ttys),
	      char * fax_ttys )
{
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

/* finish off - close modem device, rm lockfile */

void fax_close _P1( (fd),
		    int fd )
{
    fax_send( "AT+FCLASS=0\r\n", fd );
    delay(100);
    close( fd );
    rmlocks();
}

RETSIGTYPE fax_sig_goodbye( int signo )
{
    lprintf( L_AUDIT, "got signal %d, exiting...", signo );
    rmlocks();
    exit(15);				/* will close the fax device */
}

RETSIGTYPE fax_send_timeout()
{
    lprintf( L_ERROR, "timeout!" );
}

/* fax_send_page - send one complete fax-G3-file to the modem
 *
 * modem has to be in sync, waiting for at+fdt
 * NO page punctuation is transmitted -> caller can concatenate
 * multiple parts onto one page
 */
int fax_send_page _P2( (g3_file, fd),
		       char * g3_file, int fd )
{
int g3fd;
char ch;
char buf[256];
char wbuf[ sizeof(buf) * 2 ];

static	char	fax_end_of_page[] = { DLE, ETX };

    lprintf( L_NOISE, "fax_send_page(\"%s\") started...", g3_file );

    /* disable software output flow control! It would eat the XON otherwise! */
    tio_set_flow_control( fd, &fax_tio, (FAXSEND_FLOW) & FLOW_HARD );
    tio_set( fd, &fax_tio );

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
	if ( fax_read_byte( fd, &ch ) != 1 )
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
    tio_set_flow_control( fd, &fax_tio,
			 (FAXSEND_FLOW) & (FLOW_HARD|FLOW_XON_OUT));
    tio_set( fd, &fax_tio );

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
		    /* for dfax files, we can check if the resolutions match
		     */
		    if ( ( fax_par_d.vr != 0 ) != ( buf[29] != 0 ) )
		    {
			fprintf( stderr, "WARNING: sending in %s mode, fax data is %s mode\n",
				 fax_par_d.vr? "fine" : "normal",
				 buf[29]     ? "fine" : "normal" );
			lprintf( L_WARN, "resolution mismatch" );
		    }
		}
                else
		/* it's incredible how stupid users are - check for */
		/* "tiffg3" files and issue a warning if the file is */
		/* suspect */
                if ( r >= 2 && ( ( buf[0] == 0x49 && buf[1] == 0x49 ) ||
                                 ( buf[0] == 0x4d && buf[1] == 0x4d ) ) )
		{
		    lprintf( L_WARN, "may be TIFF file" );
		    fprintf( stderr, "WARNING: file may be 'tiffg3' - TIFF file format is *not* supported!\n" );
		    fprintf( stderr, "         Thus, fax transmission will most propably fail\n" );
		}   
                else
                if ( r < 10 || buf[0] != 0 )
		{
		    lprintf( L_WARN, "file looks 'suspicious', buf=%02x %02x %02x %02x...", buf[0] &0xff, buf[1] &0xff, buf[2] &0xff, buf[3] &0xff );
                    fprintf( stderr, "WARNING: are you sure that this is a G3 fax file? Doesn't seem to be...\n" );
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
	    /* well, since the handshake stuff seems to work now,
	     * this shouldn't be necessary anymore (but if you have
	     * problems with missing scan lines, you could try this)
	     */
#if 0
	    ioctl( fd, TCSETAW, &fax_tio );
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

	    if ( check_for_input( fd ) )
	    {
		lprintf( L_NOISE, "input: got " );
		do
		{
		    /* intentionally don't use fax_read_byte here */
		    if ( read( fd, &ch, 1 ) != 1 )
		    {
			lprintf( L_ERROR, "read failed" );
			break;
		    }
		    else
			lputc( L_NOISE, ch );
		}
		while ( check_for_input( fd ) );
	    }
	}		/* end while (more g3 data to read) */
    }			/* end if (open file succeeded) */

    /* transmit end of page */
    lprintf( L_NOISE, "sending DLE ETX..." );
    write( fd, fax_end_of_page, sizeof( fax_end_of_page ));

    if ( fax_wait_for( "OK", fd ) == ERROR ) return ERROR;

    return NOERROR;
}

int main _P2( (argc, argv),
	      int argc, char ** argv )
{
int argidx;
int fd;
char buf[1000];
int	opt;
int i;

/* variables settable by command line options */
char *	extra_modem_init = NULL;
boolean fax_poll_req = FALSE;
char * 	fax_page_header = NULL;
char *	poll_directory = ".";			/* override with "-d" */

static char 	fax_device_string[] = FAX_MODEM_TTYS;	/* writable! */
char *	fax_devices = fax_device_string;	/* override with "-l" */
int	fax_res_fine = 1;			/* override with "-n" */

int	tries;

    /* initialize logging */
    strcpy( log_path, FAX_LOG );
    log_level = L_NOISE;

    while ((opt = getopt(argc, argv, "d:vx:ph:l:nm:")) != EOF) {
	switch (opt) {
	case 'd':	/* set target directory for polling */
	    poll_directory = optarg;
	    break;
	case 'v':	/* switch on verbose mode */
	    verbose = TRUE;
	    break;
	case 'x':	/* set debug level */
	    log_level = atoi(optarg);
	    break;
	case 'p':	/* enable polling */
	    fax_poll_req = TRUE;
	    break;
	case 'h':	/* set page header */
	    fax_page_header = optarg;
	    lprintf( L_MESG, "page header: %s", fax_page_header );
	    break;
	case 'l':	/* set device(s) to use */
	    fax_devices = optarg;
	    if ( strchr( optarg, '/' ) != NULL )
	    {
		fprintf( stderr, "%s: -l: use device name without path\n",
		                 argv[0]);
		exit(1);
	    }
	    break;
	case 'n':	/* set normal resolution */
	    fax_res_fine = 0;
	    break;
	case 'm':
	    extra_modem_init = optarg;
	    break;
	case '?':	/* unrecognized parameter */
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

    fd = fax_open( fax_devices );

    if ( fd == -1 )
    {
	lprintf( L_WARN, "cannot open fax device(s)" );
	fprintf( stderr, "%s: cannot access fax device(s) (locked?)\n", argv[0] );
	exit(2);
    }

    /* arrange that lock files get removed if INTR or QUIT is pressed */
    signal( SIGINT, fax_sig_goodbye );
    signal( SIGQUIT, fax_sig_goodbye );
    signal( SIGTERM, fax_sig_goodbye );

    sprintf( buf, "AT+FLID=\"%s\"", FAX_STATION_ID);
    if ( fax_command( "AT", "OK", fd ) == ERROR ||
         fax_command( "AT+FCLASS=2", "OK", fd ) == ERROR ||
         fax_command( buf, "OK", fd ) == ERROR )
    {
	lprintf( L_ERROR, "cannot initialize faxmodem" );
	fprintf( stderr, "%s: cannot initialize faxmodem\n", argv[0] );
	fax_close( fd );
	exit(3);
    }

    if ( extra_modem_init != NULL )
    {
	if ( strncmp( extra_modem_init, "AT", 2 ) != 0 )
	{
	    fax_send( "AT", fd );
	}

	if ( fax_command( extra_modem_init, "OK", fd ) == ERROR )
	{
	    lprintf( L_WARN, "cannot send extra modem init string '%s'",
		    extra_modem_init );
	    fprintf( stderr, "%s: modem doesnt't accept '%s'\n",
		    argv[0], extra_modem_init );
	    fax_close( fd );
	    exit(3);
	}
    }		/* end if (extra_modem_init != NULL) */

    /* FIXME: ask modem if it can do 14400 bps / fine res. at all */

    sprintf( buf, "AT+FDCC=%d,5,0,2,0,0,0,0", fax_res_fine );
    fax_command( buf, "OK", fd );

#if REVERSE
    fax_command( "AT+FBOR=0", "OK", fd );
#else
    fax_command( "AT+FBOR=1", "OK", fd );
#endif

    /* tell the modem if we are willing to poll faxes
     */
    if ( fax_poll_req )
    {
	sprintf( buf, "AT+FCIG=\"%s\"", FAX_STATION_ID);
	if ( fax_command( buf, "OK", fd ) == ERROR ||
	     fax_command( "AT+FSPL=1", "OK", fd ) == ERROR )
	{
	    lprintf( L_WARN, "AT+FSPL=1: cannot enable polling" );
	    fprintf( stderr, "Warning: polling is not possible!\n" );
	    fax_poll_req = FALSE;
	    fax_hangup = 0;	/* reset error flag */
	}
    }

    /* set modem to use hardware handshake, dial out
     */
    if ( verbose ) { printf( "Dialing %s... ", fac_tel_no ); fflush(stdout); }

    sprintf( buf, "AT%sD%s", FAX_MODEM_HANDSHAKE, fac_tel_no );
    if ( fax_command( buf, "OK", fd ) == ERROR )
    {
	lprintf( L_WARN, "dial failed (hangup_code=%d)", fax_hangup_code );
	fprintf( stderr, "\n%s: dial %s failed (%s)\n", argv[0], fac_tel_no,
		 fax_hangup_code == FHUP_BUSY? "BUSY" : "ERROR / NO CARRIER");

	/* end program - return codes signals kind of dial failure */
	if ( fax_hangup_code == FHUP_BUSY ) { fax_close( fd ); exit(4); }
	{ fax_close( fd ); exit(10); }
    }
    if ( verbose ) printf( "OK.\n" );

    /* by now, the modem should have raised DCD, so remove CLOCAL flag */
    tio_carrier( &fax_tio, TRUE );
#ifdef sun
    /* now we can request hardware flow control since we have carrier */
    tio_set_flow_control( fd, &fax_tio, (FAXSEND_FLOW) & (FLOW_HARD|FLOW_XON_OUT) );
#endif /* sun */
    tio_set( fd, &fax_tio );

    /* process all files to send / abort, if Modem sent +FHNG result */

    tries = 0;
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

	fax_page_tx_status = -1;

        /* transmit page punctuation
	 * (three cases: more pages, last page but polling, last page at all)
	 * then evaluate +FPTS: result code
	 */

	if ( argidx == argc-1 )		/* was this the last page to send? */
	  if ( fax_poll_req && fax_to_poll )
	    fax_command( "AT+FET=1", "OK", fd );	/* end document */
	  else
	  {
	    /* take care of some modems pulling cd low too soon */
	    tio_carrier( &fax_tio, FALSE );
#ifdef sun
	    /* HW handshake has to be off while carrier is low */
	    tio_set_flow_control(fd, &fax_tio, (FAXSEND_FLOW) & FLOW_XON_OUT);
#endif
	    tio_set( fd, &fax_tio );

	    fax_command( "AT+FET=2", "OK", fd );	/* end session */
	  }
	else
	    fax_command( "AT+FET=0", "OK", fd );	/* end page */

	/* after the page punctuation command, the modem
	 * will send us a +FPTS:<ppm> page transmit status.
	 * The ppm value is written to fax_page_tx_status by
	 * fax_wait_for()
	 * If the other side requests retransmission, do so.
	 */

	switch ( fax_page_tx_status )
	{
	    case 1: tries = 0; break;		/* page good */
						/* page bad - r. req. */
	    case 2: fprintf( stderr, "ERROR: page bad - retrain requested\n" );
		    tries ++;
		    if ( tries >= FAX_SEND_MAX_TRIES )
		    {
			fprintf( stderr, "ERROR: too many retries - aborting send\n" );
			fax_hangup_code = -1;
			fax_hangup = 1;
		    }
		    else
		    {
			if ( verbose )
			    printf( "sending page again (retry %d)\n", tries );
			continue;	/* don't go to next page */
		    }
		    break;
	    case 3: fprintf( stderr, "WARNING: page good, but retrain requested\n" );
		    break;
	    case 4:
	    case 5: fprintf( stderr, "WARNING: procedure interrupt requested - don't know how to handle it\n" );
		    break;
	    case -1:			/* something broke */
		    lprintf( L_WARN, "fpts:-1" );
		    break;
	    default:fprintf( stderr, "WARNING: invalid code: +FPTS:%d\n",
	                             fax_page_tx_status );
		    break;
	}
	argidx++;
    }				/* end main page loop */

    if ( argidx < argc || fax_hangup_code != 0 )
    {
	lprintf( L_WARN, "Failure transmitting %s: +FHNG:%2d",
		 argv[argidx-1], fax_hangup_code );
	fprintf( stderr, "\n%s: FAILED to transmit '%s'.\n",
		         argv[0], argv[argidx-1] );

	if ( fax_hangup_code == -1 )
	    fprintf( stderr, "(number of tries exhausted)\n" );
	else
	    fprintf( stderr, "Transmission error: +FHNG:%2d (%s)\n",
			     fax_hangup_code,
			     fax_strerror( fax_hangup_code ) );
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
	    /* switch to fax receiver flow control */
	    tio_set_flow_control( fd, &fax_tio,
				 (FAXREC_FLOW) & (FLOW_HARD|FLOW_XON_IN) );
	    tio_set( fd, &fax_tio );
	    if ( fax_get_pages( fd, &pagenum, poll_directory ) == ERROR )
	    {
		fprintf( stderr, "warning: polling failed\n" );
		lprintf( L_WARN, "warning: polling failed!" );
		fax_close( fd );
		exit(12);
	    }
	}
	if ( verbose ) printf( "%d pages successfully polled!\n", pagenum );
    }

    fax_close( fd );
    return 0;
}
