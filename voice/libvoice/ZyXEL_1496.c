/*
 * ZyXEL_1496.c
 *
 * This file contains the ZyXEL 1496 specific hardware stuff.
 *
 */

#include "../include/voice.h"

char *libvoice_ZyXEL_1496_c = "$Id: ZyXEL_1496.c,v 1.1 1997/12/16 12:21:05 marc Exp $";

static int ZyXEL_1496_answer_phone (void)
     {
     reset_watchdog(0);

     if (voice_command("AT+VLS=2", "VCON") != VMA_USER_1)
          return(VMA_FAIL);

     return(VMA_OK);
     }

static int ZyXEL_1496_beep (int frequency, int length)
     {
     return(IS_101_beep(frequency, length / 10));
     }

static int ZyXEL_1496_init (void)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     voice_modem_state = INITIALIZING;
     voice_command("ATI1", "");
     voice_read(buffer);
     voice_read(buffer);
     voice_command("", "OK");
     rom_release = 100 * (buffer[10] - '0') + 10 *
      (buffer[12] - '0') + (buffer[13] - '0');
     lprintf(L_MESG, "ROM release %4.2f detected", rom_release / 100.0);
     lprintf(L_MESG, "initializing ZyXEL 1496 voice modem");

     /*
      * ATS40.3=1 - Enable distincitve ring type 1 (RING)
      * ATS40.4=1 - Enable distincitve ring type 2 (RING 1)
      * ATS40.5=1 - Enable distincitve ring type 3 (RING 2)
      * ATS40.6=1 - Enable distincitve ring type 4 (RING 3)
      */

     if (voice_command("ATS40.3=1 S40.4=1 S40.5=1 S40.6=1", "OK") !=
      VMA_USER_1)
          lprintf(L_WARN, "coudn't initialize distinctive RING");

     /*
      * ATS39.6=1 - Enable DTMF detection after AT+VLS=2
      * ATS39.7=0 - Don't include resynchronization information
      *             in recorded voice data
      * AT+VIT=60 - Set inactivity timer to 60 seconds
      */

     if (voice_command("ATS39.6=1 S39.7=0 +VIT=60", "OK") != VMA_USER_1)
          lprintf(L_WARN, "voice init failed, continuing");

     /*
      * AT+VDH=x - Set the threshold for DTMF detection (0-32)
      * AT+VDD=x - Set DTMF tone duration detection
      */

     sprintf(buffer, "AT+VDH=%d +VDD=%d", cvd.dtmf_threshold.d.i *
      31 / 100, cvd.dtmf_len.d.i / 5);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "setting DTMF preferences didn't work");

     /*
      * AT+VSD=x,y - Set silence threshold and duration.
      */

     sprintf(buffer, "AT+VSD=%d,%d", cvd.rec_silence_threshold.d.i *
      31 / 100, cvd.rec_silence_len.d.i);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "setting recording preferences didn't work");

     /*
      * AT+VGT - Set the transmit gain for voice samples.
      */

     if (cvd.transmit_gain.d.i == -1)
          cvd.transmit_gain.d.i = 75;

     sprintf(buffer, "AT+VGT=%d", cvd.transmit_gain.d.i * 255 / 100);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "setting transmit gain didn't work");

     /*
      * AT+VGR - Set receive gain for voice samples.
      */

     if (cvd.receive_gain.d.i == -1)
          cvd.receive_gain.d.i = 75;

     sprintf(buffer, "AT+VGR=%d", cvd.receive_gain.d.i * 255 / 100);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "setting receive gain didn't work");

     /*
      * AT+VNH=1 - Disable autohangup
      */

     if (voice_command("AT+VNH=1", "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't disable autohangup");

     voice_modem_state = IDLE;
     return(OK);
     }

static int ZyXEL_1496_set_compression (int *compression, int *speed, int *bits)
     {
     reset_watchdog(0);

     if (*compression == 0)
          *compression = 2;

     if (*speed == 0)
          *speed = 9600;

     if (*speed != 9600)
          {
          lprintf(L_WARN, "%s: Illeagal sample rate (%d)", voice_modem_name,
           *speed);
          return(FAIL);
          };

     switch (*compression)
          {
          case 1:
               *bits = 1;

               if (voice_command("AT+VSM=1", "OK") != VMA_USER_1)
                    return(FAIL);

               break;
          case 2:
               *bits = 2;

               if (voice_command("AT+VSM=2", "OK") != VMA_USER_1)
                    return(FAIL);

               break;
          case 3:
               *bits = 3;

               if (voice_command("AT+VSM=3", "OK") != VMA_USER_1)
                    return(FAIL);

               break;
          case 30:
               *bits = 3;

               if (voice_command("AT+VSM=30", "OK") != VMA_USER_1)
                    return(FAIL);

               break;
          case 4:
               *bits = 4;

               if (voice_command("AT+VSM=4", "OK") != VMA_USER_1)
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

static int ZyXEL_1496_set_device (int device)
     {
     reset_watchdog(0);

     switch (device)
          {
          case NO_DEVICE:

               if (voice_write("AT+VLS=0") != OK)
                    return(FAIL);

               if (voice_command("", "AT+VLS=0|OK") == VMA_USER_1)
                    voice_command("", "OK");

               voice_command("AT+VNH=0", "OK");
               return(OK);
          case DIALUP_LINE:
               voice_command("AT+VLS=2", "VCON");
               return(OK);
          case EXTERNAL_MICROPHONE:
               voice_command("AT+VLS=8", "VCON");
               return(OK);
          case INTERNAL_SPEAKER:
               voice_command("AT+VLS=16", "VCON");
               return(OK);
          };

     lprintf(L_WARN, "%s: Unknown output device (%d)", voice_modem_name,
      device);
     return(FAIL);
     }

voice_modem_struct ZyXEL_1496 =
     {
     "ZyXEL 1496",
     "ZyXEL 1496",
     &ZyXEL_1496_answer_phone,
     &ZyXEL_1496_beep,
     &IS_101_dial,
     &IS_101_handle_dle,
     &ZyXEL_1496_init,
     &IS_101_message_light_off,
     &IS_101_message_light_on,
     &IS_101_play_file,
     &IS_101_record_file,
     &ZyXEL_1496_set_compression,
     &ZyXEL_1496_set_device,
     &IS_101_stop_dialing,
     &IS_101_stop_playing,
     &IS_101_stop_recording,
     &IS_101_stop_waiting,
     &IS_101_switch_to_data_fax,
     &IS_101_voice_mode_off,
     &IS_101_voice_mode_on,
     &IS_101_wait
     };
