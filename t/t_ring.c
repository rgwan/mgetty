/* $Id: t_ring.c,v 1.1 2005/03/15 13:22:08 gert Exp $
 *
 * test program for mgetty "ring.c"
 *
 * feed wait_for_ring() via mdm_read_char() from here
 *
 * table driven: 
 *   <input string> <# rings> <dist-ring#> <caller id>
 *
 * $Log: t_ring.c,v $
 * Revision 1.1  2005/03/15 13:22:08  gert
 * regression test for "ring.c" module (feed fake character strings, see whether
 * ring count, dist.ring number/MSN, caller ID, etc. match)
 *
 */

#include "mgetty.h"
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#ifdef T_LOG_VERBOSE
# include <stdarg.h>
#endif

struct t_ring_tests { char * input;
		      int ring_count;
		      int dist_ring;
		      char * caller_id; } t_ring_tests[] = 
{{"RING\nRING\nRING\n", 3, -1, "" },
 {"RING 2\n", 1, 2, "" },
 {"RING A\nRING B\nRING C\nRING\n", 4, 3, "" },
 {"RING\n    FM:040404\n", 2, 0, "040404" },	/* ZyXEL + whitespc */
 {"RING\nNMBR = 0555\nRING\n", 3, -1, "0555" },	/* Rockwell */
 {"RING/0666\n", 1, 0, "" },			/* i4l - RING/to */
 {"RING;707070\n",      1, 0, "707070" },	/* ELSA - RING;from */
 {"RING;717171;7979\n", 1, 0, "717171" },	/* ELSA - RING;from;to */
 {"RING: 3 DN9 8888\n", 1, 9, "8888" },		/* Zoom */
 {"RING 090909\n", 1, 0, "090909" },		/* USR Type B */
 {"DROF=0\nDRON=11\nRING\nDROF=40\nDRON=20\nRING\n", 2, 3, "" },
						/* V.253 dist ring */
 {"\020R\n\020R\n", 2, 0, "" },			/* voice mode RING */
 {NULL, 0, 0, NULL }};

static char * read_p;

/* fake logging functions */
int lputc( int level, char ch ) { return 0; }
int lputs( int level, char * s ) { return 0; }
int lprintf( int level, const char * format, ...) 
#ifdef T_LOG_VERBOSE
    { va_list pvar; va_start( pvar, format ); 
      vprintf( format, pvar ); putchar('\n'); }
#else
    { return 0; }
#endif

/* fake modem read function */
int mdm_read_byte( int fd, char * c )
{
    while( *read_p != '\0' )
	{ *c = *read_p++; return 1; }

    /* nothing more in buffer -> pretend timeout */
    raise(SIGALRM);
    errno = EINTR;
    return -1;
}

boolean virtual_ring = FALSE;

int main( int argc, char ** argv )
{
    int rings, dist_ring;
    action_t what_action;
    struct t_ring_tests *t = t_ring_tests;
    int i;
    int fail = 0;

    i = 1;
    while( t->input != NULL )
    {
        read_p = t->input;
	rings = 0;
	dist_ring = -1;
	CallerId = "";
	while( wait_for_ring( STDIN, NULL, 10, NULL /* actions */,
			       &what_action, &dist_ring ) == SUCCESS )
	{
	    rings ++;
	}

	if ( rings != t->ring_count )
	{
	    fprintf( stderr, " %02d failed: rings=%d, should be %d\n",
				i, rings, t->ring_count );
	    fail++;
	}
	if ( dist_ring != t->dist_ring )
	{
	    fprintf( stderr, " %02d failed: dist_ring=%d, should be %d\n",
				i, dist_ring, t->dist_ring );
	    fail++;
	}
	if ( strcmp( CallerId, t->caller_id ) != 0  )
	{
	    fprintf( stderr, " %02d failed: caller_id='%s', should be '%s'\n",
				i, CallerId, t->caller_id );
	    fail++;
	}
	/* TODO: test actions */
	/* TODO: feed with msn_list (char **, 2nd parameter) */
	t++; i++;
    }

    if ( fail>0 )
	fprintf( stderr, "total: %d failed tests\n", fail );
    return (fail>0);
}
