#ident "$Id: mlo.c,v 1.2 1999/04/05 20:42:17 gert Exp $"

/* mlo.c
 *
 * Command-Line interface to ELSA MicroLink Office modem
 * (upload/download files, convert ADPCM to .RMD files, convert .T4* to G3)
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#ifndef ENOENT
# include <errno.h>
#endif

#include "version.h"
#include "mgetty.h"
#include "tio.h"
#include "policy.h"

/* for RMD stuff */
#include "voice/include/header.h"
#include <netinet/in.h>

rmd_header elsa_rmd = { "RMD1", "Elsa", 0, 0, 0, { 0,0,0,0,0,0,0 }};

int verbose=1;

extern int xmodem_blk;

int open_device _P3( (tty, tio, speed), char * tty, TIO *tio, int speed )
{
    char	device[MAXPATH];
    int	fd;

    int tries;
    
    /* ignore leading "/dev/" prefix */
    if ( strncmp( tty, "/dev/", 5 ) == 0 ) tty += 5;
    
    if ( verbose ) printf( "Trying fax device '/dev/%s'... ", tty );

    tries = 0;
    while ( makelock( tty ) != SUCCESS )
    {
	if ( ++ tries < 3 )
	{
	    if ( verbose ) { printf( "locked... " ); fflush( stdout ); }
	    sleep(5);
	}
	else
	{
	    if ( verbose ) { printf( "locked, give up!\n" );
			     fflush( stdout ); }
	    lprintf( L_MESG, "cannot lock %s", tty );
	    return -1;
	}
    }
    
    sprintf( device, "/dev/%s", tty );

    if ( ( fd = open( device, O_RDWR | O_NDELAY ) ) == -1 )
    {
	lprintf( L_ERROR, "error opening %s", device );
	if ( verbose ) printf( "cannot open %s: %s!\n", device, strerror(errno) );
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
    tio_get( fd, tio );

    tio_mode_sane( tio, TRUE );
    tio_set_speed( tio, speed );
    tio_mode_raw( tio );
#ifdef sun
    /* sunos does not rx with RTSCTS unless carrier present */
    tio_set_flow_control( fd, tio, FLOW_SOFT );
#else
    tio_set_flow_control( fd, tio, FLOW_HARD );
#endif
    
    if ( tio_set( fd, tio ) == ERROR )
    {
	lprintf( L_ERROR, "error in tio_set" );
	close( fd );
	if ( verbose ) printf( "cannot set termio values!\n" );
	rmlocks();
	return -1;
    }

    log_init_paths( NULL, NULL, &tty[ strlen(tty)-3 ] );
    lprintf( L_NOISE, "open_device succeeded, %s -> %d", tty, fd );
    
    if ( verbose ) printf( "OK.\n" );

    return fd;
}

/* finish off - close modem device, rm lockfile */

void modem_close _P1( (fd),
		    int fd )
{
    tio_flush_queue( fd, TIO_Q_BOTH );		/* unlock flow ctl. */
    mdm_send( "ATZ", fd );
    delay(500);
    tio_flush_queue( fd, TIO_Q_BOTH );		/* unlock flow ctl. */
    close( fd );
    rmlocks();
}

TIO modem_tio;


/* -------------------------------------------------------------------- */

/* query modem type, model, and firmware version */
int elsa_query _P1((fd), int fd)
{
char * l;

    l = mdm_get_idstring( "ATI", 1, fd );
    lprintf( L_NOISE, "mdm_identify: string '%s'", l );
    
    if ( strcmp( l, "<ERROR>" ) == 0 ) 
    {
	lprintf( L_WARN, "mdm_identify: can't get modem ID" );
	return -1;
    }

    if ( strcmp( l, "282" ) != 0 )
    {
	lprintf( L_WARN, "mdm_identify: no ELSA modem" );
	return -1;
    }

    l = mdm_get_idstring( "AT+GMM?", 1, fd );

    if ( strcmp( l, "<ERROR>" ) == 0 ||
         strcmp( l, "+GMM: \"MicroLink Office\"" ) != 0 )
    {
    	lprintf( L_WARN, "mdm_identify: no ELSA ML Office" );
	return -1;
    }

    l = mdm_get_idstring( "AT+GMR?", 1, fd );

    if ( verbose ) printf( "query: found ELSA MicroLink modem, version info: %s\n", l );
    return 0;
}

int elsa_list_dir _P1((fd), int fd)
{
char * l;

    if ( mdm_send( "AT$JDIR", fd ) == ERROR ) return -1;

    while( ( l = mdm_get_line( fd ) ) != NULL )
    {
	if ( strcmp( l, "AT$JDIR" ) == 0 ) continue;		/* echo */
	if ( strcmp( l, "OK" ) == 0 ) { return 0; }		/* done */

    	printf( "dir: %s\n", l );
    }
    return -1;
}


int elsa_download_raw _P5((fd, nam1, nam2, buf, len), 
			int fd, char * nam1, char * nam2, 
			char * header_buf, int header_len)
{
char buf[1050];
int outfd;
int s;				/* xmodem block size */
int total=0;

    /* open output file for saving */
    outfd = open( nam2, O_WRONLY|O_CREAT |O_TRUNC, 0644 );
    if ( outfd < 0 )
    {
	lprintf( L_ERROR, "can't write to %s", nam2);
        fprintf( stderr, "can't write to %s: %s\n", nam2, strerror(errno));
	return -1;
    }

    sprintf( buf, "AT$JDNL=\"%s\"", nam1 );
    if ( mdm_send( buf, fd ) == ERROR )
    {
        lprintf( L_ERROR, "can't send download command (%s)", buf );
	fprintf( stderr, "can't send download command (%s)\n", buf );
	close(outfd);
	unlink(nam2);
	return -1;
    }

    s = xmodem_rcv_init( fd, NULL, buf );

    if ( s <= 0 ) 
    {
        lprintf( L_ERROR, "XModem startup failed" );
        fprintf( stderr, "XModem startup failed\n" );
	close(outfd);
	unlink(nam2);
	return -1;
    }

    if ( header_buf != NULL && header_len>0 )
    {
	lprintf( L_NOISE, "writing %d byte header", header_len );
    	if ( write( outfd, header_buf, header_len ) != header_len )
	{
	    lprintf( L_ERROR, "can't write header to %s: %s", nam2, strerror(errno));
	    fprintf( stderr, "can't write header to %s: %s", nam2, strerror(errno));
	    close(outfd);
	    unlink(nam2);
	    return -1;
	}
    }

    do
    {
    	if ( write( outfd, buf, s ) != s ) 
	{
	    lprintf( L_ERROR, "can't write %d bytes to %s: %s",
	    		s, nam2, strerror(errno) );
	    fprintf( stderr, "can't write %d bytes to %s: %s\n",
	    		s, nam2, strerror(errno) );
	    close(outfd);
	    return -1;
	}
	total += s;
	printf( "block #%d, bytes %d\r", xmodem_blk, total ); fflush( stdout );

        s = xmodem_rcv_block( buf );

	lprintf( L_JUNK, "ed: s=%d", s );
    }
    while( s > 0 );
    close(outfd);

    if ( s < 0 )
    {
    	lprintf( L_ERROR, "can't receive expected block" );
    	fprintf( stderr, "can't receive expected block\n" );
	return -1;
    }

    printf( "%s received successfully\n", nam2);
    return 0;
}

/* donwload voice file - basically "download raw, prepend RMD header"
 */
int elsa_download_voice _P5((fd, nam1, nam2, bits, speed),
			int fd, char * nam1, char * nam2, 
			int bits, int speed )
{
/* compression 2/3/4 = ADPCM-2/3/4, bits = compression fuer ADPCM */
    elsa_rmd.compression = htons(bits);
    elsa_rmd.speed = htons(speed);
    elsa_rmd.bits = bits;

    return elsa_download_raw(fd, nam1, nam2, 
    			     (char *)&elsa_rmd, sizeof(elsa_rmd));
}

/* download fax file - remove DLE stuffing, interpret +F sequences
 */
int elsa_download_fax _P3((fd, nam1, nam2),
			int fd, char * nam1, char * nam2 )
{
char buf[1050], obuf[1050], line[200], ch;
int outfd;
int s,i,olen,			/* xmodem block size */
    in_g3 = 0,			/* G3 mode (vs. "line mode") */
    was_dle = 0;		/* last character seen was DLE */
int total=0;

    sprintf( buf, "AT$JDNL=\"%s\"", nam1 );
    if ( mdm_send( buf, fd ) == ERROR )
    {
        lprintf( L_ERROR, "can't send download command (%s)", buf );
	fprintf( stderr, "can't send download command (%s)\n", buf );
	close(outfd);
	return -1;
    }

    s = xmodem_rcv_init( fd, NULL, buf );

    if ( s <= 0 ) 
    {
        lprintf( L_ERROR, "XModem startup failed" );
        fprintf( stderr, "XModem startup failed\n" );
	close(outfd);
	unlink(nam2);
	return -1;
    }

    /* nothing in output buffer yet */
    olen = 0;

    do
    {
    	/* handle block */
	for ( i=0; i<s; i++ )
	{
	    ch = buf[i];
	    lputc( L_JUNK, ch );
	}

	total += s;
	printf( "block #%d, bytes %d\r", xmodem_blk, total ); fflush( stdout );

	/* get next block (or final EOT) */
        s = xmodem_rcv_block( buf );
    }
    while( s > 0 );

    close(outfd);

    if ( s < 0 )
    {
    	lprintf( L_ERROR, "can't receive expected block" );
    	fprintf( stderr, "can't receive expected block\n" );
	return -1;
    }

    printf( "%s received successfully\n", nam2);
    return 0;
}

/* -------------------------------------------------------------------- */


void exit_usage _P1((s), char * s)
{
    if ( s ) fprintf( stderr, "mlo: %s\n", s );
    fprintf( stderr, "usage: mlo -v -x<level> <device>\n" );
    exit(1);
}

int main _P2( (argc, argv),
	      int argc, char ** argv )
{
    int	fd;
    int opt;
    boolean opt_T = FALSE;			/* test mode */
    int opt_s = 38400;				/* DTE/DCE speed */
    char * Device;

    /* initialize logging */
    log_init_paths( argv[0], "/var/log/mlo.log", NULL );

    lprintf( L_MESG, "mlo: %s", mgetty_version );
    lprintf( L_NOISE, "%s compiled at %s, %s", __FILE__, __DATE__, __TIME__ );

#ifdef HAVE_SIGINTERRUPT
    /* interruptible system calls */
    siginterrupt( SIGINT,  TRUE );
    siginterrupt( SIGALRM, TRUE );
    siginterrupt( SIGHUP,  TRUE );
#endif

    while ((opt = getopt(argc, argv, "x:Ts:")) != EOF)
    {
	switch (opt)
	{
	  case 'x':	/* debug level */
	  	log_set_llevel( atoi(optarg) ); break;
	  case 'T':	/* Testing */
	  	opt_T = TRUE; break;
	  case 's':	/* download speed */
	  	opt_s = atoi(optarg); break;
	  case '?':
	    exit_usage(NULL);
	    break;
	}
    }

    /* device name is on the command line */
    if ( optind >= argc ) { exit_usage("missing device name"); }

    Device = argv[optind++];

    fd = open_device( Device, &modem_tio, opt_s );

    if ( fd < 0 ) { exit(2); }

    delay(200);					/* give modem time to settle */
    tio_flush_queue(fd, TIO_Q_BOTH);		/* clear junk */

    /* Is there a modem...? */
    if ( mdm_command( "ATV1Q0", fd ) == ERROR )
    {
	/* no??!? -- try again, maybe modem was just unwilling... */
	if ( mdm_command( "ATV1Q0", fd ) == ERROR )
	{
	    lprintf( L_AUDIT, "failed initializing modem, dev=%s", Device );
	    fprintf( stderr, "The modem doesn't respond!\n" );
	    tio_flush_queue( fd, TIO_Q_BOTH );	/* unlock flow ctl. */
	    close(fd);
	    rmlocks();
	    exit(3);
	}
	lprintf( L_WARN, "retry succeded, dev=%s", Device );
    }

    /* make sure there IS an Elsa modem */
    if ( elsa_query( fd ) < 0 ) 
    	{ modem_close(fd); exit(7); }

    elsa_list_dir( fd );

    /* now present user with command line, unless "testing only" */
    if ( opt_T )
    {
/*	elsa_download_voice( fd, "19990330211355.VOI", "/tmp/v1.rmd", 4, 7200);
	elsa_download_voice( fd, "19990405121736.VOI", "/tmp/v2.rmd", 4, 7200);
*/
/*	elsa_download( fd, "GREET_000.GRT", "/tmp/grt" );*/
	elsa_download_fax( fd, "19990405132347.FAX", "/tmp/f2.fax");
	exit(0);
    }

    modem_close(fd);

    return 0;
}
