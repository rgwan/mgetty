#ident "$Id: login.c,v 2.1 1994/11/30 23:20:44 gert Exp $ Copyright (C) 1993 Gert Doering"


/* login.c
 *
 * handle calling of login program(s) for data calls
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <ctype.h>
#ifndef EINVAL
#include <errno.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>


#include "mgetty.h"
#include "config.h"
#include "policy.h"
#include "mg_utmp.h"

#ifdef SECUREWARE
extern int setluid();
#endif

extern char * Device;			/* mgetty.c */

/* match( user, key )
 *
 * match "user" against "key"
 * key may start or end with "*" (wildcard)
 */
boolean match _P2( (user,key), char * user, char * key )
{
    int lk = strlen( key );
    int lu = strlen( user );
    int i;

    lprintf( L_NOISE, "match: user='%s', key='%s'", user, key );

    if ( lk == 0 )
    {
	return ( strlen( user) == 0 );
    }

#ifdef FIDO
    /* special handling for fido logins */
    if ( user[0] == '\377' && strcmp( key, "/FIDO/" ) == 0 )
    {
	return TRUE;
    }
#endif

    if ( key[0] == '*' )			/* "*bc?" */
    {
	if ( key[lk-1] == '*' )			/* "*bc*" */
	{
	    if ( lk < 2 ) return TRUE;		/* "*" or "**" */
	    for ( i=0; i <= lu - (lk-2); i++ )
	    {
		if ( strncmp( &user[i], &key[1], lk-2 ) == 0 ) return TRUE;
	    }
	    return FALSE;
	}
	else					/* "*bcd" */
	{
	    return ( ( lu >= lk-1 ) &&
		     ( strcmp( &user[ lu-(lk-1) ], &key[1] ) == 0 ) );
	}
    }
    else					/* "abc?" */
    {
	if ( key[lk-1] == '*' )			/* "abc*" */
	{
	    return ( ( lu >= lk-1 ) &&
		     ( strncmp( user, key, lk-1 ) == 0 ) ); 
	}
	else
	{
	    return ( ( lu == lk ) && 
		     ( strcmp( user, key ) == 0 ) );
	}
    }
    return FALSE;	/*NOTREACHED*/
}

/* execute login
 *
 * which login program is executed depends on LOGIN_FILE
 * default is "/bin/login user"
 *
 * does *NOT* return
 */

void login_dispatch _P1( (user), char * user )
{
    char * argv[10];
    int	argc = 0;
    char * cmd = NULL;
    int i;

    /* read "mgetty.login" config file */
#ifdef LOGIN_CFG_FILE
    FILE * fp = NULL;
    char * line, * key, *p;
    struct passwd * pw;
    extern struct passwd * getpwnam();

    struct stat st;

    /* first of all, some (somewhat paranoid) checks for file ownership,
     * file permissions (0i00), ...
     * If something fails, fall through to default ("/bin/login <user>")
     */       
       
    if ( stat( LOGIN_CFG_FILE, &st ) < 0 )
    {
	lprintf( L_ERROR, "login: stat('%s') failed", LOGIN_CFG_FILE );
    }
    else	/* permission check */
      if ( st.st_uid != 0 || ( ( st.st_mode & 0077 ) != 0 ) )
    {
	errno=EINVAL;
	lprintf( L_FATAL, "login: '%s' must be root/0600", LOGIN_CFG_FILE );
    }
    else
      if ( (fp = fopen( LOGIN_CFG_FILE, "r" )) == NULL )
    {
	lprintf( L_FATAL, "login: cannot open %s", LOGIN_CFG_FILE );
    }
    else
	while ( ( line = fgetline( fp ) ) != NULL )
    {
	norm_line( &line, &key );

	if ( match( user, key ) )
	{
	    char * user_id;
	    char * utmp_entry;
	    
	    lputs( L_NOISE, "*** hit!" );
#ifdef FIDO
	    if ( user[0] == '\377' && strcmp( key, "/FIDO/" ) == 0 )
	    {
		user++;
	    }
#endif

	    /* get (login) user id */
	    user_id = strtok( line, " \t" );

	    /* get utmp entry */
	    utmp_entry = strtok( NULL, " \t" );

	    /* get login program */
	    argv[0] = cmd = strtok( NULL, " \t" );

	    /* sanity checks - *before* setting anything */
	    errno = EINVAL;
	    
	    if ( user_id == NULL )
	    {
		lprintf( L_FATAL, "login: uid field blank, skipping line" );
		continue;
	    }
	    if ( utmp_entry == NULL )
	    {
		lprintf( L_FATAL, "login: utmp field blank, skipping line" );
		continue;
	    }
	    if ( cmd == NULL )
	    {
		lprintf( L_FATAL, "login: no login command, skipping line" );
		continue;
	    }

	    /* OK, all values given. Now write utmp entry */

	    if ( strcmp( utmp_entry, "-" ) != 0 )
	    {
		if ( strcmp( utmp_entry, "@" ) == 0 ) utmp_entry = user;

		lprintf( L_NOISE, "login: utmp entry: %s", utmp_entry );
		make_utmp_wtmp( Device, UT_USER, utmp_entry, Connect );
	    }

	    /* set UID (+login uid) */
	    
	    if ( strcmp( user_id, "-" ) != 0 )
	    {
		pw = getpwnam( user_id );
		if ( pw == NULL )
		{
		    lprintf( L_ERROR, "getpwnam('%s') failed", user_id );
		}
		else
		{
		    lprintf( L_NOISE, "login: user id: %s (uid %d, gid %d)",
				      key, pw->pw_uid, pw->pw_gid );
#if SECUREWARE
		    if ( setluid( pw->pw_uid ) == -1 )
		    {
			lprintf( L_ERROR, "cannot set LUID %d", pw->pw_uid);
		    }
#endif
		    if ( setgid( pw->pw_gid ) == -1 )
		    {
			lprintf( L_ERROR, "cannot set gid %d", pw->pw_gid );
		    }
		    if ( setuid( pw->pw_uid ) == -1 )
		    {
			lprintf( L_ERROR, "cannot set uid %d", pw->pw_uid );
		    }
		}
	    }				/* end if (uid given) */

	    /* now build 'login' command line */

	    /* strip path name off to-be-argv[0] */
	    p = strrchr( argv[0], '/' );
	    if ( p != NULL ) argv[0] = p+1;

	    /* break up line into whitespace-separated command line
	       arguments, substituting '@' by the user name
	       */
	    
	    argc = 1;
	    p = strtok( NULL, " \t" );
	    while ( argc < 9 && p != NULL )
	    {
		if ( strcmp( p, "@" ) == 0 )		/* user name */
		{
		    if ( user != NULL && user[0] != 0 )
		    {
			argv[argc++] = user;
		    }
		}
		else if ( strcmp( p, "\\I" ) == 0 )	/* Connect */
		{
		    argv[argc++] = Connect[0]? Connect: "??";
		}
		else
		    argv[argc++] = p;
		
		p = strtok( NULL, " \t" );
	    }

	    break;
	}		/* end if (matching line found) */
    }	/* end while( not end of config file ) */

    if ( fp != NULL ) fclose( fp );

#endif /* LOGIN_CFG_FILE */

    /* default to "/bin/login <user>" */
    if ( argc == 0 )
    {
	cmd = DEFAULT_LOGIN_PROGRAM;
	argv[argc++] = "login";

	/* append user name to argument list (if not empty) */
	if ( user[0] != 0 )
	{
	    argv[argc++] = user;
	}
    }

    /* terminate list */
    argv[argc] = NULL;

    /* verbose login message */
    lprintf( L_NOISE, "calling login: cmd='%s', argv[]='", cmd );
    for ( i=0; i<argc; i++) { lputs( L_NOISE, argv[i] ); 
			      lputs( L_NOISE, (i<argc-1)?" ":"'" ); }

    /* audit record */
    lprintf( L_AUDIT, 
       "data dev=%s, pid=%d, caller=%s, conn='%s', name='%s', cmd='%s', user='%s'",
	Device, getpid(), CallerId, Connect, CallName,
	cmd, user );

    /* execute login */
    execv( cmd, argv );

    /* FIXME: try via shell call */

    lprintf( L_FATAL, "cannot execute '%s'", cmd );
    exit(FAIL);
}
