#ident "$Id: microcom.c,v 1.3 2004/11/24 16:00:01 gert Exp $ Copyright (c) Gert Doering"

/* microcom.c
 *
 * very barebones 'talk to modem' program - similar to 'cu -l $device'
 *
 * Calls routines in io.c, tio.c
 */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>

#include "mgetty.h"
#include "policy.h"
#include "tio.h"

#define STDIN 0
#define STDOUT 1

/* we don't want logging here */
#ifdef USE_VARARGS
int lprintf() { return 0; }
#else
int lprintf(int level, const char *format, ...) { return 0; }
#endif
int lputs( int level, char * string ) { return 0; }

int main( int argc, char ** argv )
{
    int fd, len;
    TIO tio, save_tio;
    char buf[100], *device;
    int speed = 9600;

    if ( argc != 2 ) 
    { 
	fprintf( stderr, "usage: %s tty-name\n", argv[0] ); 
	exit(1);
    }

    device = argv[1];

    /* lock device */
    if ( makelock( device ) == FAIL )
    {
	fprintf( stderr, "can't lock device '%s', exiting\n", device );
	exit(1);
    }

    fd = open( device, O_RDWR | O_NDELAY );

    if ( fd < 0 )
	{ perror( "open device failed" ); exit(2); }

    /* unset O_NDELAY (otherwise waiting for characters */
    /* would be "busy waiting", eating up all cpu) */
    fcntl( fd, F_SETFL, O_RDWR);

    /* put stdin to "raw mode", handle one character at a time */
    tio_get( STDIN, &tio );
    save_tio = tio;
    tio_mode_raw( &tio );
    if ( tio_set( STDIN, &tio ) == ERROR )
    {
	fprintf( stderr, "error setting TIO settings for STDIN: %s\n",
		 strerror(errno));
	close(fd); rmlocks(); 
	exit(3);
    }

    /* same thing for modem (plus: set baud rate) - TODO: make CLI option */
    tio_get( fd, &tio );
    tio_mode_sane( &tio );
    tio_set_speed( &tio, speed );
    tio_mode_raw( &tio );
    if ( tio_set( fd, &tio ) == ERROR )
    {
	fprintf( stderr, "error setting TIO settings for '%s': %s\n",
		 device, strerror(errno));
	close(fd); rmlocks(); tio_set( STDIN, &save_tio );
	exit(3);
    }

    printf( "connected to '%s' (%d bps), exit with ctrl-X...\r\n",
	    device, speed );

    /* main loop: check with select(), then read/write bytes across */
    do
    {
	fd_set rfs;

	FD_ZERO( &rfs );
	FD_SET( STDIN, &rfs );
	FD_SET( fd, &rfs );

	select( FD_SETSIZE, &rfs, NULL, NULL, NULL );

	if ( FD_ISSET( STDIN, &rfs ) )
	{ 
	   len = read( STDIN, buf, 100 ); if ( len>0 ) write( fd, buf, len );
	}
	if ( FD_ISSET( fd, &rfs ) )
	{
	   len = read( fd, buf, 100 ); if ( len>0 ) write( STDOUT, buf, len );
	}
    }
    while ( buf[0] != 24 );		/* ^X fuer Ende */

    printf( "goodbye.\r\n" );

    tio_set( STDIN, &save_tio );
    close( fd );
    exit( 0 );
}
