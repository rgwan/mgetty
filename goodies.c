#ident "$Id: goodies.c,v 1.1 1994/07/12 22:16:57 gert Exp $ Copyright (c) 1993 Gert Doering"
;
/*
 * goodies.c
 *
 * This module is part of the mgetty kit - see LICENSE for details
 *
 * various nice functions that do not fit elsewhere 
 */

#include <stdio.h>
#include "syslibs.h"
#include <ctype.h>
#include <string.h>

#include "mgetty.h"

/* get the base file name of a file path */

char * get_basename _P1( (s), char * s )
{
char * p;

    if ( s == NULL ) return NULL;

    p = strrchr( s, '/' );

    return ( p == NULL ) ? s: p+1;
}

