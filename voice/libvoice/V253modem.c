/*
 * V253modem.c
 *
 * This file contains the commands for V253 complaient modems
 *
 */

#include "../include/voice.h"

static int V253modem_set_device (int device);

static int V253modem_init (void)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog();
     voice_modem_state = INITIALIZING;
     lprintf(L_MESG, "initializing V253 voice modem");

     sprintf(buffer, "AT+FCLASS=8");
     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "FCLASS=8");



     sprintf(buffer, "AT+VSD=%d,%d", cvd.rec_silence_threshold.d.i * 141 / 100, cvd.rec_silence_len.d.i);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set silence threshold VSD");

     sprintf(buffer, "ATS30=60");       /* fuer 38400 */

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set S30");

     if (cvd.transmit_gain.d.i == -1)
          cvd.transmit_gain.d.i = 50;

     sprintf(buffer, "AT+VGT=%d", cvd.transmit_gain.d.i * 127 / 100 +
      128);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set speaker volume");

     if (cvd.receive_gain.d.i == -1)
          cvd.receive_gain.d.i = 50;

     sprintf(buffer, "AT+VGR=%d", cvd.receive_gain.d.i * 127 / 100 +
      128);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set record volume");

     /* Delay after ringback or before any ringback
      * before modem assumes phone has been answered.
      */
     sprintf(buffer,
             "AT+VRA=%d+VRN=%d",
             cvd.ringback_goes_away.d.i,
             cvd.ringback_never_came.d.i);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "setting ringback delay didn't work");

     if ((cvd.do_hard_flow.d.i) && (voice_command("AT+IFC=2,2", "OK") ==
      VMA_USER_1) )
          {
          TIO tio;
          tio_get(voice_fd, &tio);
          tio_set_flow_control(voice_fd, &tio, FLOW_HARD);
          tio_set(voice_fd, &tio);
          }
     else
          lprintf(L_WARN, "can't turn on hardware flow control");

     voice_modem_state = IDLE;
     return(OK);
     }

static int V253modem_set_compression (int *compression, int *speed, int *bits)
     {
     char buffer[VOICE_BUF_LEN];
     reset_watchdog();

     if (*compression == 0)
          *compression = 8;

     if (*speed == 0)
          *speed = 8000;

     sprintf(buffer, "AT+VSM=1,%d,8", *speed);
     voice_command(buffer, "OK");   /* only no compression is common! */
     *bits=8;
     return(OK) ;


     }

static int V253modem_set_device (int device)
     {
     reset_watchdog();

     switch (device)
          {
          case NO_DEVICE:
               lprintf(L_JUNK, "%s: _NO_DEV: (%d)", voice_modem_name, device);
               voice_command("AT+VLS=0", "OK|VCON");
               return(OK);
          case DIALUP_LINE:
               lprintf(L_JUNK, "%s: _DIALUP: (%d)", voice_modem_name, device);
               voice_command("AT+VLS=1", "OK");
               return(OK);
          case EXTERNAL_MICROPHONE:
               voice_command("AT+VLS=11", "OK");
          case INTERNAL_MICROPHONE:
               lprintf(L_JUNK, "%s: _INT_MIC: (%d)", voice_modem_name, device);
               voice_command("AT+VLS=6", "OK|VCON");
               return(OK);
          case INTERNAL_SPEAKER:
               lprintf(L_JUNK, "%s: _INT_SEAK: (%d)", voice_modem_name, device);
               voice_command("AT+VLS=4", "OK|VCON");
               return(OK);
          }

     lprintf(L_WARN, "%s: Unknown device (%d)", voice_modem_name, device);
     return(FAIL);
     }

static char V253modem_pick_phone_cmnd[] = "AT+FCLASS=8; AT+VLS=2";
static char V253modem_pick_phone_answr[] = "VCON|OK";


static char V253modem_hardflow_cmnd[] = "AT+IFC=2,2";
static char V253modem_softflow_cmnd[] = "AT+IFC=1,1";

static char V253modem_beep_cmnd[] = "AT+VTS=[%d,,%d]";
/*static char V253modem_intr_play_answr[] = "OK|VCON";
static char V253modem_stop_play_answr[] = "OK|VCON";

static char V253modem_stop_rec_cmnd[] = "!";
static char V253modem_stop_rec_answr[] = "OK|VCON"; */


voice_modem_struct V253modem =
    {
    "V253 modem",
    "V253modem",
     (char *) V253modem_pick_phone_cmnd,
     (char *) V253modem_pick_phone_answr,
     (char *) V253modem_beep_cmnd,
     (char *) IS_101_beep_answr,
              IS_101_beep_timeunit,
     (char *) V253modem_hardflow_cmnd,
     (char *) IS_101_hardflow_answr,
     (char *) V253modem_softflow_cmnd,
     (char *) IS_101_softflow_answr,
     (char *) IS_101_start_play_cmnd,
     (char *) IS_101_start_play_answer,
     (char *) IS_101_reset_play_cmnd,
     (char *) IS_101_intr_play_cmnd,
     (char *) IS_101_intr_play_answr,
     (char *) IS_101_stop_play_cmnd,
     (char *) IS_101_stop_play_answr,
     (char *) IS_101_start_rec_cmnd,
     (char *) IS_101_start_rec_answr,
     (char *) IS_101_stop_rec_cmnd,
     (char *) IS_101_stop_rec_answr,
     (char *) IS_101_switch_mode_cmnd,
     (char *) IS_101_switch_mode_answr,
     (char *) IS_101_ask_mode_cmnd,
     (char *) IS_101_ask_mode_answr,
     (char *) IS_101_voice_mode_id,
     (char *) IS_101_play_dtmf_cmd,
     (char *) IS_101_play_dtmf_extra,
     (char *) IS_101_play_dtmf_answr,
    &IS_101_answer_phone,
    &IS_101_beep,
    &IS_101_dial,
    &IS_101_handle_dle,
    &V253modem_init,
    &IS_101_message_light_off,
    &IS_101_message_light_on,
    &IS_101_start_play_file,
    NULL,
    &IS_101_stop_play_file,
    &IS_101_play_file,
    &IS_101_record_file,
    &V253modem_set_compression,
    &V253modem_set_device,
    &IS_101_stop_dialing,
    &IS_101_stop_playing,
    &IS_101_stop_recording,
    &IS_101_stop_waiting,
    &IS_101_switch_to_data_fax,
    &IS_101_voice_mode_off,
    &IS_101_voice_mode_on,
    &IS_101_wait,
    &IS_101_play_dtmf,
    &IS_101_check_rmd_adequation,
    0
    };
