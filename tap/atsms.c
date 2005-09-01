#ident "$Id: atsms.c,v 1.1 2005/09/01 09:41:19 gert Exp $ Copyright (c) Gert Doering"

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

/* we don't want logging here */
#ifdef USE_VARARGS
int lprintf() { return 0; }
#else
int lprintf(int level, const char *format, ...) { return 0; }
#endif
int lputs( int level, char * string ) { return 0; }
int lputc( int level, char ch ) { return 0; }

/* interrupt handler for SIGALARM */
boolean got_interrupt;
RETSIGTYPE oops(SIG_HDLR_ARGS)
		{ got_interrupt=TRUE; }

int send_sms( char * device, int speed, char * sms_to, char * sms_text )
{
int fd;
TIO tio, save_tio;

char buf[200];
int l;

    printf( "send SMS message to \"%s\"...\nMSG: \"%s\"\n", sms_to, sms_text );

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

    if ( mdm_send( sms_text, fd ) == ERROR ||
         write( fd, "\032", 1 ) != 1 )		/* ctrl-Z as term.chr. */
    {
	fprintf( stderr, "can't send message '%s' to modem?!\n", sms_text );
	close(fd); rmlocks(); return -1;
    }

    /* wait for response from modem */

    signal( SIGALRM, oops );
    alarm(5);
    got_interrupt = FALSE;

    do
    {
	l = read( fd, buf, sizeof(buf) );
	if ( l>0 ) 
	{
	    fwrite( buf, 1, l, stdout );
	} 
    }
    while( l>0 && ! got_interrupt );

    if ( errno != EINTR || !got_interrupt )
    {
	fprintf( stderr, "error reading from device '%s': %s\n",
		 device, strerror(errno));
    }

    if ( tio_set( fd, &save_tio ) == ERROR )
    {
	fprintf( stderr, "error setting TIO settings for '%s': %s\n",
		 device, strerror(errno));
    }
    close(fd);
    rmlocks();
    signal( SIGALRM, SIG_DFL );
    return 0;
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

    while ((opt = getopt(argc, argv, "l:s:x:")) != EOF)
    {
	switch( opt )
	{
	    case 'l': device = optarg; break;
	    case 's': speed = atoi(optarg); break;
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

    send_sms( device, speed, argv[optind], argv[optind+1] );

    return 0;
}

