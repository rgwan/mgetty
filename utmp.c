#ident "$Id: utmp.c,v 1.17 1994/06/03 21:56:18 gert Exp $ Copyright (c) Gert Doering"
;
/* some parts of the code (writing of the utmp entry)
 * is based on the "getty kit 2.0" by Paul Sutcliffe, Jr.,
 * paul@devon.lns.pa.us, and are used with permission here.
 */

#if !defined(sun) && !defined(BSD)

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef ENOENT
#include <errno.h>
#endif

#if defined(_3B1_) || defined(MEIBE) || defined(ISC)
typedef short pid_t;
#endif

#endif

#include "mgetty.h"
#include "mg_utmp.h"

#if defined(sun) || defined(BSD) || defined(ultrix)
/* on SunOS (and other BSD-derived systems), the getty process does *
 * not have to care for the utmp entries, login and init do all the work
 * Anyway, we have to _read_ it to get the number of users logged in.
 */
void make_utmp_wtmp _P3( (line, ut_type, ut_user),
			 char * line, short ut_type, char * ut_user )
{
}
int get_current_users _P0( void )
{
    lprintf( L_WARN, "get_current_users: not implemented on BSD" );
    return 0;
}	/*! FIXME */
#else

void make_utmp_wtmp _P3( (line, ut_type, ut_user),
			 char * line, short ut_type, char * ut_user )
{
struct utmp *utmp;
pid_t	pid;
struct stat	st;
FILE *	fp;

    pid = getpid();
    lprintf(L_JUNK, "looking for utmp entry... (my PID: %d)", pid);

    while ((utmp = getutent()) != (struct utmp *) NULL)
    {
	if (utmp->ut_pid == pid &&
	    (utmp->ut_type == INIT_PROCESS || utmp->ut_type == LOGIN_PROCESS))
	{
	    strcpy(utmp->ut_line, line );

	    utmp->ut_time = time( NULL );

	    utmp->ut_type = ut_type;	/* {INIT,LOGIN,USER}_PROCESS */
	                                /* "LOGIN", "uugetty", "dialout" */
	    strncpy( utmp->ut_user, ut_user, sizeof( utmp->ut_user ) );
	    utmp->ut_user[ sizeof( utmp->ut_user ) -1 ] = 0;

#ifdef M_UNIX
	    if ( pututline(utmp) == NULL )
	    {
		lprintf( L_ERROR, "cannot create utmp entry" );
	    }
#else
	    /* Oh god, how I hate systems declaring functions as void... */
	    pututline( utmp );
#endif

	    /* write same record to end of wtmp
	     * if wtmp file exists
	     */
	    if (stat(WTMP_FILE, &st) && errno == ENOENT)
		    break;
	    if ((fp = fopen(WTMP_FILE, "a")) != (FILE *) NULL)
	    {
		(void) fseek(fp, 0L, SEEK_END);
		(void) fwrite((char *)utmp,sizeof(*utmp),1,fp);
		(void) fclose(fp);
	    }

	    lprintf(L_NOISE, "utmp + wtmp entry made");
	}
    }
    endutent();
}

int get_current_users _P0(void)
{
struct utmp * utmp;
int Nusers;

    Nusers = 0;
    setutent();
    while ((utmp = getutent()) != (struct utmp *) NULL) {
	if (utmp->ut_type == USER_PROCESS)
	{
	    Nusers++;
	    /*lprintf(L_NOISE, "utmp entry (%s)", utmp->ut_name); */
	}
    }
    endutent();
    return Nusers;
}
#endif		/* !sun */
