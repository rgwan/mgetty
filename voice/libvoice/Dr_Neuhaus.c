/*
 * Dr_Neuhaus.c
 *
 * This file contains the Dr. Neuhaus Cybermod specific hardware stuff.
 *
 */

#include "../include/voice.h"

char *libvoice_Dr_Neuhaus_c = "$Id: Dr_Neuhaus.c,v 1.1 1997/12/16 12:20:58 marc Exp $";

static int Dr_Neuhaus_init (void)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     voice_modem_state = INITIALIZING;
     lprintf(L_MESG, "initializing Dr. Neuhaus voice modem");

     /*
      * AT+VSD=x,y - Set silence threshold and duration.
      */

     sprintf(buffer, "AT+VSD=%d,%d", cvd.rec_silence_threshold.d.i *
      10 / 100 + 123, cvd.rec_silence_len.d.i);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "setting recording preferences didn't work");

     /*
      * AT+VGT - Set the transmit gain for voice samples.
      */

     if (cvd.transmit_gain.d.i == -1)
          cvd.transmit_gain.d.i = 50;

     sprintf(buffer, "AT+VGT=%d", cvd.transmit_gain.d.i * 11 / 100 +
      122);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "setting transmit gain didn't work");

     /*
      * AT+VGR - Set receive gain for voice samples.
      */

     if (cvd.receive_gain.d.i == -1)
          cvd.receive_gain.d.i = 50;

     sprintf(buffer, "AT+VGR=%d", cvd.receive_gain.d.i * 11 / 100 +
      122);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "setting receive gain didn't work");

     voice_modem_state = IDLE;
     return(OK);
     }

static int Dr_Neuhaus_set_compression (int *compression, int *speed, int *bits)
     {
     reset_watchdog(0);

     if (*compression == 0)
          *compression = 129;

     if (*speed == 0)
          *speed = 8000;

     if (*speed != 8000)
          {
          lprintf(L_WARN, "%s: Illeagal sample rate (%d)", voice_modem_name,
           *speed);
          return(FAIL);
          };

     switch (*compression)
          {
          case 128:
               *bits = 8;

               if (voice_command("AT+VSM=128", "OK") != VMA_USER_1)
                    return(FAIL);

               break;
          case 129:
               *bits = 2;

               if (voice_command("AT+VSM=129", "OK") != VMA_USER_1)
                    return(FAIL);

               break;
          case 130:
               *bits = 3;

               if (voice_command("AT+VSM=130", "OK") != VMA_USER_1)
                    return(FAIL);

               break;
          default:
               lprintf(L_WARN, "%s: Illeagal voice compression method (%d)",
                voice_modem_name, *compression);
               return(FAIL);
          };

     IS_101_set_buffer_size((*speed) * (*bits) / 10 / 8);
     return(OK);
     }

static int Dr_Neuhaus_set_device (int device)
     {
     reset_watchdog(0);

     switch (device)
          {
          case NO_DEVICE:
               voice_command("AT+VLS=0", "OK");
               return(OK);
          case DIALUP_LINE:
               voice_command("AT+VLS=2", "OK");
               return(OK);
          case INTERNAL_SPEAKER:
               voice_command("AT+VLS=4", "OK");
               return(OK);
          };

     lprintf(L_WARN, "%s: Unknown output device (%d)", voice_modem_name,
      device);
     return(FAIL);
     }

voice_modem_struct Dr_Neuhaus =
     {
     "Dr. Neuhaus Cybermod",
     "Dr. Neuhaus",
     &IS_101_answer_phone,
     &IS_101_beep,
     &IS_101_dial,
     &IS_101_handle_dle,
     &Dr_Neuhaus_init,
     &IS_101_message_light_off,
     &IS_101_message_light_on,
     &IS_101_play_file,
     &IS_101_record_file,
     &Dr_Neuhaus_set_compression,
     &Dr_Neuhaus_set_device,
     &IS_101_stop_dialing,
     &IS_101_stop_playing,
     &IS_101_stop_recording,
     &IS_101_stop_waiting,
     &IS_101_switch_to_data_fax,
     &IS_101_voice_mode_off,
     &IS_101_voice_mode_on,
     &IS_101_wait
     };
