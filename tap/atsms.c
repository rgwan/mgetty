#ident "$Id: atsms.c,v 1.8 2009/11/24 14:54:42 gert Exp $ Copyright (c) Gert Doering"

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
#include <ctype.h>

#include "mgetty.h"
#include "policy.h"
#include "tio.h"

/* interrupt handler for SIGALARM */
boolean got_interrupt;
RETSIGTYPE oops(SIG_HDLR_ARGS)
		{ got_interrupt=TRUE; }

int init_device( char * device, int speed, char * sim_pin, 
		 TIO * tio, TIO * save_tio )
{
int fd;
char *p;

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

    if ( tio_get( fd, tio ) == ERROR )
    {
	fprintf( stderr, "error reading TIO settings for '%s': %s\n",
		 device, strerror(errno));
	close(fd); rmlocks();
	return -1;
    }
    *save_tio = *tio;
    tio_mode_sane( tio, TRUE );
    tio_set_speed( tio, speed );
    tio_mode_raw( tio );
    if ( tio_set( fd, tio ) == ERROR )
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
		 device, p == NULL? "<NULL>": p );
	close(fd); rmlocks(); return -1;
    }

    if ( strcmp( p, "+CPIN: SIM PIN" ) == 0 )
    {
	char sbuf[100];
	if ( sim_pin == NULL || sim_pin[0] == '\0' )
	{
	    fprintf( stderr, "modem on '%s' wants PIN, but none specified\n", 
			device );
	    close(fd); rmlocks(); return -1;
	}
	printf( "GSM modem wants PIN, please wait (up to 2 minutes)...\n" );
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
	if ( strlen(p) == 0 )			/* no return value seen */
	    fprintf( stderr, "modem returned 'ERROR' to PIN query - maybe SIM card not properly inserted?\n" );
	else
	    fprintf( stderr, "unexpected response to PIN query: '%s'\n", p );
	close(fd); rmlocks(); return -1;
    }

    return fd;
}

int send_sms( char * device, int speed, char * sim_pin, 
		char * sms_to, char * sms_text )
{
int fd;
TIO tio, save_tio;

char buf[200], *p;
int err=0;

    printf( "send SMS message to \"%s\"...\n", sms_to );

    fd = init_device( device, speed, sim_pin, &tio, &save_tio );

    if ( fd == -1 ) return -1;

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

    /* transcode german umlauts, escape the rest */
    strncpy( buf, sms_text, 155 );
    buf[155] = '\0';
    for( p=buf; *p != '\0'; p++ )
    { 
	switch( *p )
	{
	case 0344: *p='{'; break;	/* ae */
	case 0304: *p='[';  break;	/* Ae */
	case 0366: *p='|';  break;	/* oe */
	case 0326: *p='\\'; break;	/* Oe */
	case 0374: *p='~';  break;	/* ue */
	case 0334: *p='^';  break;	/* Ue */
	case 0337: *p=0x1e; break;	/* ss */
	default:
	  if ( (*p) & 0x80 ) *p &= 0x7f; 	/* only 7 bit in TEXT mode */
	}
    }

    printf( "MSG: \"%s\"\n", buf );
    if ( mdm_send( buf, fd ) == ERROR ||
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

int receive_sms( char * device, int speed, char * sim_pin, 
		 int get_old, int do_delete )
{
int fd;
TIO tio, save_tio;

char buf[200], *p;
int err=0;

struct sms { int n; } sms[100];
int nsms = 0;

    printf( "retrieving SMS messages...\n" );

    fd = init_device( device, speed, sim_pin, &tio, &save_tio );

    if ( fd == -1 ) return -1;

    /* retrieve read/unread SMS */
    sprintf( buf, "AT+CMGL=\"REC %sREAD\"", get_old? "": "UN" );
    if ( mdm_send( buf, fd ) == ERROR )
    {
	fprintf( stderr, "can't send '%s' to modem?!\n", buf );
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

	printf( "got: %s\n", p );

	if ( strcmp( p, "OK" ) == 0 ) break;
	if ( strcmp( p, "ERROR" ) == 0 ) 
	{ 
	    fprintf( stderr, "modem reports ERROR sending SMS on '%s'\n",
		     device );
	    err++;
	    break;
	}
	if ( strncmp( p, "+CMGL:", 6 ) == 0 )
	{
	    sms[nsms++].n = atoi( &p[6] );
	    if( nsms >= sizeof(sms)/sizeof(sms[0]) )
	    {
		fprintf( stderr, "warning: too many SMSs stored in device, skip after %d\n", nsms);
		break;
	    }
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

    lprintf( L_AUDIT, "retrieved %d SMSs, %s", nsms, 
		err? "some error occured": "no errors" );

    if ( err == 0 && do_delete && nsms > 0 )
    {
	int i;
	
	printf( "\ndeleting SMS from memory...\n" );
	for( i=0; i<nsms; i++ )
	{
	    sprintf( buf, "AT+CMGD=%d", sms[i].n );
	    printf( "%s...\n", buf );
	    if ( mdm_command_timeout( buf, fd, 5 ) == ERROR ) 
		{ err++; break; }
	}
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

    fprintf( stderr, "syntax: %s [opt] [receive] <sms-number> <text>\n", program );
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
int opt_r = 0;					/* retrieve "old" SMS */
int opt_D = 0;					/* delete SMS from SIM */

    log_init_paths( argv[0], "/tmp/atsms.log", NULL );
    log_set_llevel(9);

    while ((opt = getopt(argc, argv, "l:s:x:p:rD")) != EOF)
    {
	switch( opt )
	{
	    case 'l': device = optarg; break;
	    case 's': speed = atoi(optarg); break;
	    case 'p': sim_pin = optarg; break;
	    case 'x': log_set_llevel( atoi(optarg) ); break;
	    case 'r': opt_r = 1; break;		/* receive already-read SMS */
	    case 'D': opt_D = 1; break;		/* delete SMS after reading */
	    default:
		exit_usage( argv[0], NULL );
	}
    }

#ifdef HAVE_SIGINTERRUPT
    /* some systems, notable BSD 4.3, have to be told that system
     * calls are not to be automatically restarted after those signals.
     */
    siginterrupt( SIGINT,  TRUE );
    siginterrupt( SIGALRM, TRUE );
    siginterrupt( SIGHUP,  TRUE );
#endif

    /* unbuffer stdout (nicer-to-follow output) */
    setvbuf( stdout, NULL, _IONBF, 0 );

    /* mode 1: "atsms receive" -> poll all SMSs in device
     */
    if ( optind+1 == argc && 
	  strcmp( argv[optind], "receive" ) == 0 )
    {
	rc = receive_sms( device, speed, sim_pin, opt_r, opt_D );
	return rc == 0? rc: 1;
    }

    /* default: send SMS
     */

    if ( optind == argc )		/* SMS number */
		exit_usage( argv[0], "no SMS number given" );
    if ( optind+1 == argc )		/* SMS text missing */
		exit_usage( argv[0], "no SMS text listed" );
    if ( optind+2 != argc )		/* too many arguments */
		exit_usage( argv[0], "too many arguments" );
    
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
