/*
 * voice.h
 *
 * This is the main header file for vgetty, vm and the pvf tools.
 * It includes other header files and defines some global variables.
 *
 */

#ifdef MAIN
# include "version.h"

# ifndef VOICE
#  include "../../version.h"
# endif

char *voice_h = "$Id: voice.h,v 1.1 1997/12/16 11:49:23 marc Exp $";
#endif

#ifndef _NOSTDLIB_H
# include <stdlib.h>
#endif

#include <stdio.h>
#include <unistd.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#if !defined( __bsdi__ ) && !defined(__FreeBSD__) && !defined(NeXT)
# include <malloc.h>
#endif

#include <math.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <netinet/in.h>

#ifndef VOICE
# include "../../mgetty.h"
# include "../../config.h"
# include "../../policy.h"
# include "../../tio.h"
# include "../../fax_lib.h"
#endif

#include "IS_101.h"
#include "bitsizes.h"
#include "util.h"
#include "config.h"
#include "default.h"
#include "event.h"
#include "hardware.h"
#include "header.h"
#include "paths.h"
#include "pvf.h"

/*
 * Buffer length for commands, voice modem answers and so on
 */

#define VOICE_BUF_LEN (1024)

/*
 * Program and release information
 */

extern char *vgetty_version;
extern char *mgetty_version;
extern char *program_name;

/*
 * Global variables
 */

extern int voice_fd;
extern int voice_shell_state;
extern int voice_shell_signal;
extern char voice_config_file[VOICE_BUF_LEN];
extern char *DevID;

/*
 * Vgetty global variables
 */

extern boolean virtual_ring;
extern int answer_mode;
extern char dtmf_code[VOICE_BUF_LEN];
extern int execute_dtmf_script;
extern int first_dtmf;
extern int hangup_requested;
extern int switch_to_data_fax_mode;

/*
 * The voice library functions
 */

extern int voice_analyze _PROTO((char *buffer, char *expected_answers));
#define voice_answer_phone() voice_modem->answer_phone()
#define voice_beep(a,b) voice_modem->beep(a,b)
extern int voice_close_device _PROTO((void));
extern int voice_command _PROTO((char *command, char *expected_answers));
extern int voice_config _PROTO((char *new_program_name, char *DevID));
extern int voice_detect_modemtype _PROTO((void));
#define voice_dial(a) voice_modem->dial(a)
extern int voice_execute_shell_script _PROTO((char *shell_script,
 char **shell_options));
extern int voice_flush _PROTO((int timeout));
#define voice_handle_dle(a) voice_modem->handle_dle(a)
extern int voice_handle_event _PROTO((int event, event_data data));
extern int voice_init _PROTO((void));
#define voice_message_light_off() voice_modem->message_light_off()
#define voice_message_light_on() voice_modem->message_light_on()
extern int voice_mode_on _PROTO((void));
extern int voice_mode_off _PROTO((void));
extern int voice_open_device _PROTO((void));
extern int voice_play_file _PROTO((char *name));
extern int voice_read _PROTO((char *buffer));
extern int voice_read_char _PROTO((void));
extern int voice_read_raw _PROTO((char *buffer, int count));
extern int voice_read_shell _PROTO((char *buffer));
extern int voice_register_event_handler _PROTO((int
 (*new_program_handle_event) (int event, event_data data)));
extern int voice_record_file _PROTO((char *name));
extern int voice_shell_handle_event _PROTO((int event, event_data data));
#define voice_set_device(a) voice_modem->set_device(a)
extern int voice_stop_current_action _PROTO((void));
#define voice_stop_dialing() voice_modem->stop_dialing()
#define voice_stop_playing() voice_modem->stop_playing()
#define voice_stop_recording() voice_modem->stop_recording()
#define voice_stop_waiting() voice_modem->stop_waiting()
#define voice_switch_to_data_fax(a) voice_modem->switch_to_data_fax(a)
extern int voice_unregister_event_handler _PROTO((void));
#define voice_wait(a) voice_modem->wait(a)
extern void reset_watchdog _PROTO((int count));
extern int voice_faxsnd _PROTO((char **name, int switchbd, int max_tries));
extern void voice_faxrec _PROTO(( char * spool_in, unsigned int switchbd));
extern int enter_fax_mode _PROTO((void));


#ifdef USE_VARARGS
extern int voice_write _PROTO(());
#else
extern int voice_write(const char *format, ...);
#endif

extern int voice_write_char _PROTO((char charout));
extern int voice_write_raw _PROTO((char *buffer, int count));

#ifdef USE_VARARGS
extern int voice_write_shell _PROTO(());
#else
extern int voice_write_shell(const char *format, ...);
#endif

/*
 * Internal functions
 */

extern void voice_check_events _PROTO((void));

/*
 * The vgetty functions
 */

extern int vgetty_answer _PROTO((int rings, int rings_wanted,
 action_t what_action));
extern void vgetty_button _PROTO((int rings));
extern void vgetty_create_message_flag_file _PROTO((void));
extern int vgetty_handle_event _PROTO((int event, event_data data));
extern void vgetty_message_light _PROTO((void));
extern void vgetty_rings _PROTO((int *rings_wanted));

/*
 * Value for voice_fd if the port isn't open
 */

#define NO_VOICE_FD (-1)

/*
 * Possible input or output devices
 */

#define NO_DEVICE           (0x0001)
#define DIALUP_LINE         (0x0002)
#define EXTERNAL_MICROPHONE (0x0003)
#define INTERNAL_MICROPHONE (0x0004)
#define EXTERNAL_SPEAKER    (0x0005)
#define INTERNAL_SPEAKER    (0x0006)
#define LOCAL_HANDSET       (0x0007)

/*
 * Voice modem answers
 */

/*
 * The user defined ones
 */

#define VMA_USER         (0x1000)
#define VMA_USER_1       (0x1000)
#define VMA_USER_2       (0x1001)
#define VMA_USER_3       (0x1002)
#define VMA_USER_4       (0x1003)
#define VMA_USER_5       (0x1004)
#define VMA_USER_6       (0x1005)
#define VMA_USER_7       (0x1006)
#define VMA_USER_8       (0x1007)
#define VMA_USER_9       (0x1008)
#define VMA_USER_10      (0x1009)
#define VMA_USER_11      (0x100a)
#define VMA_USER_12      (0x100b)
#define VMA_USER_13      (0x100c)
#define VMA_USER_14      (0x100d)
#define VMA_USER_15      (0x100e)
#define VMA_USER_16      (0x100f)

/*
 * The default ones
 */

#define VMA_DEFAULT      (0x2000)
#define VMA_BUSY         (0x2000)
#define VMA_CONNECT      (0x2001)
#define VMA_EMPTY        (0x2002)
#define VMA_ERROR        (0x2003)
#define VMA_FAX          (0x2004)
#define VMA_FCON         (0x2005)
#define VMA_FCO          (0x2006)
#define VMA_NO_ANSWER    (0x2007)
#define VMA_NO_CARRIER   (0x2008)
#define VMA_NO_DIAL_TONE (0x2009)
#define VMA_OK           (0x200a)
#define VMA_RINGING      (0x200b)
#define VMA_RING_1       (0x200c)
#define VMA_RING_2       (0x200d)
#define VMA_RING_3       (0x200e)
#define VMA_RING_4       (0x200f)
#define VMA_RING_5       (0x2010)
#define VMA_RING         (0x2011)
#define VMA_VCON         (0x2012)

/*
 * If something goes wrong
 */

#define VMA_FAIL         (0x4000)

/*
 * Possible voice modem and shell execution states
 */

#define DIALING          (0x0000)
#define IDLE             (0x0001)
#define INITIALIZING     (0x0002)
#define OFF_LINE         (0x0003)
#define ON_LINE          (0x0004)
#define PLAYING          (0x0005)
#define RECORDING        (0x0006)
#define WAITING          (0x0007)

/*
 * The different vgetty answer modes
 */

#define ANSWER_DATA  (1)
#define ANSWER_FAX   (2)
#define ANSWER_VOICE (4)

/*
 * Some tricks for vgetty
 */

#ifdef VOICE
# undef LOG_PATH
# define LOG_PATH VGETTY_LOG_PATH
#endif
