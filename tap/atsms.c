#ident "$Id: atsms.c,v 1.14 2010/04/28 09:05:17 gert Exp $ Copyright (c) Gert Doering"

/* atsms.c
 *
 * send SMS via AT commands on serial interface
 *
 * Calls routines in io.c, tio.c
 *
 * $Log: atsms.c,v $
 * Revision 1.14  2010/04/28 09:05:17  gert
 * don't try to open report_file if no "-F" argument given (= NULL)
 * cast chacter to (uchar) before checking for 8-bit Umlauts
 *
 * Revision 1.13  2010/04/26 15:42:44  gert
 * * implement retrieval of stored delivery reports -> write to '-F' file
 * * after sending SMSs, if delivery reports are requested, check for stored
 *   reports right away "might find something there"
 * * split receive_sms in upper half and lower half (do_receive())
 *
 * Revision 1.12  2010/04/26 13:35:27  gert
 * write acct_info and seqno to L_AUDIT line
 *
 * Revision 1.11  2010/04/26 13:28:06  gert
 * * add new options: -F <report file>, -A <acct info>, -v (verbose)
 * * if -F is given
 *   - write sent SMS with sequence number to file
 *   - if delivery report was requested, write report to file as well
 *   -> caller framework can correctly associate asynchronous reports later
 * * delivery reports are parsed & logged in handle_delivery_report()
 * * suppress lots of messages "unless $opt_v;"
 * * add some comments, get rid of no-prototype warnings
 *
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

#define LOG_FILE LOG_DIR "/atsms.log"

/* global option */
boolean opt_v = FALSE;				/* -v: verbose */

/* prototypes */
int mdm_command_timeout (char * send, int fd, int timeout );
int do_receive( int fd, int get_old, int do_delete, char * report_file );

/* interrupt handler for SIGALARM */
boolean got_interrupt;
RETSIGTYPE oops(SIG_HDLR_ARGS)
		{ got_interrupt=TRUE; }

int init_device( char * device, int speed, char * sim_pin, 
		 boolean * want_status_msg,
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

    /* enable/disable reception of transmission status message */
    if ( want_status_msg != NULL )
    {
	char sbuf[100];
	sprintf( sbuf, "AT+CNMI=2,1,0,%d", *want_status_msg? 2: 0 );
	if ( mdm_command( sbuf, fd ) == ERROR )
	{
	    if ( *want_status_msg )
	    {
		fprintf( stderr, "ERROR enabling SMS status reporting with AT+CNMI, disabling\n" );
		*want_status_msg = FALSE;
	    }
	    /* if not requested anyway, just ignore error */
	}

	/* I don't know what "17" stands for, but "49" is "enable status
	 * response and "17" is "default setting, no status response"
	 */
	sprintf( sbuf, "AT+CSMP=%d,167,0,0", *want_status_msg? 49: 17 );
	if ( mdm_command( sbuf, fd ) == ERROR )
	{
	    if ( *want_status_msg )
	    {
		fprintf( stderr, "ERROR enabling SMS status reporting with AT+CNMI, disabling\n" );
		*want_status_msg = FALSE;
	    }
	    /* if not requested anyway, just ignore error */
	}
    }

    return fd;
}

void handle_delivery_report( int seqno, char * rep, char * report_file )
{
    FILE * fp;
    int rep_seqno = -1;
    int err_code = -1;
    char * err_msg = "??";
    int n;
    char * copy, * p, * q;

    printf( "REP: %s\n", rep );

    /* +CMGR: "REC UNREAD",6,6,,,"10/04/26,14:32:26+08","10/04/26,14:32:32+08",0
     */
    copy = strdup( rep );
    if ( copy == NULL )
    {
	fprintf( stderr, "hdr: strdup() failed, no report written: %s\n",
		 strerror(errno) );
	return;
    }

    n = 0;
    p = copy;
    while( ( q = strsep( &p, "," ) ) != NULL )
    {
	/* printf( "%d: '%s'\n", n, q ); */
	if ( n == 2 && *q != '\0' ) rep_seqno = atoi(q);
	if ( n == 9 && *q != '\0' ) err_code = atoi(q);
	n++;
    }
    free( copy );

    if ( rep_seqno == -1 || err_code == -1 )
    {
	lprintf( L_MESG, "can't parse retrieved string '%s' (seqno=%d, err_code=%d), assume 'no delivery report'", rep, rep_seqno, err_code ); return;
    }

    if ( err_code == 0 )		/* success! */
	err_msg = "SUCCESS";
    else if ( err_code < 64 )		/* temporary error */
	err_msg = "TEMP ERROR";
    else				/* final failure */
	err_msg = "FINAL FAIL";

    if ( report_file != NULL )
    {
	fp = fopen( report_file, "a+" );
	if ( fp == NULL )
	{
	    fprintf( stderr, "hdr: error opening report file %s: %s\n",
			report_file, strerror(errno));
	}
	else
	{
	    fprintf( fp, "REP|%d|%d|%s|%s\n",
			    rep_seqno, err_code, err_msg, rep );
	    fclose(fp);
	}
    }
}

int send_sms( char * device, int speed, char * sim_pin, 
		boolean want_status_msg, 
		char * sms_to, char * sms_text,
		char * report_file, char * acct_info )
{
int fd;
TIO tio, save_tio;

char buf[200], *p;
int err=0;
int seqno = -1;			/* sms sequence number */

    printf( "send SMS message to \"%s\"...\n", sms_to );

    fd = init_device( device, speed, sim_pin, 
			&want_status_msg, &tio, &save_tio );

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
	switch( (unsigned char) *p )
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

    if ( opt_v ) printf( "MSG: \"%s\"\n", buf );
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
	if ( opt_v ) printf( "got: '%s'\n", p );

        if ( strncmp( p, "+CMGS:", 6 ) == 0 )
	{
	    seqno = atoi( p+6 );
	    printf( "SMS accepted!  Sequence counter: %d\n", seqno );
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

    /* append SMS sequence number + accounting info to report file
     * (to be able to correlate asynchronous delivery reports to 
     * specific sent SMSs)
     */
    if ( seqno >= 0 && report_file )
    {
	FILE * fp = fopen( report_file, "a+" );
	if ( fp == NULL )
	{
	    fprintf( stderr, "error opening report file %s: %s\n",
			report_file, strerror(errno));
	}
	else
	{
	    fprintf( fp, "SEND|%d|%s|%s|%s\n",
			seqno, acct_info, sms_to, sms_text );
	    fclose(fp);
	}
    }

    /* delivery report looks like this:
     * +CMGS: 11
     * 
     * OK
     * 
     * +CDSI: "MT",1
     * AT+CMGR=1
     * +CMGR: "REC UNREAD",6,11,,,"10/04/23,17:58:33+08","10/04/23,17:58:38+08",0
     * (stat, fo, mr, ra, tora, scts, dt, st [,data])
     * (st = "GSM 03.40 TP-Status in integer format")
     *
     * OK
     */

    if ( want_status_msg && !err && !got_interrupt )
    {
	if ( opt_v ) printf( "wait for delivery report... (120s)\n" );

	signal( SIGALRM, oops );
	alarm(120);

	do
	{
	    p = mdm_get_line( fd );

	    if ( p == NULL ) { err++; break; }
	    if ( opt_v ) printf( "got: '%s'\n", p );
	    if ( strncmp( p, "+CDSI:", 5 ) == 0 ) break;	/* got it! */
	}
	while( !got_interrupt );
	alarm(0);

	if ( p != NULL )					/* got it! */
	{
	    char * np;
	    int memloc;
	    char sbuf[100];

	    np = strchr( p, ',' );
	    if ( np == NULL )
	    {
		fprintf( stderr, "ERROR: can't parse modem response '%s'\n", p);
	    }
	    else
	    {
		/* retrieve report, then delete (free memory location) */
		memloc = atoi( np+1 );
		sprintf( sbuf, "AT+CMGR=%d", memloc );
		p = mdm_get_idstring( sbuf, 1, fd );
		handle_delivery_report( seqno, p, report_file );

		sprintf( sbuf, "AT+CMGD=%d", memloc );
		mdm_command( sbuf, fd );
	    }
	}

	/* turn off asynchronous responses (for still-pending SMS) now
	 * errors are ignored, we can't do anything about it anyway
	 */
	mdm_command( "AT+CNMI=2,1,0,0", fd );
    }

    /* while we're at it, check whether there are unread queued 
     * status messages...
     */
    if ( want_status_msg && !err && !got_interrupt )
    {
        err = do_receive( fd, FALSE, TRUE, report_file );
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
    lprintf( L_AUDIT, "SMS to %s: %s, seq=%d, acct=\"%s\"", 
			    sms_to, err? "failed": "sent", seqno, acct_info );
    return ( err > 0 ) ? -1: 0;
}

/* worker part of receive_sms(), also called after sending to 
 * fetch "pending" delivery reports
 */
int do_receive( int fd, int get_old, int do_delete, char * report_file )
{
char buf[200], *p;
int err=0;

struct sms { int n; } sms[100];
int nsms = 0;

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

	if ( opt_v ) printf( "got: %s\n", p );

	if ( strcmp( p, "OK" ) == 0 ) break;
	if ( strcmp( p, "ERROR" ) == 0 ) 
	{ 
	    fprintf( stderr, "modem reports ERROR retrieving SMS\n" );
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

	    /* if "-F" given, assume that this could be a delivery report 
	     * (if not, handle_delivery_report will just ignore it)
	     */
	    if ( report_file )
	    {
		char * first_komma = strchr( p, ',' );
		if ( first_komma != NULL )
		{
		    handle_delivery_report( -1, first_komma+1, report_file );
		}
	    }
	}
    }
    while( !got_interrupt );
    alarm(0);

    if ( got_interrupt )
    {
	fprintf( stderr, "timeout waiting for list of SMSs\n" );
    }
    else
      if ( p == NULL )
    {
	fprintf( stderr, "error reading from modem: %s\n", strerror(errno));
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

    return err;
}

int receive_sms( char * device, int speed, char * sim_pin, 
		 int get_old, int do_delete, char * report_file )
{
int fd;
TIO tio, save_tio;
int err;

    if ( opt_v ) printf( "retrieving SMS messages...\n" );

    fd = init_device( device, speed, sim_pin, NULL, &tio, &save_tio );

    if ( fd == -1 ) return -1;

    err = do_receive( fd, get_old, do_delete, report_file );

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
int opt_R = 0;					/* check delivery report */
char * report_file = NULL;			/* write report to file */
char * acct_info = "";				/* -A: acct info for SMS */

    log_init_paths( argv[0], LOG_FILE, NULL );
    log_set_llevel(9);

    while ((opt = getopt(argc, argv, "vl:s:x:p:rDRF:A:")) != EOF)
    {
	switch( opt )
	{
	    case 'v': opt_v = TRUE; break;
	    case 'l': device = optarg; break;
	    case 's': speed = atoi(optarg); break;
	    case 'p': sim_pin = optarg; break;
	    case 'x': log_set_llevel( atoi(optarg) ); break;
	    case 'r': opt_r = 1; break;		/* receive already-read SMS */
	    case 'D': opt_D = 1; break;		/* delete SMS after reading */
	    case 'R': opt_R = 1; break;		/* delivery report */
	    case 'F': report_file = optarg; break;
	    case 'A': acct_info = optarg; break;
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
	rc = receive_sms( device, speed, sim_pin, opt_r, opt_D, 
			  report_file );
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
    
    rc = send_sms( device, speed, sim_pin, 
		    opt_R, argv[optind], argv[optind+1],
		    report_file, acct_info );

    return rc == 0? rc: 1;
}

static boolean fwf_timeout = FALSE;
static RETSIGTYPE fwf_sig_alarm(SIG_HDLR_ARGS)      	/* SIGALRM handler */
{
    signal( SIGALRM, fwf_sig_alarm );
    lprintf( L_WARN, "Warning: got alarm signal!" );
    fwf_timeout = TRUE;
}

/* mdm_command() with caller-selectable timeout
 */
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
