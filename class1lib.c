#ident "$Id: class1lib.c,v 4.1 1997/12/12 12:03:49 gert Exp $ Copyright (c) Gert Doering"

/* class1lib.c
 *
 * Low-level functions to handle class 1 fax -- 
 * send a frame, receive a frame, dump frame to log file, ...
 */

#include <stdlib.h>
#include <unistd.h>

#include "mgetty.h"
#include "fax_lib.h"

/* static variables
 *
 * only set by functions in this module and used by other functions, 
 * but have to be module-global
 */

#define F1LID 20
static char fax1_local_id[F1LID];	/* local system ID */
static int fax1_min, fax1_max;		/* min/max speed */
static int fax1_res;			/* flag for normal resolution */

int fax1_set_l_id _P2( (fd, fax_id), int fd, char * fax_id )
{
    int i,l;
    char *p;

    l = strlen( fax_id );
    if ( l > F1LID ) { l = F1LID; }

    /* bytes are transmitted in REVERSE order! */
    p = &fax1_local_id[F1LID-1];

    for ( i=0; i<l; i++ )     *(p--) = *(fax_id++);
    for (    ; i<F1LID; i++ ) *(p--) = ' ';

    return NOERROR;
}


/* set fine/normal resolution flags and min/max transmission speed
 * TODO: find out maximum speed modem can do!
 */
int fax1_set_fdcc _P4( (fd, fine, max, min),
		       int fd, int fine, int max, int min )
{
    char * p;

    lprintf( L_MESG, "fax1_set_fdcc: fine=%d, max=%d, min=%d", fine, max, min );

    fax1_max = max/2400 -1;
    fax1_min = (min>2400)? min/2400 -1: 0;
    fax1_res = fine;

    lprintf( L_MESG, "max: %d, min: %d", fax1_max, fax1_min );

    if ( fax1_max < fax1_min ||
	 fax1_max < 1 || fax1_max > 5 ||
	 fax1_min < 0 || fax1_min > 5 ) 
    {
	fax1_min = 0; fax1_max = 5;
	return ERROR;
    }

    if ( fax1_res < 0 || fax1_res > 1 ) 
    {
	fax1_res = 1;
	return ERROR;
    }

    p = mdm_get_idstring( "AT+FTH=?", 1, fd );
    lprintf( L_MESG, "modem can send HDLC headers: '%s'", p );

    p = mdm_get_idstring( "AT+FTM=?", 1, fd );
    lprintf( L_MESG, "modem can send page data: '%s'", p );

    return NOERROR;
}

int fax1_set_bor _P2( (fd, bor), int fd, int bor )
{
    /*!!! TODO - what TODO ??! */
    return NOERROR;
}
