#ident "$Id: config.h,v 1.3 1994/12/10 14:30:36 gert Exp $ Copyright (c) 1993 Gert Doering"

/* type definitions, prototypes, defines needed for configuration stuff
 */

typedef struct conf_data {
		   char * key;
		   union { int i; void * p; } d;
		   enum { CT_INT, CT_STRING, CT_CHAT, CT_BOOL,
			  CT_FLOWL, CT_ACTION } type;
		   enum { C_EMPTY, C_PRESET, C_OVERRIDE, C_CONF,
			  C_IGNORE } flags;
		 } conf_data;

int get_config _PROTO(( char * conf_file, conf_data * cd,
		        char * section_key, char * key_value ));

void display_cd _PROTO(( conf_data * cd ));

/* some versions of BSD have their own variant of fgetline that
 * behaves differently. Just change the name for now...
 * FIXME.
 */
#ifdef BSD
# define fgetline mgetty_fgetline
#endif

char * fgetline _PROTO(( FILE * fp ));
void   norm_line _PROTO(( char ** line, char ** key ));
void * conf_get_chat _PROTO(( char * line ));

#ifndef ERROR
#define ERROR -1
#define NOERROR 0
#endif


/* macros for effecient initializing of "conf_data" values */

#define conf_set_string( cp, s ) { (cp)->d.p = (s); (cp)->flags = C_OVERRIDE; }
#define conf_set_bool( cp, b )   { (cp)->d.i = (b); (cp)->flags = C_OVERRIDE; }
#define conf_set_int( cp, n )    { (cp)->d.i = (n); (cp)->flags = C_OVERRIDE; }

/* macros for implementation-indepentent access */
#define c_isset( cp )	( c.cp.flags != C_EMPTY )
#define c_string( cp )	((char *) c.cp.d.p)
#define c_bool( cp )	c.cp.d.i
#define c_int( cp )	c.cp.d.i
#define c_chat( cp )	((char **) c.cp.d.p)

/* concatenate two paths (if second path doesn't start with "/") */
/* two variants: ANSI w/ macro, K&R w/ C subroutine in config.c  */
#ifdef __STDC__
#define makepath( file, base ) ((file)[0] == '/'? (file) : (base"/"file))
#else
extern char * makepath _PROTO(( char * file, char * base ));
#endif

