#ident "$Id: sff2g3.c,v 1.1 2004/07/16 14:46:11 gert Exp $ Copyright (C) 1994 Gert Doering"

/* sff2g3
 *
 * unpack a "structured file format" (ISDN CAPI fax file format) into
 * one or more individual "raw G3" files
 *
 * options: -d     output digifax header
 *	    -r     reverse bytes
 *	    -v     verbose output
 *
 * $Log: sff2g3.c,v $
 * Revision 1.1  2004/07/16 14:46:11  gert
 * SFF (shitty file format / CAPI fax format) to "raw G3" converter
 * first cut: can parse SFF input files and bit-reverse output
 * output file writing + EOL adding missing
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "syslibs.h"
#include <ctype.h>

#include "ugly.h"

#define TRUE 1
#define FALSE 0

void exit_usage _P1( (name), char * name )
{
    fprintf( stderr,
	     "usage: %s [-d] [-r] [-v] SFF_file G3_file.%%03d\n",
	      name );
    exit(1);
}
     
extern int	optind;
extern char *	optarg;

FILE * rfp;			/* read file pointer */
int verbose = 0;		/* option: -v */
int out_byte_tab[ 256 ];	/* for g3 byte reversal */

#define SIZEOF_SFF_HEADER	0x14
unsigned char hbuf[SIZEOF_SFF_HEADER];
#define SIZEOF_PAGE_HEADER	0x12
unsigned char pbuf[SIZEOF_PAGE_HEADER];
unsigned char workbuf[1024];

/* helper macros */
#define sff_short(x)	( (*(x)) + (*(x+1)<<8) )
#define sff_long(x)	( (*(x)) + (*(x+1)<<8) + (*(x+2)<<16) + (*(x+3)<<24) )

/* helper functions */
void sff_skip_bytes( int skip )
{
    if ( skip > 0 )
    {
	if ( verbose ) printf( "skipping 0x%02x bytes\n", skip );
	while( skip > 0 )
	{
	    int l = skip>sizeof(workbuf)?  sizeof(workbuf): skip;
	    fread( workbuf, 1, l, rfp ); skip -= l;
	}
    }
}

void sff_copy_line( int bytes )
{
int i, ch;

    if ( verbose>1 ) printf( "sff_copy_line: %d bytes:", bytes);
    for( i=0;i<bytes;i++)
    {
	ch=getc(rfp);
	if ( i<16 && verbose>1 ) 
		printf( " %02x", out_byte_tab[ch&0xff] );
    }
    if ( verbose>1 ) printf( "\n" );
}

int main _P2( (argc, argv), int argc, char ** argv )
{
    int c, len;
    int digifax_header = 0;
    int cur_page = 0;
    char * in_file, * out_file;
    
    init_byte_tab( FALSE, out_byte_tab );
    while ( (c = getopt(argc, argv, "drv") ) != EOF)
    {
	switch (c)
	{
	  case 'd': digifax_header = 1; break;
	  case 'r': init_byte_tab( TRUE, out_byte_tab ); break;
	  case 'v': verbose++; break;
	  default: exit_usage( argv[0] );
	}
    }

    if ( optind != argc-2 )			/* need 2 file names */
			exit_usage( argv[0] );
	
    in_file = argv[optind];
    out_file = argv[optind+1];

    if ( strcmp( in_file, "-" ) == 0 )		/* read from stdin */
    {
	rfp = stdin;
    }
    else					/* read from file */
    {
	rfp = fopen( in_file, "r" );
	if ( rfp == NULL )
	{
	    fprintf( stderr, "%s: cannot open %s: ", argv[0], in_file );
	    perror( "" );
	    exit(2);
	}
    }

    /* ok, open succeeded. now get file header */
    if ( fread( hbuf, 1, SIZEOF_SFF_HEADER, rfp ) != SIZEOF_SFF_HEADER )
    {
	fprintf( stderr, "%s: can't read SFF header from %s (short read)\n", argv[0], in_file );
	exit(3);
    }

    /* reject unknown file types */
    if ( memcmp( &hbuf[0], "SFFF", 4 ) == 0 || 
         hbuf[4] != 0x01 || hbuf[5] != 0x00 )
    {
	fprintf( stderr, "%s: input file not SFF version 1, abort.\n", argv[0]);
	exit(3);
    }

    if ( verbose )
    {
	printf( "SFFF header: version %d, userinfo=%04x, pages=%0d\n",
			hbuf[4], sff_short(&hbuf[6]), sff_short(&hbuf[8]) );
	printf( "             OffsetFPH=%04x, OffsetLPH=%08x, OffsetEnd=%08x\n",
			sff_short(&hbuf[0x0a]), sff_long(&hbuf[0x0c]), 
			sff_long(&hbuf[0x10]) );
    }

    /* skip over offset to first page header, if needed */
    sff_skip_bytes( sff_short(&hbuf[0x0a]) - SIZEOF_SFF_HEADER );

    /* now read page by page data */
    while( 1 )
    {
	cur_page++;
	len = fread( pbuf, 1, SIZEOF_PAGE_HEADER, rfp );

	/* this "should not happen", but creator apps are lazy :-( */
	if ( len == 0 )
	{
	    if ( verbose ) printf( "End-Of-File reached (ph#%d)\n", cur_page);
	    break;
	}

	/* end of file reached? -> might lead to 'short read' */
	if ( ( len >= 2 && pbuf[1] == 0 ) || 
	     ( len >= 3 && pbuf[2] == 255 ) ) break;

	if ( len != SIZEOF_PAGE_HEADER )
	{
	    fprintf( stderr, "%s: can't read page header (#%d) from %s (short read: want %d, got %d)\n", argv[0], cur_page, in_file, SIZEOF_PAGE_HEADER, len );
	    exit(3);
	}

	/* check type field (must bei 254) and coding (must be 0 = G3) */
	if ( pbuf[0] != 254 || pbuf[4] != 0 )
	{
	    fprintf( stderr, "%s: unsupported page header type (%d) or coding (%d), abort.\n", argv[0], pbuf[0], pbuf[4] );
	}

	if ( verbose )
	{
	    printf( "Page header %2d: len=0x%02x, vres=%d, hres=%d, coding=%d\n",
			cur_page, pbuf[1], pbuf[2], pbuf[3], pbuf[4] );
	    printf( "                pels=%d, lines=%d, OPP=%08x, ONP=%08x\n",
			sff_short(&pbuf[6]), sff_short(&pbuf[8]),
			sff_long(&pbuf[0x0a]), sff_long(&pbuf[0xe]) );
	}

	/* impossible offset values? */
	if ( pbuf[1]+2 < SIZEOF_PAGE_HEADER )
	{
	    fprintf( stderr, "%s: malformed page header: offset=0x%02d, abort\n", argv[0], pbuf[1] );
	    exit(3);
	}

	/* skip over offset to page data, if needed 
         * ("+2" because offset is calculated from vres field!?)
         */
	sff_skip_bytes( pbuf[1]+2 - SIZEOF_PAGE_HEADER );

	/* now handle page data */
	while( 1 )
	{
	    c = getc(rfp);
	    if ( c < 0 )			/* EOF/error -> end page */
		{ break; }
	    if ( c == 0 )			/* multibyte length */
	    {
		int c1, c2;
		c1 = getc(rfp); 
		c2 = getc(rfp);
		if ( c1<0 || c2<0 ) break;
		sff_copy_line( c1 + (c2<<8) );
	    }
	    else if ( c > 0 && c <= 216 )	/* <c> bytes of page data */
		{ sff_copy_line(c); }
	    else if ( c >= 217 && c<= 253 )	/* blank lines */
		printf( "TODO: c=%d->%d blank lines here\n", c, c-216 );
	    else if ( c == 254 )		/* next page header */
		{ ungetc(c, rfp); break; }
	    else if ( c == 255 )		/* user data */
		{ sff_skip_bytes( getc(rfp) ); break; }
	}
    }

    if ( verbose ) printf( "end of input reached, pages=%d\n", cur_page-1 );
    exit(0);
}
