#ident "$Id: tio.h,v 3.4 1996/02/25 22:23:32 gert Exp $ Copyright (c) 1993 Gert Doering"

#ifndef __TIO_H__
#define __TIO_H__

/* tio.h
 *
 * contains definitions / prototypes needed for tio.c
 *
 */

#if !defined( POSIX_TERMIOS ) && !defined( BSD_SGTTY ) && !defined( SYSV_TERMIO)
# if defined(linux) || defined(sunos4) || defined(_AIX) || defined(BSD) || \
     defined(SVR4) || defined(solaris2) || defined(m88k) || defined(M_UNIX)
#  define POSIX_TERMIOS
# else
#  define SYSV_TERMIO
# endif
#endif

#ifdef SYSV_TERMIO

#undef POSIX_TERMIOS
#undef BSD_SGTTY
#include <termio.h>
typedef struct termio TIO;
#endif

#ifdef POSIX_TERMIOS
#undef BSD_SGTTY
#include <termios.h>
typedef struct termios TIO;
#endif

#ifdef BSD_SGTTY
#include <sgtty.h>
typedef struct sgttyb TIO;
#endif

/* define some types for gettydefs.c */

#ifdef SYSV_TERMIO

/* You may have to look at sys/termio.h to determine the type of the
 * c_?flag structure members.
 */
typedef unsigned short tioflag_t;

#define TIONCC NCC
#endif

#ifdef POSIX_TERMIOS
typedef tcflag_t tioflag_t;
#define TIONCC NCCS
#endif

#if defined(BSD_SGTTY) && defined(USE_GETTYDEFS)
#include "cannot use /etc/gettydefs with sgtty (yet?)"
#endif

/* if not defined in the default header files, #define some important things
 */
#ifdef _AIX
#include <sys/ttychars.h>
#endif

#if	!defined(VSWTCH) && defined(VSWTC)
#define	VSWTCH	VSWTC
#endif

#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE '\377'
#endif

/* default control chars */
#ifndef CESC
#define	CESC	'\\'
#endif
#ifndef CINTR
#define	CINTR	0177	/* DEL */
#endif
#ifndef CQUIT
#define	CQUIT	034	/* FS, cntl | */
#endif
#ifndef CERASE
#define	CERASE	'\b'	/* BS, nonstandard */
#endif
#ifndef CKILL
#define	CKILL	'\025'	/* NAK, nonstandard */
#endif
#ifndef CEOF
#define	CEOF	04	/* cntl d */
#endif
#ifndef CSTART
#define	CSTART	021	/* cntl q */
#endif
#ifndef CSTOP
#define	CSTOP	023	/* cntl s */
#endif
#ifndef CEOL
#define	CEOL	000	/* cntl j */
#endif

#ifdef CSWTCH
# undef CSWTCH		/* usually ^z, unwanted here */
#endif
#define CSWTCH	000	/* <undef> */

#ifndef CSUSP
# ifdef SVR42
#  define CSUSP 026	/* cntl z */
# else
#  define CSUSP _POSIX_VDISABLE		/* have only job control aware */
					/* shells use it */
# endif
#endif

/* the following are used only if the corresponding V... defines are */
/* available, and that's only on SVR42 (as far as I know) */
#ifndef CDSUSP
#define CDSUSP		025	/* cntl y */
#endif
#ifndef CRPRNT
#define CRPRNT		000	/* <undef> */
#endif
#ifndef CFLUSH
#define CFLUSH		000	/* <undef> */
#endif
#ifndef CWERASE
#define CWERASE		000	/* <undef> */
#endif
#ifndef CLNEXT
#define CLNEXT		000	/* <undef> */
#endif

/* hardware handshake flags */
#define FLOW_NONE	0x00
#define FLOW_HARD	0x01		/* rts/cts */
#define FLOW_XON_IN	0x02		/* incoming data, send xon/xoff */
#define FLOW_XON_OUT	0x04		/* send data, honor xon/xoff */
#define FLOW_SOFT	(FLOW_XON_IN | FLOW_XON_OUT)
#define FLOW_BOTH	(FLOW_HARD | FLOW_SOFT )
#define FLOW_XON_IXANY	0x08		/* set IXANY flag together with IXON */

/* queue selection flags (for tio_flush_queue) */
#define TIO_Q_IN	0x01		/* incoming data queue */
#define TIO_Q_OUT	0x02		/* outgoing data queue */
#define TIO_Q_BOTH	( TIO_Q_IN | TIO_Q_OUT )

/* function prototypes */
int  tio_get _PROTO (( int fd, TIO *t ));
int  tio_set _PROTO (( int fd, TIO *t ));
int  tio_check_speed _PROTO (( int speed ));
int  tio_set_speed   _PROTO (( TIO *t, unsigned int speed ));
int  tio_get_speed   _PROTO (( TIO *t ));
void tio_mode_raw    _PROTO (( TIO *t ));
void tio_mode_cbreak _PROTO (( TIO *t ));
void tio_mode_sane   _PROTO (( TIO *t, int set_clocal_flag ));
void tio_default_cc  _PROTO (( TIO *t ));
void tio_map_cr      _PROTO (( TIO *t, int perform_crnl_mapping ));
void tio_map_uclc    _PROTO (( TIO *t, int perform_case_mapping ));
int  tio_set_flow_control  _PROTO(( int fd, TIO *t, int flowctrl_type ));
int  tio_set_flow_control2 _PROTO(( int fd, int flowctrl_type ));
void tio_carrier     _PROTO (( TIO *t, int carrier_sensitive ));
int  tio_toggle_dtr  _PROTO(( int fd, int msec_wait ));
int  tio_flush_queue _PROTO(( int fd, int queue ));
int  tio_flow        _PROTO(( int fd, int restart_output ));
int  tio_break       _PROTO(( int fd ));
int  tio_drain_output _PROTO(( int fd ));

#ifdef USE_GETTYDEFS
typedef struct {
    char *tag;
    TIO before;
    TIO after;
    char *prompt;
    char *nexttag;
} GDE;

int	loadgettydefs _PROTO((char *s));
void	dumpgettydefs _PROTO((char *file));
GDE	*getgettydef _PROTO((char *s));
#endif		/* USE_GETTYDEFS */
#endif		/* __TIO_H__ */
