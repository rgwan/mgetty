#ident "$Id: utmp.c,v 1.8 1993/11/03 19:20:19 gert Exp $ Copyright (c) Gert Doering";
/* some parts of the code (writing of the utmp entry)
 * is based on the "getty kit 2.0" by Paul Sutcliffe, Jr.,
 * paul@devon.lns.pa.us, and are used with permission here.
 */

#ifndef sun

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utmp.h>

#ifndef ENOENT
#include <errno.h>
#endif

#if defined(_3B1_) || defined(MEIBE)
typedef short pid_t;
#endif

#endif

#include "mgetty.h"

#if defined(sun) || defined(BSD)
/* on SunOS (and other BSD-derived systems), the getty process does *
 * not have to care for the utmp entries, login and init do all the work
 * Anyway, we have to _read_ it to get the number of users logged in.
 */
void make_utmp_wtmp _P2( (line, login_process),
			 char * line, boolean login_process )
{
}
int get_current_users _P0( void )
{
    lprintf( L_WARN, "get_current_users: not implemented on BSD" );
    return 0;
}	/*! FIXME */
#else

/* define some prototypes - not all supported systems have these */
#if !defined(SVR4) && !defined(linux) && !defined(__hpux)
struct	utmp	*getutent _PROTO((void));
struct	utmp	*pututline _PROTO((struct utmp * utmp));
void		setutent _PROTO((void));
void		endutent _PROTO((void));
#endif

void make_utmp_wtmp _P2( (line, login_process),
			 char * line, boolean login_process )
{
struct utmp *utmp;
pid_t	pid;
struct stat	st;
FILE *	fp;

    pid = getpid();
    lprintf(L_JUNK, "looking for utmp entry... (my PID: %d)", pid);

    while ((utmp = getutent()) != (struct utmp *) NULL)
    {
	if (utmp->ut_type == INIT_PROCESS && utmp->ut_pid == pid)
	{
	    strcpy(utmp->ut_line, line );

	    utmp->ut_time = time( NULL );

	    if ( login_process )
	    {		/* show login process in utmp */
		strcpy(utmp->ut_user, "LOGIN");
		utmp->ut_type = LOGIN_PROCESS;
	    }
	    else
	    {		/* still waiting for call */
		strcpy(utmp->ut_user, "uugetty" );
		utmp->ut_type = INIT_PROCESS;
	    }

	    pututline(utmp);

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
