#ident "$Id: config.c,v 1.6 1995/07/24 15:22:11 gert Exp $ Copyright (c) 1993 Gert Doering"

/*
 * config.c
 *
 * This module is part of the mgetty kit - see LICENSE for details
 *
 * Read and parse config file(s), see conf_*.[ch] for example use
 */

#include <stdio.h>
#include "syslibs.h"
#include <ctype.h>
#include <string.h>
#ifndef ENOENT
#include <errno.h>
#endif

#include "mgetty.h"
#include "config.h"

/* read a line from FILE * fp, terminated by "\n"
 * the line can be of any length (buffer will dynamically grow)
 * - trailing "\n" is chopped off
 * - continuation lines (trailing "\") will automatically be put together
 * - comment lines (leading "#") will be skipped
 */

char * fgetline _P1((fp), FILE * fp )
{
static int	bufsz = 0;
static char *	bufp = NULL;
int		bufidx;
char	*	p;

    if ( bufp == NULL )
    {
	if ( ( bufp = malloc( bufsz = 1024 ) ) == NULL )
	{
	    lprintf( L_ERROR, "fgetline: cannot allocate memory" );
	    return NULL;
	}
    }

    bufidx = 0;

    while ( TRUE )
    {
        p = fgets( &bufp[ bufidx ], bufsz-bufidx, fp );

	if ( p == NULL )	/* nothing more to read */
	    if ( bufidx == 0 )
	    {
		free( bufp ); bufp = NULL; bufsz = 0;
		return NULL;
	    }
	    else break;

	/* continuation lines? Buffer overflow? */

	bufidx += strlen( &bufp[ bufidx ] );

	/* discard trailing '\n' */

	if ( bufidx > 0 && bufp[ bufidx-1 ] == '\n' )
	    bufp[ --bufidx ] = 0;

	/* buffer full */
	if ( bufidx == bufsz-1 )
	{
	    lprintf( L_NOISE, "realloc line" );
	    if ( ( p = realloc( bufp, bufsz += 1024 ) ) == NULL )
	    {
		free( bufp ); bufp = NULL; bufsz = 0;
		lprintf( L_ERROR, "fgetline: cannot realloc" );
		return NULL;
	    }
	    bufp = p;
	    continue;
	}

	/* continuation lines */
	if ( bufidx > 0 && bufp[ bufidx-1 ] == '\\' )
	{
	    bufidx--; continue;
	}

	/* comments */
	if ( bufidx > 0 )
	{
	    char * sp = bufp;
	    while( isspace( *sp ) ) sp++;		/* skip whitespace */
	    
	    if ( *sp == '#' )
	    {
		bufidx = 0; continue;
	    }
	}
	break;
    }

    return bufp;
}

/* compress whitespace, drop leading / trailing whitespace,
 * put pointer to key word into *key
 */

void norm_line _P2( (line, key), char ** line, char ** key )
{
char * r, *wp;
int  w, kflag;

    r = wp = *line;
    w = 0;
    kflag = 0;

    while ( *r )
    {
	if ( isspace( *r ) )
	{
	    if ( w > 0 && wp[ w-1 ] != ' ' )
	    {
		if ( kflag == 0 ) kflag = w;
		wp[ w++ ] = ' ';
	    }
	}
	else wp[ w++ ] = *r;
	r++;
    }
    wp[ w ] = 0;
    if ( w > 0 && wp[ w-1 ] == ' ' ) w--;
    wp[ w ] = 0;

    /* set key / line pointers */
    *key = wp;
    if ( kflag == 0 )
	*line = &wp[w];
    else
	{ wp[ kflag ] = 0; *line = &wp[ kflag+1 ]; }
}

/* change a input line into a (char ** ) chat sequence
 * all the data is in one malloc() block, so one free() suffices
 */

void * conf_get_chat _P1( (line), char * line )
{
int	cnt, i;
int	quote;
char ** p;
char *  s;

    quote=0;
    for ( cnt = i = 0; line[i]; i++ )
    {
	if ( line[i] == '"' ) quote = !quote;
	if ( line[i] == ' ' && !quote ) cnt++;
    }
    cnt+=2;

/* lprintf( L_JUNK, "gc: %d strings", cnt-1 ); */

    p = (char **) malloc( cnt * sizeof( char * ) + strlen(line) +1 );
    if ( p == 0 )
        { lprintf( L_ERROR, "conf_get_chat: cannot malloc" ); return NULL; }

    s = (char * ) &p[cnt];		/* pointer to data area */

    quote = 0;
    p[ cnt=0 ] = s;
    while( *line )
    {
	if ( *line == '"' ) quote = !quote;
	else
	  if ( *line == ' ' && ! quote )
	{
	    *(s++) = 0; cnt++; p[cnt] = s;
	}
	else
	  *(s++) = *line;

	line++;
    }

    /* terminate last string and ptr list */
    *s = 0;
    cnt++; p[cnt] = NULL;

/*     for ( i=0; p[i]; i++ )
	lprintf( L_JUNK, "chat string %d = '%s'", i, p[i] );
*/
    return p;
}

/* write the config structure into the log file */
void display_cd _P1( (cd), conf_data * cd )
{
conf_data * cp;
char ** p;

    cp = cd;
    while ( cp->key != NULL )
    {
    char buf[100];
	lprintf( L_NOISE, "key: '%s', type=%d, flags=%d, data=",
		cp->key, cp->type, cp->flags );
	if ( cp->flags == C_EMPTY )
		lputs( L_NOISE, "(empty)" );
	else
	if ( cp->flags == C_IGNORE )
		lputs( L_NOISE, "(ignored)" );
	else
	  switch ( cp->type )
	{
	    case CT_INT: sprintf( buf, "%d", cp->d.i );
			 lputs( L_NOISE, buf );
			 break;
	    case CT_STRING:
			 lputs( L_NOISE, (char *) cp->d.p );
			 break;
	    case CT_BOOL:
			 lputs( L_NOISE, cp->d.i ? "TRUE" : "FALSE" );
			 break;
	    case CT_CHAT:
			 p = (char **) cp->d.p;
			 while ( *p != NULL )
			 {
			     lputs( L_NOISE, *p ); lputc( L_NOISE, ' ');
			     p++;
			 }
			 break;
	    default:
			 lputs( L_NOISE, "**unprintable**" );
			 break;
	}
	cp++;
    }
}


int get_config _P4( (conf_file,cd,section_key,key_value),
		char * conf_file, conf_data * cd,
		char * section_key, char * key_value )
{
FILE * conf_fp;
char * line;
char * key;
conf_data * cp;
int errflag = 0;
int ignore = 0;		/* ignore keywords in non-matching section */

    conf_fp = fopen( conf_file, "r" );
    if ( conf_fp == NULL )
    {
	if ( errno == ENOENT )
	    lprintf( L_WARN, "no config file found: %s", conf_file );
	else
	    lprintf( L_FATAL, "cannot open %s", conf_file );
	return ERROR;
    }

/* display_cd ( cd ); */

    while ( ( line = fgetline( conf_fp ) ) != NULL )
    {
	norm_line( &line, &key );
	if ( key[0] == 0 ) continue;		/* empty line */

	lprintf( L_NOISE, "conf lib: read: '%s %s'", key, line );

	/* sort in data */
	errflag = 0;
	cp = cd;

	if ( strcmp( key, section_key ) == 0 )	/* new section */
	{
	    ignore = ( key_value == NULL ) ||	/* match "wanted" section? */
		     ( strcmp( line, key_value ) != 0 );
	    lprintf( L_NOISE, "section: %s %s, %s",
		     key, line, (ignore)?"ignore":"**found**" );
	}
	else if ( ! ignore ) 
	  while ( cp->key != NULL )
	{
	    if ( strcmp( cp->key, key ) == 0 )
	    {
		if ( cp->flags == C_CONF &&
		     ( cp->type == CT_STRING || cp->type == CT_CHAT ) )
		{
		    free( cp->d.p ); cp->d.p = NULL;
		}

		if ( cp->flags != C_OVERRIDE && cp->flags != C_IGNORE )
		{
		  switch( cp->type )
		    {
		      case CT_INT:
			if ( ! isdigit( line[0] ) ) errflag ++;
			else cp->d.i = strtol( line, NULL, 0 );
			break;
		      case CT_STRING:
			if ( ( cp->d.p = malloc( strlen( line ) +1 ) ) == NULL )
				errflag ++;
			else
				strcpy( cp->d.p, line );
			break;
		      case CT_CHAT:
			if ( ( cp->d.p = conf_get_chat( line ) ) == NULL )
				errflag ++;
			break;
		      case CT_BOOL:
			cp->d.i = ( line[0] == 0 || line[0] == 1 ||
				    tolower(line[0]) == 'y' ||
				    tolower(line[0]) == 't' ||
				    strncmp( line, "on", 2 ) == 0 );
			break;
		      default:
			lprintf( L_ERROR, "yet unable to handle type %d",
			         cp->type );
			errflag ++;
			break;
		    }
		    cp->flags = C_CONF;
		}
		break;
	    }
	    cp++;
	}
	if ( cp->key == NULL || errflag )
	{
	    lprintf( L_WARN, "something foul in config line: '%s %s'",
	             key, line );
	    if ( cp->key == NULL )
		lprintf( L_WARN, "    (keyword '%s' not found)", key );
	    else
		lprintf( L_WARN, "    (most likely syntax error)" );
	}
    }

    display_cd ( cd );

    fclose( conf_fp );
    return NOERROR;
}

#ifndef makepath
/* makepath
 *
 * only needed on non-ANSI-compilers
 */

char * makepath _P2( (file, path), char * file, char * path )
{
    char * p;

    if ( file[0] == '/' ) return file;

    p = malloc( strlen( path ) + strlen( file ) +2 );

    if ( p == NULL )
    {
	lprintf( L_FATAL, "malloc error (in makepath)" ); exit(1);
    }

    sprintf( p, "%s/%s", path, file );
    return p;
}
#endif
