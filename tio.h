#ident "$Id: tio.h,v 1.3 1993/11/07 01:52:09 gert Exp $ Copyright (c) 1993 Gert Doering";

/* tio.h
 *
 * contains definitions / prototypes needed for tio.c
 *
 */

#if !defined( POSIX_TERMIOS ) && !defined( BSD_SGTTY ) && !defined( SYSV_TERMIO)
# if defined(linux) || defined(sun)
#  define POSIX_TERMIOS
# else
#  define SYSV_TERMIO
# endif
#endif

#ifdef SYSV_TERMIO

#undef POSIX_TERMIOS
#undef BSG_SGTTY
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
void tio_mode_raw    _PROTO (( TIO *t ));
void tio_mode_cbreak _PROTO (( TIO *t ));
void tio_mode_sane   _PROTO (( TIO *t, int set_clocal_flag ));
void tio_map_cr      _PROTO (( TIO *t, int perform_crnl_mapping ));
int  tio_set_flow_control  _PROTO(( TIO *t, int flowctrl_type ));
int  tio_set_flow_control2 _PROTO(( int fd, int clowctrl_type ));
void tio_carrier     _PROTO (( TIO *t, int carrier_sensitive ));
int  tio_toggle_dtr  _PROTO(( int fd, int msec_wait ));

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
#endif
