/* $Id: mgetty.h,v 1.8 1993/03/06 19:38:23 gert Exp $ (c) Gert Doering */

/* stuff in logfile.c */

#define L_FATAL 0
#define L_ERROR 1
#define L_WARN 2
#define L_MESG 3
#define L_NOISE 4

extern	int	log_level;
extern	char	log_path[];
int lputc( int level, char ch );
int lputs( int level, char * s );
int lprintf();

/* various defines */

#ifndef linux
#define USE_POLL
#endif

#define TRUE (1==1)
#define FALSE (1==0)
#define FAIL	-1
#define SUCCESS	0

typedef	void	sig_t;
typedef	char	boolean;

#define MAXLINE 256		/* max. # chars in a line */
#define MAXPATH MAXLINE
#ifdef OLD_STUFF
#define STDIN	fileno(stdin)
#else
#define STDIN	0
#endif

typedef	enum	{ A_FAIL, A_FAX } action_t;
typedef struct	chat_actions {
			char * expect;
			action_t action; } chat_action_t ;

/* do_chat.c */
void delay( int waittime );
int do_chat( char * expect_send[],
	     chat_action_t actions[], action_t * action,
             int chat_timeout_time, boolean timeout_first,
             boolean locks );
int clean_line( int tenths );

/* locks.c */

int makelock(char * name);
boolean	checklock(char * name);
int readlock(char * name);
sig_t	rmlocks();

extern	char	*lock;

/* fax stuff */
void faxrec( void );

/* how long should I wait for a string from modem */
#define FAX_RESPONSE_TIMEOUT	120
/* how much time may pass while receiving a fax without getting data */
#define	FAX_PAGE_TIMEOUT	60

/********* system prototypes **************/
char * mktemp( char * template );

#include "policy.h"
