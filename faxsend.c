#ident "$Id: faxsend.c,v 1.4 1994/04/24 12:49:42 gert Exp $ Copyright (c) 1994 Gert Doering"
;
/* faxsend.c
 *
 * Send single fax pages using a class 2 faxmodem.
 * Called by faxrec.c (poll server) and sendfax.c (sending faxes).
 *
 * Eventually add headers to each page.
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

static boolean fax_sendpg_timeout = FALSE;

static RETSIGTYPE fax_send_timeout()
{
    signal( SIGALRM, fax_send_timeout );	/* reactivate handler */
    
    lprintf( L_WARN, "fax_send: timeout" );
    fax_sendpg_timeout = TRUE;
}

/* DLE ETX: send at end of all fax data to terminate page */

static	char	fax_end_of_page[] = { DLE, ETX };

static void fax_send_panic_exit _P1( (fd), int fd )
{
    lprintf( L_FATAL, "PANIC: timeout sending fax page data, trying force modem reset\n" );

    /* by all means, reset modem */

    /* heavily use alarm(), to make sure nothing blocks */

    /* flush output queue */
    alarm( 5 ); tio_flush_queue( fd, TIO_Q_OUT );

    /* restart possibly suspended output */
    alarm( 5 ); tio_flow( fd, TRUE );

    /* tell modem that the page is finished. Try twice */
    alarm( 5 );	write( fd, fax_end_of_page, sizeof( fax_end_of_page ) );
    
    alarm( 5 );	write( fd, fax_end_of_page, sizeof( fax_end_of_page ) );

    /* Hang up. Various methods */
    alarm( 5 ); write( fd, "AT+FK\r\n", 7 );
    alarm( 5 ); write( fd, "ATH0\r\n", 6 );

    /* Now, try to reset it by lowering DTR */
    alarm( 10 );
    tio_toggle_dtr( fd, 500 );
    delay(500);
    tio_toggle_dtr( fd, 500 );
    delay(500);

    /* try again to hang up + reset it */
    alarm( 5 ); write( fd, "ATH0Z\r\n", 7 );
    delay(500);

    /* if the modem is *still* off-hook, there's nothing we can do. */
    /* Hope that mgetty will be able to reinitialize it */

    alarm( 0 );
    exit(11);
}

/* fax_send_page - send one complete fax-G3-file to the modem
 *
 * modem has to be in sync, waiting for at+fdt
 * NO page punctuation is transmitted -> caller can concatenate
 * multiple parts onto one page
 */
int fax_send_page _P3( (g3_file, tio, fd),
		       char * g3_file, TIO * tio, int fd )
{
int g3fd;
char ch;
char buf[256];
char wbuf[ sizeof(buf) * 2 ];

int w_total = 0;		/* total bytes written */

    lprintf( L_NOISE, "fax_send_page(\"%s\") started...", g3_file );

    /* disable software output flow control! It would eat the XON otherwise! */
    tio_set_flow_control( fd, tio, (FAXSEND_FLOW) & FLOW_HARD );
    tio_set( fd, tio );

    /* tell modem that we're ready to send - modem will answer
     * with a couple of "+F..." messages and finally CONNECT and XON
     */

    if ( fax_command( "AT+FDT", "CONNECT", fd ) == ERROR ||
	 fax_hangup != 0 )
    {
	lprintf( L_WARN, "AT+FDT -> some error (%d), abort fax send!",
		 fax_hangup_code );
	return ERROR;
    }

    /* when modem is ready to receive data, it will send us an XON
     * (20 seconds timeout)
     *
     * unfortunately, not all issues of the class 2 draft require this
     * XON - so, it's optional
     */

#ifndef FAXSEND_NO_XON
    lprintf( L_NOISE, "waiting for XON, got:" );

    signal( SIGALRM, fax_send_timeout );
    alarm( 20 );
    do
    {
	if ( fax_read_byte( fd, &ch ) != 1 )
	{
	    lprintf( L_ERROR, "timeout waiting for XON" );
	    fprintf( stderr, "error waiting for XON!\n" );
	    close( fd );
	    exit(11);		/*! FIXME! should be done farther up */
	}
	lputc( L_NOISE, ch );
    }
    while ( ch != XON );
    alarm(0);
#endif

    /* Since some faxmodems (ZyXELs!) do need XON/XOFF flow control
     * we have to enable it here
     */
    tio_set_flow_control( fd, tio, (FAXSEND_FLOW) & (FLOW_HARD|FLOW_XON_OUT));
    tio_set( fd, tio );

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
	int w_refresh = 0;
	boolean first = TRUE;

	alarm( 30 );		/* timeout if we get stuck in flow control */

	while ( ( r = read( g3fd, buf, 64 ) ) > 0 )
	{
	    /* refresh alarm counter every 1000 bytes */
	    if ( w_refresh > 1000 )
	    {
		w_refresh -= 1000;
		alarm( 30 );
	    }
	    
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

	    /* escape DLE characters. If necessary (rockjunk!), swap bits */
	    
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

	    /* check for timeout */

	    if ( fax_sendpg_timeout )
	    {
		fax_send_panic_exit( fd );
	    }

	    w_total += w;
	    w_refresh += w;
	    
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

    alarm(0);

    if ( fax_wait_for( "OK", fd ) == ERROR ) return ERROR;

    lprintf( L_MESG, "page complete, %d bytes sent", w_total );

    return NOERROR;
}
