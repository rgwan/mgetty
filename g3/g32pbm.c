#ident "$Id: g32pbm.c,v 1.6 1993/10/06 00:34:16 gert Exp $ (c) Gert Doering";

#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>

#ifndef NULL
#define NULL 0L
#endif

/* #define DEBUG 1*/

/*!! FIXME: extended codes */

struct g3code { int nr_pels, bit_code, bit_length; };

struct g3code t_white[66] = {
{   0, 0x0ac,  8 },
{   1, 0x038,  6 },
{   2, 0x00e,  4 },
{   3, 0x001,  4 },
{   4, 0x00d,  4 },
{   5, 0x003,  4 },
{   6, 0x007,  4 },
{   7, 0x00f,  4 },
{   8, 0x019,  5 },
{   9, 0x005,  5 },
{  10, 0x01c,  5 },
{  11, 0x002,  5 },
{  12, 0x004,  6 },
{  13, 0x030,  6 },
{  14, 0x00b,  6 },
{  15, 0x02b,  6 },
{  16, 0x015,  6 },
{  17, 0x035,  6 },
{  18, 0x072,  7 },
{  19, 0x018,  7 },
{  20, 0x008,  7 },
{  21, 0x074,  7 },
{  22, 0x060,  7 },
{  23, 0x010,  7 },
{  24, 0x00a,  7 },
{  25, 0x06a,  7 },
{  26, 0x064,  7 },
{  27, 0x012,  7 },
{  28, 0x00c,  7 },
{  29, 0x040,  8 },
{  30, 0x0c0,  8 },
{  31, 0x058,  8 },
{  32, 0x0d8,  8 },
{  33, 0x048,  8 },
{  34, 0x0c8,  8 },
{  35, 0x028,  8 },
{  36, 0x0a8,  8 },
{  37, 0x068,  8 },
{  38, 0x0e8,  8 },
{  39, 0x014,  8 },
{  40, 0x094,  8 },
{  41, 0x054,  8 },
{  42, 0x0d4,  8 },
{  43, 0x034,  8 },
{  44, 0x0b4,  8 },
{  45, 0x020,  8 },
{  46, 0x0a0,  8 },
{  47, 0x050,  8 },
{  48, 0x0d0,  8 },
{  49, 0x04a,  8 },
{  50, 0x0ca,  8 },
{  51, 0x02a,  8 },
{  52, 0x0aa,  8 },
{  53, 0x024,  8 },
{  54, 0x0a4,  8 },
{  55, 0x01a,  8 },
{  56, 0x09a,  8 },
{  57, 0x05a,  8 },
{  58, 0x0da,  8 },
{  59, 0x052,  8 },
{  60, 0x0d2,  8 },
{  61, 0x04c,  8 },
{  62, 0x0cc,  8 },
{  63, 0x02c,  8 },
{-1, 0, 11 },			/* 11 0-bits == EOL, special handling */
{-1, 0, 0 }};			/* end of table */

/* make-up codes white */
struct g3code m_white[28] = {
{  64, 0x01b,  5 },
{ 128, 0x009,  5 },
{ 192, 0x03a,  6 },
{ 256, 0x076,  7 },
{ 320, 0x06c,  8 },
{ 384, 0x0ec,  8 },
{ 448, 0x026,  8 },
{ 512, 0x0a6,  8 },
{ 576, 0x016,  8 },
{ 640, 0x0e6,  8 },
{ 704, 0x066,  9 },
{ 768, 0x166,  9 },
{ 832, 0x096,  9 },
{ 896, 0x196,  9 },
{ 960, 0x056,  9 },
{ 1024, 0x156,  9 },
{ 1088, 0x0d6,  9 },
{ 1152, 0x1d6,  9 },
{ 1216, 0x036,  9 },
{ 1280, 0x136,  9 },
{ 1344, 0x0b6,  9 },
{ 1408, 0x1b6,  9 },
{ 1472, 0x032,  9 },
{ 1536, 0x132,  9 },
{ 1600, 0x0b2,  9 },
{ 1664, 0x006,  6 },
{ 1728, 0x1b2,  9 },
{ -1, 0, 0} };


struct g3code t_black[66] = {
{   0, 0x3b0, 10 },
{   1, 0x002,  3 },
{   2, 0x003,  2 },
{   3, 0x001,  2 },
{   4, 0x006,  3 },
{   5, 0x00c,  4 },
{   6, 0x004,  4 },
{   7, 0x018,  5 },
{   8, 0x028,  6 },
{   9, 0x008,  6 },
{  10, 0x010,  7 },
{  11, 0x050,  7 },
{  12, 0x070,  7 },
{  13, 0x020,  8 },
{  14, 0x0e0,  8 },
{  15, 0x030,  9 },
{  16, 0x3a0, 10 },
{  17, 0x060, 10 },
{  18, 0x040, 10 },
{  19, 0x730, 11 },
{  20, 0x0b0, 11 },
{  21, 0x1b0, 11 },
{  22, 0x760, 11 },
{  23, 0x0a0, 11 },
{  24, 0x740, 11 },
{  25, 0x0c0, 11 },
{  26, 0x530, 12 },
{  27, 0xd30, 12 },
{  28, 0x330, 12 },
{  29, 0xb30, 12 },
{  30, 0x160, 12 },
{  31, 0x960, 12 },
{  32, 0x560, 12 },
{  33, 0xd60, 12 },
{  34, 0x4b0, 12 },
{  35, 0xcb0, 12 },
{  36, 0x2b0, 12 },
{  37, 0xab0, 12 },
{  38, 0x6b0, 12 },
{  39, 0xeb0, 12 },
{  40, 0x360, 12 },
{  41, 0xb60, 12 },
{  42, 0x5b0, 12 },
{  43, 0xdb0, 12 },
{  44, 0x2a0, 12 },
{  45, 0xaa0, 12 },
{  46, 0x6a0, 12 },
{  47, 0xea0, 12 },
{  48, 0x260, 12 },
{  49, 0xa60, 12 },
{  50, 0x4a0, 12 },
{  51, 0xca0, 12 },
{  52, 0x240, 12 },
{  53, 0xec0, 12 },
{  54, 0x1c0, 12 },
{  55, 0xe40, 12 },
{  56, 0x140, 12 },
{  57, 0x1a0, 12 },
{  58, 0x9a0, 12 },
{  59, 0xd40, 12 },
{  60, 0x340, 12 },
{  61, 0x5a0, 12 },
{  62, 0x660, 12 },
{  63, 0xe60, 12 },
{  -1, 0x000, 11 },
{  -1, 0, 0 } };

struct g3code m_black[28] = {
{  64, 0x3c0, 10 },
{ 128, 0x130, 12 },
{ 192, 0x930, 12 },
{ 256, 0xda0, 12 },
{ 320, 0xcc0, 12 },
{ 384, 0x2c0, 12 },
{ 448, 0xac0, 12 },
{ 512, 0x6c0, 13 },
{ 576, 0x16c0, 13 },
{ 640, 0xa40, 13 },
{ 704, 0x1a40, 13 },
{ 768, 0x640, 13 },
{ 832, 0x1640, 13 },
{ 896, 0x9c0, 13 },
{ 960, 0x19c0, 13 },
{ 1024, 0x5c0, 13 },
{ 1088, 0x15c0, 13 },
{ 1152, 0xdc0, 13 },
{ 1216, 0x1dc0, 13 },
{ 1280, 0x940, 13 },
{ 1344, 0x1940, 13 },
{ 1408, 0x540, 13 },
{ 1472, 0x1540, 13 },
{ 1536, 0xb40, 13 },
{ 1600, 0x1b40, 13 },
{ 1664, 0x4c0, 13 },
{ 1728, 0x14c0, 13 },
{ -1, 0, 0 } };

/* The number of bits looked up simultaneously determines the amount
 * of memory used by the program - some values:
 * 10 bits : 87 Kbytes, 8 bits : 20 Kbytes
 *  5 bits :  6 Kbytes, 1 bit  :  4 Kbytes
 * - naturally, using less bits is also slower...
 */

/*
#define BITS 5
#define BITM 0x1f
*/

#define BITS 8
#define BITM 0xff

/*
#define BITS 12
#define BITM 0xfff
*/

#define BITN 1<<BITS

struct g3_tree { int nr_bits;
		 struct g3_tree *	nextb[ BITN ];
		 };

struct g3_leaf { int nr_bits;
		 int nr_pels; };

void tree_add_node( struct g3_tree *p, int nr_pels,
		    int bit_code, int bit_length )
{
int i;

    if ( bit_length <= BITS )		/* leaf (multiple bits) */
    {
    struct g3_leaf *p3;

	p3 = (struct g3_leaf *) malloc( sizeof( struct g3_leaf ) );
	if ( p3 == NULL ) { perror( "malloc(2)"); exit(10); }

	p3->nr_bits = bit_length;	/* leaf tag */
	p3->nr_pels = nr_pels;

	if ( bit_length == BITS )	/* full width */
	{
	    p->nextb[ bit_code ] = (struct g3_tree *) p3;
	}
	else				/* fill bits */
	  for ( i=0; i< ( 1 << (BITS-bit_length)); i++ )
	  {
	    p->nextb[ bit_code + ( i << bit_length ) ] = (struct g3_tree *) p3;
	  }
    }
    else				/* node */
    {
    struct g3_tree *p2;

	p2 = p->nextb[ bit_code & BITM ];
	if ( p2 == 0 )			/* no sub-node exists */
	{
	    p2 = p->nextb[ bit_code & BITM ] =
		( struct g3_tree * ) calloc( 1, sizeof( struct g3_tree ));
	    if ( p2 == NULL ) { perror( "malloc 3" ); exit(11); }
	    p2->nr_bits = 0;		/* node tag */

	}
	if ( p2->nr_bits != 0 )
	{
	    fprintf( stderr, "internal table setup error\n" ); exit(6);
	}
	tree_add_node( p2, nr_pels, bit_code >> BITS, bit_length - BITS );
    }
}

void build_tree( struct g3_tree ** p, struct g3code * c )
{
    if ( *p == NULL )
    {
	(*p) = (struct g3_tree *) calloc( 1, sizeof(struct g3_tree) );
	if ( *p == NULL ) { perror( "malloc(1)" ); exit(10); }

	(*p)->nr_bits=0;
    }

    while ( c->bit_length != 0 )
    {
	tree_add_node( *p, c->nr_pels, c->bit_code, c->bit_length );
	c++;
    }
}

#ifdef DEBUG
void putbin( unsigned long d )
{
unsigned long i = 0x80000000;

    while ( i!=0 )
    {
	putc( ( d & i ) ? '1' : '0', stderr );
	i >>= 1;
    }
    putc( '\n', stderr );
}

void print_g3_tree( char * t, struct g3_tree * p )
{
int i;
    if ( p->nr_bits )
	fprintf( stderr, "%s (%08x) leaf( nr_bits=%2d, PELs=%3d )\n",
		 t, (int) p, p->nr_bits, ((struct g3_leaf *) p)->nr_pels );
    else
    {
#if DEBUG > 1
	fprintf( stderr, "%s (%08x) node (BITs=%d)\n", t, (int) p, BITS );
	for ( i=0; i<BITN; i++ )
	{
	    fprintf( stderr, "    d[%d]->%08x\n", i, (int) (p->nextb[i]) );
	}
#endif
    }
}
#endif

static int byte_tab[ 256 ];

void init_byte_tab( int reverse )
{
int i;
    if ( reverse ) for ( i=0; i<256; i++ ) byte_tab[i] = i;
    else
      for ( i=0; i<256; i++ )
	     byte_tab[i] = ( ((i & 0x01) << 7) | ((i & 0x02) << 5) |
			     ((i & 0x04) << 3) | ((i & 0x08) << 1) |
			     ((i & 0x10) >> 1) | ((i & 0x20) >> 3) |
			     ((i & 0x40) >> 5) | ((i & 0x80) >> 7) );
}
     
struct g3_tree * black, * white;

#define CHUNK 2048;
static	char rbuf[2048];	/* read buffer */
static	int  rp;		/* read pointer */
static	int  rs;		/* read buffer size */

#define MAX_ROWS 4300
#define MAX_COLS 1728		/* !! FIXME - command line parameter */

int main( int argc, char ** argv )
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
int	col;

    /* initialize lookup trees */
    build_tree( &white, t_white );
    build_tree( &white, m_white );
    build_tree( &black, t_black );
    build_tree( &black, m_black );

    init_byte_tab( 0 );

    i = 1;
    while ( argv[i][0] == '-' )		/* option processing */
    {
	if ( argv[i][1] == 'r' )	/* -reversebits */
	{
	    init_byte_tab( 1 );
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
    row = col = 0;
    bitmap = (char *) calloc( MAX_ROWS, MAX_COLS / 8 );
    if ( bitmap == NULL )
    {
	fprintf( stderr, "cannot allocate %d bytes",
		 MAX_ROWS * MAX_COLS/8 );
	close( fd );
	exit(9);
    }
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
		if ( rs == 0 ) { fprintf( stderr, "EOF!" ); goto write; }
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
			if ( rs == 0 ) goto write;
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
			if ( rs == 0 ) goto write;
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
		row++; col=0; bp = &bitmap[ row * MAX_COLS/8 ]; 
		cons_eol = 0;
	    }
	}
	else		/* not eol */
	{
	    if ( col+nr_pels > MAX_COLS ) nr_pels = MAX_COLS - col;

	    if ( color == 0 )
		col += nr_pels;
	    else
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
	    if ( nr_pels < 64 ) color = !color;		/* terminal code */
	}
    }		/* end main loop */
/*!! FIXME - MAX_COLS -> really used COLs */
write:		/* write pbm (or whatever) file */

    if( fd != 0 ) close(fd);	/* close input file */

#ifdef DEBUG
    fprintf( stderr, "consecutive EOLs: %d\n", cons_eol );
#endif

    sprintf( rbuf, "P4\n%d %d\n", MAX_COLS, row );
    write( 1, rbuf, strlen( rbuf ));

    write( 1, bitmap, (MAX_COLS/8) * row );

    return 0;
}
