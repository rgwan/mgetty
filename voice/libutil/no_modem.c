/*
 * no_modem.c
 *
 * This file contains a dummy event routine.
 *
 */

#include "../include/voice.h"

char *lib_modem_no_modem_c = "$Id: no_modem.c,v 1.2 1998/01/21 10:24:40 marc Exp $";

static int no_modem_answer_phone(void)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: answer_phone called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_beep(int frequency, int length)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: beep called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_dial(char *number)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: dial called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_handle_dle(char data)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: handle_dle called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_init(void)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: init called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_message_light_off(void)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: message_light_off called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_message_light_on(void)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: message_light_on called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_start_play_file(void)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: start_play_file called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_reset_play_file(void)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: next_play_file called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_stop_play_file(void)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: stop_play_file called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_play_file(FILE *fd, int bps)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: play_file called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_record_file(FILE *fd, int bps)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: record_file called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_set_compression(int *compression, int *speed, int *bits)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: set_compression called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_set_device(int device)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: set_device called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_stop_dialing(void)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: stop_dialing called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_stop_playing(void)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: stop_playing called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_stop_recording(void)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: stop_recording called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_stop_waiting(void)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: stop_waiting called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_switch_to_data_fax(char *mode)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: switch_to_data_fax called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_voice_mode_off(void)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: voice_mode_off called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_voice_mode_on(void)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: voice_mode_on called", voice_modem_name);
     return(FAIL);
     }

static int no_modem_wait(int wait_timeout)
     {
     errno = ENOSYS;
     LPRINTF(L_ERROR, "%s: wait called", voice_modem_name);
     return(FAIL);
     }

voice_modem_struct no_modem =
     {
     "serial port",
     "none",
     "",
     "",
     "",
     "",
     0,
     "",
     "",
     "",
     "",
     "",
     "",
     "",
     "",
     "",
     "",
     "",
     "",
     "",
     "",
     "",
     "",
     "",
     "",
     "",
     "",
     &no_modem_answer_phone,
     &no_modem_beep,
     &no_modem_dial,
     &no_modem_handle_dle,
     &no_modem_init,
     &no_modem_message_light_off,
     &no_modem_message_light_on,
     &no_modem_start_play_file,
     &no_modem_reset_play_file,
     &no_modem_stop_play_file,
     &no_modem_play_file,
     &no_modem_record_file,
     &no_modem_set_compression,
     &no_modem_set_device,
     &no_modem_stop_dialing,
     &no_modem_stop_playing,
     &no_modem_stop_recording,
     &no_modem_stop_waiting,
     &no_modem_switch_to_data_fax,
     &no_modem_voice_mode_off,
     &no_modem_voice_mode_on,
     &no_modem_wait
     };
