#ident "$Id: g32pbm.c,v 1.15 1994/05/20 09:07:23 gert Exp $ (c) Gert Doering"
;
#include <stdio.h>
#include <unistd.h>
#include "syslibs.h"
#include <string.h>
#include <fcntl.h>

#include "ugly.h"

#include "g3.h"

#ifdef DEBUG
void putbin _P1( (d), unsigned long d )
{
unsigned long i = 0x80000000;

    while ( i!=0 )
    {
	putc( ( d & i ) ? '1' : '0', stderr );
	i >>= 1;
    }
    putc( '\n', stderr );
}
#endif

static int byte_tab[ 256 ];
static int o_stretch;			/* -stretch: double each line */

struct g3_tree * black, * white;

#define CHUNK 2048;
static	char rbuf[2048];	/* read buffer */
static	int  rp;		/* read pointer */
static	int  rs;		/* read buffer size */

#define MAX_ROWS 4300
#define MAX_COLS 1728		/* !! FIXME - command line parameter */

int main _P2( (argc, argv), int argc, char ** argv )
{
int data;
int hibit;
struct	g3_tree * p;
int	nr_pels;
int fd;
int color;
int i;
int cons_eol;

char *	bitmap;			/* MAX_ROWS by MAX_COLS/8 bytes */
char *	bp;			/* bitmap pointer */
int	row;
int	max_rows;		/* max. rows allocated */
int	col, hcol;

    /* initialize lookup trees */
    build_tree( &white, t_white );
    build_tree( &white, m_white );
    build_tree( &black, t_black );
    build_tree( &black, m_black );

    init_byte_tab( 0, byte_tab );

    i = 1;
    while ( i<argc && argv[i][0] == '-' )/* option processing */
    {
	if ( argv[i][1] == 'r' )	/* -reversebits */
	{
	    init_byte_tab( 1, byte_tab );
	}
	else if (argv[i][1] == 's')
	{
	    o_stretch++;		/* -stretch this normal-res g3 */
	}
	i++;
    }

    if ( i < argc ) 			/* read from file */
    {
	fd = open( argv[i], O_RDONLY );
	if ( fd == -1 )
	{    perror( argv[i] ); exit( 1 ); }
    }
    else
	fd = 0;

    hibit = 0;
    data = 0;

    cons_eol = 0;	/* consecutive EOLs read - zero yet */

    color = 0;		/* start with white */

    rs = read( fd, rbuf, sizeof(rbuf) );
    if ( rs < 0 ) { perror( "read" ); close( rs ); exit(8); }

			/* skip GhostScript header */
    rp = ( rs >= 64 && strcmp( rbuf+1, "PC Research, Inc" ) == 0 ) ? 64 : 0;

    /* initialize bitmap */

    row = col = hcol = 0;
    bitmap = (char *) malloc( ( max_rows = MAX_ROWS ) * MAX_COLS / 8 );
    if ( bitmap == NULL )
    {
	fprintf( stderr, "cannot allocate %d bytes",
		 max_rows * MAX_COLS/8 );
	close( fd );
	exit(9);
    }
    memset( bitmap, 0, max_rows * MAX_COLS/8 );
    bp = &bitmap[ row * MAX_COLS/8 ]; 

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
		if ( rs == 0 ) { fprintf( stderr, "EOF!" ); goto do_write; }
	    }
#ifdef DEBUG
	    fprintf( stderr, "hibit=%2d, data=", hibit );
	    putbin( data );
#endif
	}

	if ( color == 0 )		/* white */
	    p = white->nextb[ data & BITM ];
	else				/* black */
	    p = black->nextb[ data & BITM ];

	while ( p != NULL && ! ( p->nr_bits ) )
	{
	    data >>= BITS;
	    hibit -= BITS;
	    p = p->nextb[ data & BITM ];
	}

	if ( p == NULL )	/* invalid code */
	{ 
	    fprintf( stderr, "invalid code, row=%d, col=%d, file offset=%lx, skip to eol\n",
		     row, col, lseek( fd, 0, 1 ) - rs + rp );
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
	    data >>= p->nr_bits;
	    hibit -= p->nr_bits;

	    nr_pels = ( (struct g3_leaf *) p ) ->nr_pels;
#ifdef DEBUG
	    fprintf( stderr, "PELs: %d (%c)\n", nr_pels, '0'+color );
#endif
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

	    if ( col == 0 )
		cons_eol++;		/* consecutive EOLs */
	    else
	    {
	        if ( col > hcol && col <= MAX_COLS ) hcol = col;
		row++;

		/* bitmap memory full? make it larger! */
		if ( row >= max_rows )
		{
		    char * p = realloc( bitmap,
				       ( max_rows += 500 ) * MAX_COLS/8 );
		    if ( p == NULL )
		    {
			perror( "realloc() failed, page truncated" );
			rs = 0;
		    }
		    else
		    {
			bitmap = p;
			memset( &bitmap[ row * MAX_COLS/8 ], 0,
			       ( max_rows - row ) * MAX_COLS/8 );
		    }
		}
			
		col=0; bp = &bitmap[ row * MAX_COLS/8 ]; 
		cons_eol = 0;
	    }
	}
	else		/* not eol */
	{
	    if ( col+nr_pels > MAX_COLS ) nr_pels = MAX_COLS - col;

	    if ( color == 0 )                  /* white */
		col += nr_pels;
	    else                               /* black */
	    {
            register int bit = ( 0x80 >> ( col & 07 ) );
	    register char *w = & bp[ col>>3 ];

		for ( i=nr_pels; i > 0; i-- )
		{
		    *w |= bit;
		    bit >>=1; if ( bit == 0 ) { bit = 0x80; w++; }
		    col++;
		}
	    }
	    if ( nr_pels < 64 ) color = !color;		/* terminating code */
	}
    }		/* end main loop */

do_write:      	/* write pbm (or whatever) file */

    if( fd != 0 ) close(fd);	/* close input file */

#ifdef DEBUG
    fprintf( stderr, "consecutive EOLs: %d, max columns: %d\n", cons_eol, hcol );
#endif

    sprintf( rbuf, "P4\n%d %d\n", hcol, ( o_stretch? row*2 : row ) );
    write( 1, rbuf, strlen( rbuf ));

    if ( hcol == MAX_COLS && !o_stretch )
        write( 1, bitmap, (MAX_COLS/8) * row );
    else
    {
	if ( !o_stretch )
	  for ( i=0; i<row; i++ )
	{
	    write( 1, &bitmap[ i*(MAX_COLS/8) ], (hcol+7)/8 );
	}
	else				/* Double each row */
	  for ( i=0; i<row; i++ )
        {
	    write( 1, &bitmap[ i*(MAX_COLS/8) ], (hcol+7)/8 );
	    write( 1, &bitmap[ i*(MAX_COLS/8) ], (hcol+7)/8 );
	}
    }

    return 0;
}
