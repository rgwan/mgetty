#ident "$Id: goodies.c,v 2.2 1994/12/23 13:00:20 gert Exp $ Copyright (c) 1993 Gert Doering"

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
#include <unistd.h>

#include <pwd.h>
#include <grp.h>

#include "mgetty.h"
#include "config.h"

#ifdef SVR4
# include <sys/procfs.h>
# include <fcntl.h>
#endif

#ifdef linux
# include <fcntl.h>
# include <unistd.h>
#endif

/* get the base file name of a file path */

char * get_basename _P1( (s), char * s )
{
char * p;

    if ( s == NULL ) return NULL;

    p = strrchr( s, '/' );

    return ( p == NULL ) ? s: p+1;
}

/* auxiliary function: get a uid/gid pair from two strings
 * specifying user and group
 */

void get_ugid _P4( (user, group, uid, gid),
		  conf_data * user, conf_data * group,
		  uid_t * uid, gid_t * gid )
{
    /* default */
    *uid = *gid = 0;

    if ( user->flags != C_EMPTY )		/* user set */
    {
	struct passwd *pwd;

	if ( isdigit( *(char*)(user->d.p) ))	/* numeric */
	    pwd = getpwuid( atoi( (char*) (user->d.p) ));
	else					/* string */
	    pwd = getpwnam( (char*)(user->d.p) );

	if ( pwd == NULL )
	    lprintf( L_ERROR, "can't get user id for '%s'", user->d.p );
	else
	{
	    *uid = pwd->pw_uid;
	    *gid = pwd->pw_gid;
	}
	endpwent();
    }


    /* if group is set, override group corresponding to user */
    if ( group->flags != C_EMPTY )
    {
	struct group * grp;

	if ( isdigit( *(char*)(group->d.p) ))	/* numeric */
	    grp = getgrgid( atoi( (char*)(group->d.p)) );
	else					/* string */
	    grp = getgrnam( (char*) (group->d.p) );

	if ( grp == NULL )
	    lprintf( L_ERROR, "can't get group '%s'", group->d.p );
	else
	    *gid = grp->gr_gid;

	endgrent();
    }
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

    procfd = open (procfname, O_RDONLY);
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
    return pscomm;
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

#ifdef NeXT
  /* provide dummy putenv() function */
  int putenv( char * s ) { return 0; }
#endif
