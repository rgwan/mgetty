/* g3split.c
 *
 * program to split multi-page digifax files into single-page files
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

/* Digifax Header
 *   Preamble is static ("magic bytes")
 *   Byte 24+25 mark total number of pages (lo/hi byte)
 *   Byte 26+27 mark current pages number (lo/hi byte)
 *   Byte 45+29 are used for lo res (0/0) vs. hi res (0x40/0x01)
 */
static char hdr[64] = "\000PC Research, Inc\000\000\000\000\000\000";

int exit_usage(char * p )
{
    fprintf( stderr, "%s: syntax error\n", p );
    fprintf( stderr, "usage: %s input.g3 output<%%d>.g3\n", p );
    exit(1);
}

int main( int argc, char ** argv )
{
int pagecount;
char name[PATH_MAX];

FILE * in;
FILE * out;

char wbuf[8192];		/* "transit" buffer */
int  w;				/* write buffer counter */
int ch;
int hcnt;			/* "header match" counter */

    /* too few/too many arguments */
    if ( argc != 3 ) { exit_usage( argv[0] ); }

    /* target path too long */
    if ( strlen( argv[2] ) + 20 > sizeof( name ) ) { exit_usage( argv[0]); }

    if ( ( in = fopen( argv[1], "r" ) ) == NULL )
    {
	fprintf( stderr, "%s: can't read '%s': %s\n", argv[0], argv[1],
		 strerror(errno) );
	exit(2);
    }

    pagecount = 0;

    w=0;			/* output buffer write pointer */
    hcnt=0;			/* "header comparison" counter */

    while( ( ch = fgetc( in ) ) != EOF )
    {
	wbuf[w++] = ch;			/* save to output buffer */

	if ( ch == hdr[hcnt] ) 		/* found matching char.  */
	    hcnt++;
	else				/* mismatch -> reset ctr.*/
	    hcnt=0;

#ifdef DEBUG
	fprintf( stderr, "%02x w=%d hcnt=%d pc=%d\n", ch, w, hcnt, pagecount );
#endif

	if ( hcnt > 20 )			/* found "enough" header */
	{					/* to start new file */
	    pagecount++;
	    if ( pagecount != 1 )		/* not first file */
	    {					/* -> flush buffer */
		if ( fwrite( wbuf, 1, w-hcnt, out ) != w-hcnt )
		{
		    fprintf( stderr, "%s: can't write all %d bytes to %s: %s", argv[0], w-hcnt, name, strerror(errno) );
		    exit(4);
		}
		fclose( out );

		/* copy "what's left in the buffer" to "start of buffer"
		 */
		memmove( wbuf, &wbuf[w-hcnt], hcnt );
		w=hcnt;
	    }
	    hcnt=0;

	    /* and open next output file
	     */
	    sprintf( name, argv[2], pagecount );
	    if ( ( out = fopen( name, "w" ) ) == NULL )
	    {
		fprintf( stderr, "%s: can't open '%s' for writing: %s\n", 
			 argv[0], name, strerror(errno) );
		exit(3);
	    }
	}

	/* buffer more than 50% full -> flush out first 4K */
	if ( w > sizeof(wbuf)/2 )
	{
	    if ( pagecount == 0 )	/* no file open! */
			    { break; }

	    if ( fwrite( wbuf, 1, sizeof(wbuf)/2, out ) != sizeof(wbuf)/2 )
	    {
		fprintf( stderr, "%s: can't write all %d bytes to %s: %s", argv[0], (int) sizeof(wbuf)/2, name, strerror(errno) );
		exit(4);
	    }
	    w -= sizeof(wbuf)/2;
	    memcpy( wbuf, wbuf+sizeof(wbuf)/2, w );
	}
    }

    /* end of input file -> flush buffer to output file */
    if ( pagecount == 0 )	/* no file open! */
    {
	fprintf( stderr, "%s: input file %s is no digifax file\n", argv[0], argv[1] );
	exit(2);
    }
    if ( fwrite( wbuf, 1, w, out ) != w )
    {
	fprintf( stderr, "%s: can't write all %d bytes to %s: %s", argv[0], w, name, strerror(errno) );
	exit(4);
    }
    fclose(out);
    fclose(in);

    return 0;
}

