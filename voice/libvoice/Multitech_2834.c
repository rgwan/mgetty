/*
 * Multitech_2834.c
 *
 * This file contains the MultiTech 2834 specific hardware stuff.
 *
 * A first version was written by Russell King <rmk@ecs.soton.ac.uk>,
 * based on the ZyXEL 2864 driver.
 */

#include "../include/voice.h"

char *libvoice_Multitech_2834_c = "$Id: Multitech_2834.c,v 1.1 1997/12/16 12:21:01 marc Exp $";

/*
 * Here we save the current mode of operation of the voice modem when
 * switching to voice mode, so that we can restore it afterwards.
 */

static char mode_save[VOICE_BUF_LEN] = "";

static int Multitech_2834_answer_phone (void)
     {
     int result;

     reset_watchdog(0);

     if (((result = voice_command("AT+VLS=1", "OK|CONNECT")) & VMA_USER) !=
      VMA_USER)
          return(VMA_ERROR);

     if (result == VMA_USER_2)
          return(VMA_CONNECT);

     return(VMA_OK);
     }

static int Multitech_2834_init (void)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     voice_modem_state = INITIALIZING;
     lprintf(L_MESG, "initializing Multitech 2834 voice modem");

     /*
      * AT+VIT=10 - Set inactivity timer to 10 seconds
      */

     if (voice_command("AT+VIT=10", "OK") != VMA_USER_1)
          lprintf(L_WARN, "voice init failed, continuing");
#if 0
     /*
      * AT+VDD=x,y - Set DTMF tone detection threshold and duration detection
      */

     sprintf(buffer, "AT+VDD=%d,%d", cvd.dtmf_threshold.d.i *
      31 / 100, cvd.dtmf_len.d.i / 5);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "setting DTMF preferences didn't work");
#endif
     /*
      * AT+VSD=x,y - Set silence threshold and duration.
      */

     sprintf(buffer, "AT+VSD=%d,%d", /*cvd.rec_silence_threshold.d.i *
      1 / 100 +*/ 128, cvd.rec_silence_len.d.i);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "setting recording preferences didn't work");

     /*
      * AT+VGT - Set the transmit gain for voice samples.
      */

     if (cvd.transmit_gain.d.i == -1)
          cvd.transmit_gain.d.i = 50;

     sprintf(buffer, "AT+VGT=%d", cvd.transmit_gain.d.i * 144 / 100 +
      56);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "setting transmit gain didn't work");

     /*
      * AT+VGR - Set receive gain for voice samples.
      */

     if (cvd.receive_gain.d.i == -1)
          cvd.receive_gain.d.i = 50;

     sprintf(buffer, "AT+VGR=%d", cvd.receive_gain.d.i * 144 / 100 +
      56);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "setting receive gain didn't work");

     voice_modem_state = IDLE;
     return(OK);
     }

static int Multitech_2834_set_compression (int *compression, int *speed, int *bits)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);

     if (*compression == 0)
          *compression = 4;

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
          case 4:
               *bits = 4;
               sprintf(buffer, "AT+VSM=2,%d", *speed);

               if (voice_command(buffer, "OK") != VMA_USER_1)
                    return(FAIL);

               break;
          case 132:
               *bits = 4;
               sprintf(buffer, "AT+VSM=132,%d", *speed);

               if (voice_command(buffer, "OK") != VMA_USER_1)
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

static int Multitech_2834_set_device (int device)
     {
     reset_watchdog(0);

     switch (device)
          {
          case NO_DEVICE:
               voice_command("AT+VLS=0", "OK");
               return(OK);
          case LOCAL_HANDSET:
               voice_command("AT+VLS=11", "OK");
               return(OK);
          case DIALUP_LINE:
               voice_command("AT+VLS=2", "OK");
               return(OK);
          case EXTERNAL_MICROPHONE:
               voice_command("AT+VLS=11", "OK");
               return(OK);
          case INTERNAL_SPEAKER:
               voice_command("AT+VLS=4", "OK");
               return(OK);
          };

     lprintf(L_WARN, "%s: Unknown output device (%d)", voice_modem_name,
      device);
     return(FAIL);
     }

int Multitech_2834_voice_mode_off (void)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     if (!strncmp (mode_save, "+FCLASS=", 8))
     sprintf(buffer, "AT+FCLASS=%s", mode_save + 8);
     else
     sprintf(buffer, "AT+FCLASS=%s", mode_save);

     if (voice_command(buffer, "OK") != VMA_USER_1)
     return(FAIL);

     return(OK);
     }

int Multitech_2834_voice_mode_on (void)
     {
     reset_watchdog(0);

     if (voice_command("AT+FCLASS?", "") != OK)
     return(FAIL);

     do
     {

         if (voice_read(mode_save) != OK)
          return(FAIL);

     }
     while (strlen(mode_save) == 0);

     if (voice_command("", "OK") != VMA_USER_1)
     return(FAIL);

     if (voice_command("AT+FCLASS=8", "OK") != VMA_USER_1)
     return(FAIL);

     return(OK);
     }

voice_modem_struct Multitech_2834ZDXv =
     {
     "Multitech 2834ZDXv",
     "Multitech2834",
     &Multitech_2834_answer_phone,
     &IS_101_beep,
     &IS_101_dial,
     &IS_101_handle_dle,
     &Multitech_2834_init,
     &IS_101_message_light_off,
     &IS_101_message_light_on,
     &IS_101_play_file,
     &IS_101_record_file,
     &Multitech_2834_set_compression,
     &Multitech_2834_set_device,
     &IS_101_stop_dialing,
     &IS_101_stop_playing,
     &IS_101_stop_recording,
     &IS_101_stop_waiting,
     &IS_101_switch_to_data_fax,
     &Multitech_2834_voice_mode_off,
     &Multitech_2834_voice_mode_on,
     &IS_101_wait
     };
