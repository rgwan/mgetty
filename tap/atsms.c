#ident "$Id: atsms.c,v 1.5 2005/10/27 13:28:17 gert Exp $ Copyright (c) Gert Doering"

/* atsms.c
 *
 * send SMS via AT commands on serial interface
 *
 * Calls routines in io.c, tio.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#include "mgetty.h"
#include "policy.h"
#include "tio.h"

/* interrupt handler for SIGALARM */
boolean got_interrupt;
RETSIGTYPE oops(SIG_HDLR_ARGS)
		{ got_interrupt=TRUE; }

int send_sms( char * device, int speed, char * sim_pin, 
		char * sms_to, char * sms_text )
{
int fd;
TIO tio, save_tio;

char buf[200], *p;
int err=0;

    printf( "send SMS message to \"%s\"...\n", sms_to );

    /* lock device */
    if ( makelock( device ) == FAIL )
    {
	fprintf( stderr, "can't lock device '%s', skipping\n", device );
	return -1;
    }

    fd = open( device, O_RDWR | O_NDELAY );
    if ( fd < 0 )
    {
	fprintf( stderr, "error opening '%s': %s\n", device, strerror(errno));
	return -1;
    }

    /* unset O_NDELAY (otherwise waiting for characters */
    /* would be "busy waiting", eating up all cpu) */
    fcntl( fd, F_SETFL, O_RDWR);

    if ( tio_get( fd, &tio ) == ERROR )
    {
	fprintf( stderr, "error reading TIO settings for '%s': %s\n",
		 device, strerror(errno));
	close(fd); rmlocks();
	return -1;
    }
    save_tio = tio;
    tio_mode_sane( &tio, TRUE );
    tio_set_speed( &tio, speed );
    tio_mode_raw( &tio );
    if ( tio_set( fd, &tio ) == ERROR )
    {
	fprintf( stderr, "error setting TIO settings for '%s': %s\n",
		 device, strerror(errno));
	close(fd); rmlocks();
	return -1;
    }

    /* modem out there? */
    delay(10);			/* give device time to wake up*/
    write( fd, "\033", 1 );	/* cancel potentially leftover SMS */
    delay(10);

    if ( mdm_command( "AT", fd ) == ERROR )
    {
	fprintf( stderr, "modem on device '%s' doesn't respond\n", device );
	close(fd); rmlocks(); return -1;
    }

    /* PIN needed? */
    p = mdm_get_idstring( "AT+CPIN?", 1, fd );
    if ( p == NULL || strcmp( p, "<ERROR>" ) == 0 )
    {
	fprintf( stderr, "can't query modem on '%s' for PIN status: '%s'\n",
		 device, p );
	close(fd); rmlocks(); return -1;
    }

    if ( strcmp( p, "+CPIN: SIM PIN" ) == 0 )
    {
	if ( sim_pin == NULL || sim_pin[0] == '\0' )
	{
	    fprintf( stderr, "modem on '%s' wants PIN, but none specified\n", 
			device );
	    close(fd); rmlocks(); return -1;
	}
	printf( "GSM modem wants PIN, please wait (up to 2 minutes)...\n" );
	char sbuf[100];
	sprintf( sbuf, "AT+CPIN=%.20s", sim_pin );
	if ( mdm_command_timeout( sbuf, fd, 120 ) == ERROR )
	{
	    fprintf( stderr, "PIN '%s' not accepted on modem on '%s'\n",
			sim_pin, device );
	    close(fd); rmlocks(); return -1;
	}
	printf( "OK!\n" );
    }
    else
      if ( strcmp( p, "+CPIN: READY" ) != 0 )
    {
	fprintf( stderr, "unexpected response to PIN query: '%s'\n", p );
	close(fd); rmlocks(); return -1;
    }

    /* enter SMS text mode */
    if ( mdm_command( "AT+CMGF=1", fd ) == ERROR )
    {
	fprintf( stderr, "modem on device '%s' doesn't respond\n", device );
	close(fd); rmlocks(); return -1;
    }

    /* now send SMS destination */
    sprintf( buf, "AT+CMGS=\"%.*s\"", (int)(sizeof(buf)-20), sms_to );
    if ( mdm_send( buf, fd ) == ERROR )
    {
	fprintf( stderr, "can't send '%s' to modem?!\n", buf );
	close(fd); rmlocks(); return -1;
    }

    /* TODO: wait for '>' prompt */
    delay(100);

    printf( "MSG: \"%s\"\n", sms_text );
    if ( mdm_send( sms_text, fd ) == ERROR ||
         write( fd, "\032", 1 ) != 1 )		/* ctrl-Z as term.chr. */
    {
	fprintf( stderr, "can't send message '%s' to modem?!\n", sms_text );
	close(fd); rmlocks(); return -1;
    }

    /* wait for response from modem */

    signal( SIGALRM, oops );
    alarm(15);
    got_interrupt = FALSE;

    do
    {
	p = mdm_get_line( fd );

	if ( p == NULL ) { err++; break; }
	printf( "got: '%s'\n", p );

        if ( strncmp( p, "+CMGS:", 6 ) == 0 )
	{
	    int lfn = atoi( p+6 );
	    printf( "SMS sequence counter: %d\n", lfn );
	}
	if ( strcmp( p, "OK" ) == 0 ) break;
	if ( strcmp( p, "ERROR" ) == 0 ) 
	{ 
	    fprintf( stderr, "modem reports ERROR sending SMS on '%s'\n",
		     device );
	    err++;
	    break;
	}
    }
    while( !got_interrupt );
    alarm(0);

    if ( got_interrupt )
    {
	fprintf( stderr, "timeout waiting for modem ACK on '%s'\n", device );
    }
    else
      if ( p == NULL )
    {
	fprintf( stderr, "error reading from device '%s': %s\n",
		 device, strerror(errno));
	err++;
    }

    if ( tio_set( fd, &save_tio ) == ERROR )
    {
	fprintf( stderr, "error setting TIO settings for '%s': %s\n",
		 device, strerror(errno));
	err++;
    }
    close(fd);
    rmlocks();
    signal( SIGALRM, SIG_DFL );
    return ( err > 0 ) ? -1: 0;
}

void exit_usage( char * program, char * msg )
{
    if ( msg != NULL )
	fprintf( stderr, "%s: %s\n", program, msg );

    fprintf( stderr, "syntax: %s [opt] <sms-number> <text>\n", program );
    fprintf( stderr, "valid options: -l <device>, -s <speed>\n" );
    exit(99);
}

int main( int argc, char ** argv )
{
int opt;
char * device = "/dev/ttyh1";		/* TODO */

int speed = 38400;				/* port speed */
char * sim_pin = NULL;				/* pin number */
int rc;

    log_init_paths( argv[0], "/tmp/atsms.log", NULL );
    log_set_llevel(9);

    while ((opt = getopt(argc, argv, "l:s:x:p:")) != EOF)
    {
	switch( opt )
	{
	    case 'l': device = optarg; break;
	    case 's': speed = atoi(optarg); break;
	    case 'p': sim_pin = optarg; break;
	    case 'x': log_set_llevel( atoi(optarg) ); break;
	    default:
		exit_usage( argv[0], NULL );
	}
    }

    if ( optind == argc )		/* SMS number */
		exit_usage( argv[0], "no SMS number given" );
    if ( optind+1 == argc )		/* SMS text missing */
		exit_usage( argv[0], "no SMS text listed" );
    if ( optind+2 != argc )		/* too many arguments */
		exit_usage( argv[0], "too many arguments" );
    
#ifdef HAVE_SIGINTERRUPT
    /* some systems, notable BSD 4.3, have to be told that system
     * calls are not to be automatically restarted after those signals.
     */
    siginterrupt( SIGINT,  TRUE );
    siginterrupt( SIGALRM, TRUE );
    siginterrupt( SIGHUP,  TRUE );
#endif

    setvbuf( stdout, NULL, _IONBF, 0 );

    rc = send_sms( device, speed, sim_pin, argv[optind], argv[optind+1] );

    return rc == 0? rc: 1;
}

static boolean fwf_timeout = FALSE;
static RETSIGTYPE fwf_sig_alarm(SIG_HDLR_ARGS)      	/* SIGALRM handler */
{
    signal( SIGALRM, fwf_sig_alarm );
    lprintf( L_WARN, "Warning: got alarm signal!" );
    fwf_timeout = TRUE;
}
int mdm_command_timeout (char * send, int fd, int timeout )
{
    char * l;
    
    if ( mdm_send( send, fd ) == ERROR ) return ERROR;

    /* wait for OK or ERROR, *without* side effects (as fax_wait_for
     * would have)
     */
    signal( SIGALRM, fwf_sig_alarm ); alarm(timeout); fwf_timeout = FALSE;

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
