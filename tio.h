#ident "$Id: tio.h,v 1.10 1994/01/05 03:40:57 gert Exp $ Copyright (c) 1993 Gert Doering"
;
#ifndef __TIO_H__
#define __TIO_H__

/* tio.h
 *
 * contains definitions / prototypes needed for tio.c
 *
 */

#if !defined( POSIX_TERMIOS ) && !defined( BSD_SGTTY ) && !defined( SYSV_TERMIO)
# if defined(linux) || defined(sun) || defined(_AIX)
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
#ifndef CSWTCH
#define CSWTCH	000	/* cntl z */
#endif
#ifdef CNSWTCH
#define CNSWTCH	0
#endif
#ifndef CSUSP
#define CSUSP _POSIX_VDISABLE
#endif

/* hardware handshake flags */
#define FLOW_NONE	0x00
#define FLOW_HARD	0x01		/* rts/cts */
#define FLOW_XON_IN	0x02		/* incoming data, send xon/xoff */
#define FLOW_XON_OUT	0x04		/* send data, honor xon/xoff */
#define FLOW_SOFT	(FLOW_XON_IN | FLOW_XON_OUT)

/* function prototypes */
int  tio_get _PROTO (( int fd, TIO *t ));
int  tio_set _PROTO (( int fd, TIO *t ));
int  tio_set_speed   _PROTO (( TIO *t, int speed ));
int  tio_get_speed   _PROTO (( TIO *t ));
void tio_mode_raw    _PROTO (( TIO *t ));
void tio_mode_cbreak _PROTO (( TIO *t ));
void tio_mode_sane   _PROTO (( TIO *t, int set_clocal_flag ));
void tio_map_cr      _PROTO (( TIO *t, int perform_crnl_mapping ));
int  tio_set_flow_control  _PROTO(( int fd, TIO *t, int flowctrl_type ));
int  tio_set_flow_control2 _PROTO(( int fd, int flowctrl_type ));
void tio_carrier     _PROTO (( TIO *t, int carrier_sensitive ));
int  tio_toggle_dtr  _PROTO(( int fd, int msec_wait ));

extern struct	speedtab {
    unsigned short cbaud;	/* baud rate, e.g. B9600 */
    int	 nspeed;		/* speed in numeric format */
    char *speed;		/* speed in display format */
} speedtab[];

#ifdef USE_GETTYDEFS
typedef struct {
    char *tag;
    TIO before;
    TIO after;
    char *prompt;
    char *nexttag;
} GDE;

int	loadgettydefs _PROTO((char *s));
GDE	*getgettydef _PROTO((char *s));
#endif		/* USE_GETTYDEFS */
#endif		/* __TIO_H__ */
