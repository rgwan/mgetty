/*
 * US_Robotics.c V0.4b3
 *
 * This file contains hardware driver functions for some USRobotics modems.
 * Made from compilations from the old US_Robotics driver, originally from
 * Steven wormley <wormley@step.mother.com> and the generic file IS_101.c
 * Thomas Hellstroem <thomas@vtd.volvo.se> 1996-11-14
 * Revision history:
 * 1996-11-14
 * V0.1 :Complete rewrite for the new vgetty driver interface. Added some
 *       stuff to disable local echoing of data while playing.
 * 1996-11-25
 * V0.2 :Fixed the fax & data handling to fit vgetty-0.99.4. Fixed
 *       the beeping length to fit IS_101 definition.
 * 1996-12-05
 * V0.3 :Changed compression types and some initializing stuff that causes
 *       problems with some modems. The driver now supports ADPCM. The
 *       switch-to-data-fax inconsistency is now hopefulle resolved. Note
 *       that te ADPCM-compatibility probing is somewhat dirty.
 * 1996-12-09
 * V0.4 :Moved all voice configuring commands so that they occur where they're
 *       relevant and in voice mode. Software flow control is no longer
 *       supported. b2:
 *
 * $Id: US_Robotics.c,v 1.4 1998/09/09 21:07:24 gert Exp $
 *
 */

#include "../include/voice.h"

#define COMPRESSION_DEFAULT 0
#define COMPRESSION_GSM 1
#define COMPRESSION_ADPCM_2 2
#define COMPRESSION_ADPCM_3 3
#define COMPRESSION_ADPCM_4 4

/*
 * Internal status variables for aborting some voice modem actions.
 */

static int supports_adpcm = FALSE;
static int probed_for_adpcm = FALSE;
static int in_adpcm_mode = FALSE;
static int internal_speaker_used = FALSE;

static int USR_beep(int frequency, int length)
     {

     /*
      * Ugly hack: my USRobotics SportsterFlash doesn't catch dtmf codes
      * after a beep.  If I send it the "AT#VLS=4A" (answer phone) string
      * after an "AT#VTS[%d,0,%d]" (beep) command it would resume detecting
      * tones
      * 
      */
      
     if (IS_101_beep(frequency, length) != OK)
          return(FAIL);
          
     if (IS_101_answer_phone() != VMA_OK)
          return(FAIL);
     
     return(OK);
     } 

static int USR_init(void)
     {
     static char buffer[VOICE_BUF_LEN];

     reset_watchdog();
     lprintf(L_MESG, "US Robotics voice modem");
     lprintf(L_WARN, "This is a driver beta version. V0.4.b3");
     voice_modem_state = INITIALIZING;

     if (voice_command("AT&H1&R2&I0", "OK") == VMA_USER_1)
          {
          TIO tio;
          tio_get(voice_fd, &tio);
          tio_set_flow_control(voice_fd, &tio, FLOW_HARD);
          tio_set(voice_fd, &tio);
          }
     else
          lprintf(L_WARN,"can't turn on hardware flow control");

     if (voice_command("AT#VTD=3F,3F,3F", "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set DLE responses");
     else
          lprintf(L_WARN, "VTD setup successful");

     /*
      * Set silence threshold and length. Must be in voice mode to do this.
      */

     sprintf(buffer, "AT#VSD=1#VSS=%d#VSP=%d",
      cvd.rec_silence_threshold.d.i * 3 / 100,
      cvd.rec_silence_len.d.i);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN,"setting recording preferences didn't work");

     voice_modem_state = IDLE;
     return(OK);
     }

static int USR_stop_play_file(void)
     {
     int stop_playing = IS_101_stop_play_file();

     if (cvd.enable_command_echo.d.i)
          {

          if (voice_write_raw("ATE1\r",5) != OK)
               return(FAIL);

          if ((voice_command("", "OK|VCON") & VMA_USER) != VMA_USER)
               return(FAIL);
               
          }

     if (voice_command("AT", "OK") != VMA_USER_1)
          return(FAIL);

     voice_modem_state = IDLE;
     voice_check_events();

     if (stop_playing)
          return(INTERRUPTED);

     return(OK);
     }

static int USR_set_compression(int *compression, int *speed, int *bits)
     {
     char buffer[VOICE_BUF_LEN];

     /*
      * 8000 Hz is currently the only recording freq supported by sportster
      * Vi and voice
      */

     if (*compression == 0)
          *compression = COMPRESSION_GSM;

     if (*speed == 0)
          *speed = 8000;

     if (*speed != 8000)
          {
          lprintf(L_WARN, "%s: Illegal sample rate (%d)",
           voice_modem_name, *speed);
          return(FAIL);
          };

     /*
      * Does the modem support ADPCM?
      */

     if ((!probed_for_adpcm) && (*compression != COMPRESSION_GSM))
          {

          if (!(supports_adpcm =
           (voice_command("AT#VSM=129,8000", "OK") == VMA_USER_1)))
               lprintf(L_WARN,"%s: Ignore the above error!", program_name);
          else
               in_adpcm_mode = TRUE;

          probed_for_adpcm = TRUE;
          }

     /*
      * Below, the default mode for modems supporting ADPCM is ADPCM
      * 2 bits/sample, since some of these modems seems to have broken
      * GSM playback. The default mode for modems not supporting ADPCM is
      * GSM. The mode may be configured in voice.conf.
      */

     if (*compression == COMPRESSION_DEFAULT)
          {

          if (supports_adpcm)
               *compression = COMPRESSION_ADPCM_2;
          else
               *compression = COMPRESSION_GSM;

          }

     switch (*compression)
          {
          case COMPRESSION_GSM: /* This is the GSM mode. 8 bits / sample */
               *bits = 8;

               if ((supports_adpcm) && (in_adpcm_mode))
                    {
                    /* Set the mode to GSM */

                    if (voice_command("AT#VSM=128,8000", "OK") != VMA_FAIL)
                         {
                         in_adpcm_mode = FALSE;
                         return(OK);
                         }
                    else
                         return(VMA_FAIL);

                    }

               return(OK);

          case COMPRESSION_ADPCM_2:
               /* This is the adpcm 2 bits per sample mode */
          case COMPRESSION_ADPCM_3:
               /* This is the adpcm 3 bits per sample mode */
          case COMPRESSION_ADPCM_4:
               /* This is the adpcm 4 bits per sample mode */
               *bits = *compression;

               if ((supports_adpcm) && (!in_adpcm_mode))
                    {
                    /* Set the mode to ADPCM */

                    if (voice_command("AT#VSM=129,8000", "OK") == VMA_FAIL)
                         return(VMA_FAIL);

                    in_adpcm_mode = TRUE;
                    }

      /* Note: If the modem does not support ADPCM, the following command
         still gets issued. It has no effect in GSM mode, but is included
         for backwards compatibility with the old driver. */

               sprintf(buffer,"AT#VBS=%1d", *compression);

               if (voice_command(buffer, "OK") != VMA_FAIL)
                    return(OK);
               else
                    return(VMA_FAIL);

          }

     lprintf(L_WARN,"Illegal voice compression method (%d)", *compression);
     return(FAIL);
     }

static int USR_set_device(int device)
     {
     reset_watchdog();
     internal_speaker_used = FALSE;

     switch (device)
          {
          case NO_DEVICE:
               voice_command("AT#VLS=0H0","OK|VCON");
               return(OK);
          case DIALUP_LINE:
               voice_command("AT#VLS=0","OK|VCON");
               return(OK);
          case EXTERNAL_MICROPHONE:
               voice_command("AT#VLS=1","OK|VCON");
               return(OK);
          case INTERNAL_SPEAKER:
               internal_speaker_used = TRUE;
               voice_command("AT#VLS=4","OK|VCON");
               return(OK);
          case EXTERNAL_SPEAKER:
               voice_command("AT#VLS=2","OK|VCON");
               return(OK);
          case INTERNAL_MICROPHONE:
               voice_command("AT#VLS=3","OK|VCON");
               return(OK);
          default:
               lprintf(L_WARN,"USR: Unknown output device (%d)",device);
               return(FAIL);
          };

     }

static int USR_switch_to_data_fax (char *mode)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog();
     voice_modem->voice_mode_off();
     sprintf(buffer, "AT+FCLASS=%s", mode);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

static char USR_pick_phone_cmnd[] = "AT#VLS=4A";
static char USR_pick_phone_answr[] = "VCON";
static char USR_beep_cmnd[] = "AT#VTS=[%d,0,%d]";
#define     USR_beep_timeunit 100
static char USR_hardflow_cmnd[] = "AT";
static char USR_softflow_cmnd[] = "AT";
static char USR_start_play_cmnd[] = "ATE0#VTX";
static char USR_intr_play_cmnd[] = {DLE, CAN, 0x00};
static char USR_intr_play_answr[] = "OK|VCON";
static char USR_stop_play_answr[] = "OK|VCON";
static char USR_start_rec_cmnd[] = "AT#VRX";
static char USR_stop_rec_cmnd[] = {DLE, 0x00};
static char USR_stop_rec_answr[] = "OK|VCON";
static char USR_switch_mode_cmnd[] = "AT#CLS=";
static char USR_ask_mode_cmnd[] = "AT#CLS?";

voice_modem_struct US_Robotics =
     {
     "US Robotics",
     "US Robotics",
     (char *) USR_pick_phone_cmnd,
     (char *) USR_pick_phone_answr,
     (char *) USR_beep_cmnd,
     (char *) IS_101_beep_answr,
              USR_beep_timeunit,
     (char *) USR_hardflow_cmnd,
     (char *) IS_101_hardflow_answr,
     (char *) USR_softflow_cmnd,
     (char *) IS_101_softflow_answr,
     (char *) USR_start_play_cmnd,
     (char *) IS_101_start_play_answer,
     (char *) IS_101_reset_play_cmnd,
     (char *) USR_intr_play_cmnd,
     (char *) USR_intr_play_answr,
     (char *) IS_101_stop_play_cmnd,
     (char *) USR_stop_play_answr,
     (char *) USR_start_rec_cmnd,
     (char *) IS_101_start_rec_answr,
     (char *) USR_stop_rec_cmnd,
     (char *) USR_stop_rec_answr,
     (char *) USR_switch_mode_cmnd,
     (char *) IS_101_switch_mode_answr,
     (char *) USR_ask_mode_cmnd,
     (char *) IS_101_ask_mode_answr,
     (char *) IS_101_voice_mode_id,
     &IS_101_answer_phone,
     &USR_beep,
     &IS_101_dial,
     &IS_101_handle_dle,
     &USR_init,
     &IS_101_message_light_off,
     &IS_101_message_light_on,
     &IS_101_start_play_file,
     NULL,
     &USR_stop_play_file,
     &IS_101_play_file,
     &IS_101_record_file,
     &USR_set_compression,
     &USR_set_device,
     &IS_101_stop_dialing,
     &IS_101_stop_playing,
     &IS_101_stop_recording,
     &IS_101_stop_waiting,
     &USR_switch_to_data_fax,
     &IS_101_voice_mode_off,
     &IS_101_voice_mode_on,
     &IS_101_wait
     };
