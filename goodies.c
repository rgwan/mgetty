#ident "$Id: goodies.c,v 1.3 1994/08/10 14:43:57 gert Exp $ Copyright (c) 1993 Gert Doering"

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

#ifdef SVR4
# include <sys/procfs.h>
# include <fcntl.h>
#endif

#ifdef linux
# include <fcntl.h>
#endif

/* get the base file name of a file path */

char * get_basename _P1( (s), char * s )
{
char * p;

    if ( s == NULL ) return NULL;

    p = strrchr( s, '/' );

    return ( p == NULL ) ? s: p+1;
}

/* return process name + arguments for process "PID"
 *
 * use /proc filesystem on Linux and SVR4
 *
 * if no information is available, return NULL
 */

char * get_ps_args _P1 ((pid), int pid )
{
#ifdef SVR4
    char *pscomm = NULL;
# ifdef PIOCPSINFO
    int procfd;
    char procfname[12];
    static prpsinfo_t psi;

    sprintf (procfname, "/proc/%05d", pid);

    procfd = open (procfname, O_RDONLY),;
    if ( procfd < 0 )
    {
	lprintf( L_ERROR, "cannot open %s", procfname );
    }
    else
    {
	if (ioctl (procfd, PIOCPSINFO, &psi) != -1)
	{
	    psi.pr_psargs[PRARGSZ-1] = '\0';
	    pscomm = psi.pr_psargs;
	}
	close(procfd);
    }
# endif /* PIOCPSINFO */
#endif /* SVR4 */

#ifdef linux

    char procfn[30];
    int procfd;
    int i,l;

    static char psinfo[60];	/* 60 is considered long enough */

    sprintf( procfn, "/proc/%d/cmdline", pid );

    procfd = open( procfn, O_RDONLY );

    if ( procfd < 0 )
    {
	lprintf( L_ERROR, "cannot open %s", procfn );
	return NULL;
    }

    l = read( procfd, psinfo, sizeof( psinfo ) -1 );

    if ( l < 0 )
    {
	lprintf( L_ERROR, "reading %s failed", procfn );
	close( procfd );
	return NULL;
    }

    close( procfd );

    psinfo[l] = 0;

    /* arguments separated by \0, replace with space */
    for ( i=0; i<l; i++ )
	if ( psinfo[i] == 0 ) psinfo[i] = ' ';

    return psinfo;
#endif /* linux */

#if !defined(SVR4) && !defined(linux)
    return NULL;
#endif
}
