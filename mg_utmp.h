#ident "$Id: mg_utmp.h,v 1.4 1994/07/11 19:16:00 gert Exp $ Copyright (c) Gert Doering"
;
/* definitions for utmp reading / writing routines,
 * highly SysV / BSD dependent
 */

#if !defined(sunos4) && !defined(BSD) && !defined(ultrix) /* SysV style */

#include <utmp.h>

#define UT_INIT		INIT_PROCESS
#define UT_LOGIN	LOGIN_PROCESS
#define UT_USER		USER_PROCESS

#else						 /* SunOS or generic BSD */

#include <sys/types.h>
#include <utmp.h>

/* BSDish /etc/utmp files do not have the "ut_type" field */

#define UT_INIT		0
#define UT_LOGIN	0
#define UT_USER		0

#endif						/* SysV vs. BSD */

/* prototypes */

void make_utmp_wtmp _PROTO(( char * line, short ut_type, char * ut_user ));
int  get_current_users _PROTO(( void ));

/* system prototypes - not all supported systems have these */

#if defined(M_UNIX)

struct	utmp	*getutent _PROTO((void));
struct	utmp	*pututline _PROTO((struct utmp * utmp));
void		setutent _PROTO((void));
void		endutent _PROTO((void));

#endif /* M_UNIX */
