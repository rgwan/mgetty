#ident "$Id: faxfile.c,v 1.1 1999/04/11 21:12:06 gert Exp $"

/* faxfile.c
 *
 * Auxiliary functions for fax (raw G3 or TIFF/F) file access
 *
 * the idea behind this is that all the G3-writing (and reading) programs 
 * can access a single interface to "raw G3", digifax, and TIFF files
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

#include "mgetty.h"

/* for fax rec stuff */
#define ETX     003
#define DLE     020
#define SUB     032
#define DC2     022

static int fax_out_fd;

#define BUFSIZE 1024
static unsigned char fo_buf[BUFSIZE+20];
static int fo_bufidx;
static int fo_was_dle;

static unsigned char fo_swaptable[256];

/* set up bit swap table to "direct" or "reverse bits"
 */
void fax_init_swaptable _P2( (direct, byte_tab),
                              int direct, unsigned char byte_tab[] )
{
int i;
    if ( direct ) for ( i=0; i<256; i++ ) byte_tab[i] = i;
    else
      for ( i=0; i<256; i++ )
             byte_tab[i] = ( ((i & 0x01) << 7) | ((i & 0x02) << 5) |
                             ((i & 0x04) << 3) | ((i & 0x08) << 1) |
                             ((i & 0x10) >> 1) | ((i & 0x20) >> 3) |
                             ((i & 0x40) >> 5) | ((i & 0x80) >> 7) );
}

/* open file "name" for output, set up swaptable
 */
int faxfile_write_g3 _P2((name, direct),
			char * name, int direct)
{
    fax_out_fd = open( name, O_WRONLY | O_CREAT | O_TRUNC, 0644 );

    if ( fax_out_fd < 0 )
    {
    	lprintf( L_ERROR, "can't open '%s' for writing", name );
	return -1;
    }

    fo_bufidx = 0;
    fo_was_dle = 0;

    fax_init_swaptable( direct, fo_swaptable );

    return 0;
}

/* strip DLE characters, recognize DLE ETX, swap bits if needed
 * return values: 0 = ok, >0 = end of file, -1 = error
 */
int faxfile_wbyte _P1((byte), unsigned char byte)
{
    if ( !fo_was_dle )
    {
	if ( byte == DLE ) fo_was_dle = 1;
		      else fo_buf[fo_bufidx++] = fo_swaptable[byte];
    }
    else				/* last character WasDLE */
    {
	if ( byte == DLE ) 		/* DLE DLE -> DLE */
	    { fo_buf[fo_bufidx++] = fo_swaptable[byte]; }
	else
	  if ( byte == SUB )		/* DLE SUB -> 2x DLE */
	{
	    fo_buf[fo_bufidx] = fo_buf[fo_bufidx+1] = fo_swaptable[DLE];
	    fo_bufidx+=2;
	}
	else
	  if ( byte == ETX )
	{
	    lprintf( L_NOISE, "DLE ETX -> page complete" );
	    if ( write( fax_out_fd, fo_buf, fo_bufidx ) != fo_bufidx )
	    {
	    	lprintf( L_ERROR, "can't write all %d bytes to file", fo_bufidx );
		close( fax_out_fd );
		return -1;
	    }
	    close( fax_out_fd );
	    return 1;
	}

	fo_was_dle=0;
    }

    if ( fo_bufidx >= BUFSIZE )	/* buffer full */
    {
    	if ( write( fax_out_fd, fo_buf, BUFSIZE ) != BUFSIZE )
	{
	    lprintf( L_ERROR, "can't write all %d bytes to file", BUFSIZ);
	    close( fax_out_fd );
	    return -1;
	}
	fo_bufidx -= BUFSIZE;

	if ( fo_bufidx > 0 )	/* some leftover bytes */
		memcpy( fo_buf, fo_buf+BUFSIZE, fo_bufidx );
    }

    return 0;
}

