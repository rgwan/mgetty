/* vm.h
 *
 * This file contains definitions for global variables and functions
 * for the VoiceModem program.
 *
 */

#include "../include/voice.h"

#ifdef MAIN
char *vm_h = "$Id: vm.h,v 1.2 1998/01/21 10:25:36 marc Exp $";
#endif

/*
 * DTMF code handling defines
 */

#define IGNORE_DTMF      (0)
#define READ_DTMF_DIGIT  (1)
#define READ_DTMF_STRING (2)

/*
 * Global variables prototypes
 */

extern int dtmf_mode;
extern char dtmf_string_buffer[VOICE_BUF_LEN];
extern int use_on_hook_off_hook;
extern int start_recording;

/*
 * Function prototypes
 */

extern int handle_event _PROTO((int event, event_data data));
extern void usage _PROTO(());
