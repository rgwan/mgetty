#ident "$Id: login.c,v 1.3 1994/03/07 21:17:56 gert Exp $ Copyright (C) 1993 Gert Doering"
;

/* login.c
 *
 * handle calling of login program(s) for data calls
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <ctype.h>

#include "mgetty.h"
#include "config.h"
#include "policy.h"

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

void login _P1( (user), char * user )
{
    char * argv[10];
    int	argc = 0;
    char * cmd = NULL;
    int i;

    /* read "mgetty.login" config file */
#ifdef LOGIN_CFG_FILE
    FILE * fp;
    char * line, * key, *p;
    struct passwd * pw;
    extern struct passwd * getpwnam();

    fp = fopen( LOGIN_CFG_FILE, "r" );
    if ( fp == NULL )
    {
	lprintf( L_FATAL, "cannot open %s", LOGIN_CFG_FILE );
	/* fall through to default /bin/login */
    }
    else
	while ( ( line = fgetline( fp ) ) != NULL )
    {
	norm_line( &line, &key );

	if ( match( user, key ) )
	{
	    lputs( L_NOISE, "*** hit!" );

#ifdef FIDO
	    if ( user[0] == '\377' && strcmp( key, "/FIDO/" ) == 0 )
	    {
		user++;
	    }
#endif

	    /* get+set (login) user id */
	    norm_line( &line, &key );

	    if ( strcmp( key, "-" ) != 0 )
	    {
		pw = getpwnam( key );
		if ( pw == NULL )
		{
		    lprintf( L_ERROR, "getpwnam('%s') failed", key );
		}
		else
		{
		    lprintf( L_NOISE, "login user id: %s (uid %d, gid %d)",
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

	    /* get command + arguments */
	    argv[0] = cmd = p = line;
	    while( ! isspace(*p) && *p != 0 )
	    {
		if ( *p == '/' ) argv[0] = p+1;
		p++;
	    }

	    argc = 1;
	    while ( argc < 9 && *p != 0 )
	    {
		if ( *p != 0 )
		{
		    *p = 0; p++;
		    while ( *p != 0 && isspace(*p) ) p++;
		}
		argv[argc] = p;
		while ( *p != 0 && ! isspace(*p) ) p++;
		argc++;
	    }

	    break;
	}
    }	/* end while( not end of config file ) */
#endif /* LOGIN_CFG_FILE */

    /* default to /bin/login */
    if ( argc == 0 )
    {
	cmd = "/bin/login";
	argv[argc++] = "login";
    }

    /* append user name to argument list (if not empty) */
    if ( user != NULL && user[0] != 0 )
    {
	argv[argc++] = user;
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
