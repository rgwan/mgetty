#ident "%W% %E% Copyright (c) Gert Doering"

/* ltest.c
 *
 * show status of all the RS232 lines (RTS, CTS, ...)
 * Calls routines in delay.c, tio.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "mgetty.h"
#include "tio.h"

char * Device;
int delay_time = 0;			/* in milliseconds, 0 = one-shot */

/* we don't want logging here */
int lprintf() { return 0; };
int lputs( int level, char * string ) { return 0; };

int main( int argc, char ** argv )
{
int opt, fd, f;

    while ((opt = getopt(argc, argv, "i:m:")) != EOF)
    {
	switch( opt )
	{
	    case 'i': delay_time = 1000 * atoi(optarg); break;	/* secs */
	    case 'm': delay_time = atoi(optarg); break;		/* msecs */
	    default:
		fprintf( stderr, "Valid options: -i <seconds-delay>, -m <msec-delay>" ); exit(7);
	}
    }

    if ( optind < argc )		/* argument == tty to use */
    {
	Device = argv[optind++];
	fd = open( Device, O_RDONLY | O_NDELAY );

	if ( fd < 0 )
		{ perror( "Opening device failed" ); exit(17); }

	fcntl( fd, F_SETFL, O_RDONLY );
    }
    else				/* default: use stdin */
    {
	Device = "stdin"; fd = fileno(stdin);
    }

    do
    {
	f = tio_get_rs232_lines( fd );

	if ( f == -1 )
	{
	    printf( "%s: can't read RS232 line status (-1)\n", Device );
	    exit(17);
	}

	printf( "%s: active lines:", Device );

	if ( f & TIO_F_DTR ) printf( " DTR" );
	if ( f & TIO_F_DSR ) printf( " DSR" );
	if ( f & TIO_F_RTS ) printf( " RTS" );
	if ( f & TIO_F_CTS ) printf( " CTS" );
	if ( f & TIO_F_DCD ) printf( " DCD" );
	if ( f & TIO_F_RI  ) printf( " RI" );

	if ( f == 0 ) printf( " <none>" );

	printf( "\n" );

	if ( delay_time ) delay( delay_time );
    }
    while( delay_time );

    return 0;
}

