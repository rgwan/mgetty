#ident "$Id: g3cat.c,v 1.10 1994/04/13 14:39:15 gert Exp $ (c) Gert Doering"
;
/* g3cat.c - concatenate multiple G3-Documents
 *
 * (Second try. Different algorithm.)
 *
 * Syntax: g3cat [options] [file(s) (or "-" for stdin)]
 *
 * Valid options: -l (separate g3 files with a black line)
 *                -d (output digifax header)
 *                -a (byte-align EOLs)
 *		  -h <lines> put <lines> empty lines on top of page
 *		  -p <pad>   zero-fill all lines up to <pad> bytes
 */

/* #define DEBUG 1 */

#include <stdio.h>
#ifndef _NOSTDLIB_H
#include <stdlib.h>
#endif
#include <unistd.h>
#include <fcntl.h>

extern int	optind;
extern char *	optarg;

#include "ugly.h"
#include "g3.h"

int byte_align = 0;

static unsigned char buf[2048];
static int buflen = 0;
static unsigned int out_data = 0;
static unsigned int out_hibit = 0;

static int out_byte_tab[ 256 ];
static int byte_tab[ 256 ];

static int padding = 0;			/* default: no padding done */
static int b_written = 0;		/* bytes of a line already */
					/* written */

#ifdef __GNUC__
inline
#endif
void putcode _P2( (code, len), int code, int len )
{
    out_data |= ( code << out_hibit );
    out_hibit += len;

    while( out_hibit >= 8 )
    {
	buf[ buflen++ ] = out_byte_tab[( out_data ) & 0xff];
	out_data >>= 8;
	out_hibit -= 8;
	if ( buflen >= sizeof( buf ) )
	{
	    write( 1, buf, buflen ); b_written += buflen; buflen = 0;
	}
    }
}

#ifdef __GNUC__
inline
#endif
void puteol _P0( void )			/* write byte-aligned EOL */
{
    static int last_buflen = 0;
    
    if ( padding > 0 )			/* padding? */
    {
	while( out_hibit != 4 ) putcode( 0, 1 );	/* implies */
							/* aligning */
	while( ( buflen + b_written ) - last_buflen < padding )
	{
	    putcode( 0, 8 );
	}
	last_buflen = buflen;
	b_written = 0;
    }
	
    if ( byte_align ) while( out_hibit != 4 ) putcode( 0, 1 );
    putcode( 0x800, 12 );
}

static	int putblackline = 0;	/* do not output black line */	

#define CHUNK 2048;
static	char rbuf[2048];	/* read buffer */
static	int  rp;		/* read pointer */
static	int  rs;		/* read buffer size */

struct g3_tree *white, *black;

void exit_usage _P1( (program), char * program )
{
    fprintf( stderr, "usage: %s [-h <lines>] [-a] [-l] [-p <n>] g3-file ...\n",
	    program );
    exit(1);
}

int main _P2( (argc, argv),
	      int argc, char ** argv )
{
    int i;				/* argument count */
    int fd;				/* file descriptor */
    unsigned int data;		/* read word */
    int hibit;			/* highest valid bit in "data" */

    int cons_eol;
    int color;
    int row, col;
    struct g3_tree * p;
    int nr_pels;
    int first_file = 1;		/* "-a" flag has to appear before */
				/* starting the first g3 file */
    int empty_lines = 0;	/* blank lines at top of page */

    /* initialize lookup trees */
    build_tree( &white, t_white );
    build_tree( &white, m_white );
    build_tree( &black, t_black );
    build_tree( &black, m_black );

    init_byte_tab( 0, byte_tab );
    init_byte_tab( 0, out_byte_tab );

    /* process the command line
     */

    while ( (i = getopt(argc, argv, "lah:p:")) != EOF )
    {
	switch (i)
	{
	  case 'l': putblackline = 1; break;
	  case 'a': byte_align = 1; break;
	  case 'h': empty_lines = atoi( optarg ); break;
	  case 'p': padding = atoi( optarg ); break;
	  case '?': exit_usage(argv[0]); break;
	}
    }
	    
    for ( i=optind; i<argc; i++ )
    {
	/* '-l' option my be embedded */
        if ( strcmp( argv[i], "-l" ) == 0)
        {
	    putblackline = 1; continue;
        }
		
	/* process file(s), one by one */
	if ( strcmp( argv[i], "-" ) == 0 )
		fd = 0;
	else
		fd = open( argv[i], O_RDONLY );

	if ( fd == -1 ) { perror( argv[i] ); continue; }

	if ( first_file )
	{
	    if ( byte_align || padding > 0 ) putcode( 0, 4 );
	    putcode( 0x800, 12 );			/* EOL (w/o */
							/* padding) */
	    first_file = 0;
	    while ( empty_lines-- > 0 )			/* leave space at */
							/* top of page */
	    {
		putcode( 0x1b2, 9 ); putcode( 0x0ac, 8 );
		puteol();
	    }
	}
	
	hibit = 0;
	data = 0;

	cons_eol = 0;		/* consecutive EOLs read - zero yet */

	color = 0;		/* start with white */

	rs = read( fd, rbuf, sizeof(rbuf) );
	if ( rs < 0 ) { perror( "read" ); close( rs ); exit(8); }

			    /* skip GhostScript header */
	rp = ( rs >= 64 && strcmp( rbuf+1, "PC Research, Inc" ) == 0 ) ? 64 : 0;

	row = col = 0;

	while ( rs > 0 && cons_eol < 4 )	/* i.e., while (!EOF) */
	{
#ifdef DEBUG
	    fprintf( stderr, "hibit=%2d, data=", hibit );
	    putbin( data );
#endif
	    while ( hibit < 20 )
	    {
		data |= ( byte_tab[ (int) (unsigned char) rbuf[ rp++] ] << hibit );
		hibit += 8;

		if ( rp >= rs )
		{
		    rs = read( fd, rbuf, sizeof( rbuf ) );
		    if ( rs < 0 ) { perror( "read2"); break; }
		    rp = 0;
		    if ( rs == 0 ) { fprintf( stderr, "EOF!" ); goto do_write;}
		}
#ifdef DEBUG
		fprintf( stderr, "hibit=%2d, data=", hibit );
		putbin( data );
#endif
	    }

#if DEBUG > 1
	    if ( color == 0 )
		print_g3_tree( "white=", white );
	    else
		print_g3_tree( "black=", black );
#endif

	    if ( color == 0 )		/* white */
		p = white->nextb[ data & BITM ];
	    else				/* black */
		p = black->nextb[ data & BITM ];

	    while ( p != NULL && ! ( p->nr_bits ) )
	    {
#if DEBUG > 1
		print_g3_tree( "p=", p );
#endif
		data >>= BITS;
		hibit -= BITS;
		p = p->nextb[ data & BITM ];
	    }

	    if ( p == NULL )	/* invalid code */
	    { 
		fprintf( stderr, "invalid code, row=%d, col=%d, file offset=%lx, skip to eol\n",
			 row, col, lseek( 0, 0, 1 ) - rs + rp );
		while ( ( data & 0x03f ) != 0 )
		{
		    data >>= 1; hibit--;
		    if ( hibit < 20 )
		    {
			data |= ( byte_tab[ (int) (unsigned char) rbuf[ rp++] ] << hibit );
			hibit += 8;

			if ( rp >= rs )	/* buffer underrun */
			{   rs = read( fd, rbuf, sizeof( rbuf ) );
			    if ( rs < 0 ) { perror( "read4"); break; }
			    rp = 0;
			    if ( rs == 0 ) goto do_write;
			}
		    }
		}
		nr_pels = -1;		/* handle as if eol */
	    }
	    else				/* p != NULL <-> valid code */
	    {
#if DEBUG > 1
		print_g3_tree( "p=", p );
#endif
		data >>= p->nr_bits;
		hibit -= p->nr_bits;

		nr_pels = ( (struct g3_leaf *) p ) ->nr_pels;
	    }

	    /* handle EOL (including fill bits) */
	    if ( nr_pels == -1 )
	    {
#ifdef DEBUG
		fprintf( stderr, "hibit=%2d, data=", hibit );
		putbin( data );
#endif
		/* skip filler 0bits -> seek for "1"-bit */
		while ( ( data & 0x01 ) != 1 )
		{
		    if ( ( data & 0xf ) == 0 )	/* nibble optimization */
		    {
			hibit-= 4; data >>= 4;
		    }
		    else
		    {
			hibit--; data >>= 1;
		    }
		    /* fill higher bits */
		    if ( hibit < 20 )
		    {
			data |= ( byte_tab[ (int) (unsigned char) rbuf[ rp++] ] << hibit );
			hibit += 8;

			if ( rp >= rs )	/* buffer underrun */
			{   rs = read( fd, rbuf, sizeof( rbuf ) );
			    if ( rs < 0 ) { perror( "read3"); break; }
			    rp = 0;
			    if ( rs == 0 ) goto do_write;
			}
		    }
#ifdef DEBUG
		fprintf( stderr, "hibit=%2d, data=", hibit );
		putbin( data );
#endif
		}				/* end skip 0bits */
		hibit--; data >>=1;
		
		color=0; 
#ifdef DEBUG
		fprintf( stderr, "EOL!\n" );
#endif
		if ( col == 0 )
		    cons_eol++;
		else
		{
		    row++; col=0;
		    cons_eol = 0;
		    puteol();
		}
	    }
	    else		/* not eol */
	    {
		/* output same code to g3 file on stdout */
		putcode( ( (struct g3_leaf *) p ) ->bit_code,
			 ( (struct g3_leaf *) p ) ->bit_length );

		col += nr_pels;
		if ( nr_pels < 64 ) color = !color;		/* terminal code */
	    }
	}		/* end processing one file */
do_write:      		/* write eol, separating lines, next file */

	if( fd != 0 ) close(fd);	/* close input file */

#ifdef DEBUG
	fprintf( stderr, "%s: number of EOLs: %d (%d)\n", argv[i], eols,cons_eols );
#endif
	/* separate multiple files with a line */
	if ( i != argc -1 )
	{
	    putcode( 0x1b2, 9 ); putcode( 0x0ac, 8 );	/* white line */
	    puteol();
            if ( putblackline )                         /* black line */
		{ putcode( 0x0ac, 8 );
		  putcode( 0x14c0, 13 );
		  putcode( 0x3b0, 10);
		  puteol(); }
	    putcode( 0x1b2, 9 ); putcode( 0x0ac, 8 );	/* white line */
	    puteol();
	}

    }	/* end for (all arguments) */

    /* output final RTC */
    for ( i=0; i<6; i++ ) puteol();

    /* flush buffer */
    if ( out_hibit != 0 )
        buf[buflen++] = out_byte_tab[out_data & 0xff];
    write( 1, buf, buflen );

    if ( first_file )
    {
	fprintf( stderr, "%s: warning: no input file specified, empty g3 file created\n", argv[0] );
    }

    return 0;
}
