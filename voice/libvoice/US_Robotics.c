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
 */

#include "../include/voice.h"

char *libvoice_US_Robotics_c = "$Id: US_Robotics.c,v 1.1 1997/12/16 12:21:04 marc Exp $";

#define BEEPLEN_DIVISOR 10
#define USR_BUFFER_SIZE (8000/2/10)
#define GSM_BUFFER_SIZE 400

#define COMPRESSION_DEFAULT 0
#define COMPRESSION_GSM 1
#define COMPRESSION_ADPCM_2 2
#define COMPRESSION_ADPCM_3 3
#define COMPRESSION_ADPCM_4 4

static char stop_recording_char = 0x10;
static volatile int buffer_size = USR_BUFFER_SIZE;

/*
 * Here we save the current mode of operation of the voice modem when
 * switching to voice mode, so that we can restore it afterwards.
 */

static char mode_save[VOICE_BUF_LEN] = "";

/*
 * Internal status variables for aborting some voice modem actions.
 */

static volatile int stop_playing;
static volatile int stop_recording;
static int supports_adpcm = FALSE;
static int probed_for_adpcm = FALSE;
static int in_adpcm_mode = FALSE;
static volatile int in_voice_mode = FALSE;
static int internal_speaker_used = FALSE;

static int USR_init (void)
     {
     reset_watchdog(0);
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

     voice_modem_state = IDLE;
     return(OK);
     }

static int USR_answer_phone (void)
     {
     reset_watchdog(0);

     if (voice_command("AT#VLS=4A", "VCON") != VMA_USER_1)
          return(VMA_ERROR);

     return(VMA_OK);
     }

static int USR_beep (int frequency, int length)
     {
     char buffer[VOICE_BUF_LEN];

     sleep(1);
     reset_watchdog(0);
     sprintf(buffer, "AT#VTS=[%d,0,%d]", frequency, length / BEEPLEN_DIVISOR);

     if (voice_command("AT","OK") != VMA_USER_1)
          return(FAIL);

     sleep(1);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          return(FAIL);

     if (voice_command("AT","OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

static int USR_handle_dle (char data)
     {

     switch (data)
          {

          /*
           * shielded <DLE> code
           */

          case DLE:
               lprintf(L_WARN, "%s: Shielded <DLE> received", program_name);
               return(OK);

          /*
           * <ETX> code
           */

          case ETX:
               lprintf(L_WARN, "%s: <DLE> <ETX> received", program_name);
               return(OK);

          /*
           * DTMF tone detected
           */

          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
          case '*':
          case '#':
          case 'A':
          case 'B':
          case 'C':
          case 'D':
               {
               event_type *event;
               event_data dtmf;

               event = create_event(RECEIVED_DTMF);
               dtmf.c = data;
               event->data = dtmf;
               queue_event(event);
               return(OK);
               }

          /*
           * Busy tone detected
           */

          case 'b':
               return(queue_event(create_event(BUSY_TONE)));

          /*
           * Fax calling tone detected
           */

          case 'c':
               return(queue_event(create_event(FAX_CALLING_TONE)));

          /*
           * Dial tone detected
           */

          case 'd':
               return(queue_event(create_event(DIAL_TONE)));

          /*
           * Data calling tone detected
           */

          case 'e':
               return(queue_event(create_event(DATA_CALLING_TONE)));


          case 'q':
               return(queue_event(create_event(SILENCE_DETECTED)));

          /*
           * Local handset goes on hook
        */

          case 'h':
               return(queue_event(create_event(HANDSET_ON_HOOK)));

          /*
           * Local handset goes off hook
           */

          case 't':
               return(queue_event(create_event(HANDSET_OFF_HOOK)));

          /*
           * XON received
           */

          case 'Q':
               lprintf(L_WARN, "%s: XON received", program_name);
               return(OK);

          /*
           * Ringback detected
           */

          case 'r':
               return(queue_event(create_event(RINGBACK_DETECTED)));

          /*
           * Modem could not detect voice energy on the line
           */

          case 's':
               return(queue_event(create_event(NO_VOICE_ENERGY)));

          /*
           * XOFF received
           */

          case 'S':
               lprintf(L_WARN, "%s: XOFF received", program_name);
               return(OK);

          /*
           * Buffer underrun
           */

          case 'u':
               lprintf(L_WARN, "%s: Buffer underrun", program_name);
               return(OK);

          /*
           * Voice detected
           */

          case 'v':
          case 'V':
               return(queue_event(create_event(VOICE_DETECTED)));

          /*
           * Call waiting, beep interrupt
           */

          case 'w':
               return(queue_event(create_event(CALL_WAITING)));

          /*
           * Lost data detected event
           */

          case 'Y':
               lprintf(L_WARN, "%s: Lost data detected event", program_name);
               return(OK);
          };

     /*
      * Unknown DLE code
      */

     lprintf(L_WARN, "%s: Unknown code <DLE> <%c>", program_name, data);
     return(FAIL);
     }

static int USR_play_file (int fd)
     {
     TIO tio_save;
     TIO tio;
     char input_buffer[USR_BUFFER_SIZE];
     char output_buffer[2 * USR_BUFFER_SIZE];
     int i;
     int bytes_in;
     int bytes_out;

     reset_watchdog(0);
     voice_check_events();
     stop_playing = FALSE;
     voice_modem_state = PLAYING;
     tio_get(voice_fd, &tio);
     tio_save = tio;

     if (cvd.do_hard_flow.d.i)
          tio_set_flow_control(voice_fd, &tio, FLOW_HARD);
     else
          tio_set_flow_control(voice_fd, &tio, FLOW_HARD|FLOW_XON_OUT);

     tio_set(voice_fd, &tio);

     if (voice_command("ATE0#VTX", "CONNECT") != VMA_USER_1)
          return(FAIL);

     while (!stop_playing)
          {
          reset_watchdog(0);

          if ((bytes_in = read(fd, input_buffer, buffer_size)) <= 0)
               break;

          bytes_out = 0;

          for(i = 0; i < bytes_in; i++)
               {
               output_buffer[bytes_out] = input_buffer[i];

               if (output_buffer[bytes_out++] == DLE)
                    output_buffer[bytes_out++] = DLE;

               };

          lprintf(L_JUNK, "%s: <DATA %d bytes>", program_name, bytes_out);

          if (voice_write_raw(output_buffer, bytes_out) != OK)
               return(FAIL);

          while (check_for_input(voice_fd))
               {
               char modem_byte;

               if ((modem_byte = voice_read_char()) == FAIL)
                    return(FAIL);

               if (modem_byte == DLE)
                    {

                    if ((modem_byte = voice_read_char()) == FAIL)
                         return(FAIL);

                    lprintf(L_JUNK, "%s: <DLE> <%c>", voice_modem_name,
                     modem_byte);


                    if (!((modem_byte == 'd') && internal_speaker_used))
                         voice_modem->handle_dle(modem_byte);

                    }
               else
                    lprintf(L_WARN, "%s: unexpected byte %c from voice modem",
                     program_name, modem_byte);
               };

          voice_check_events();
          };

     if (stop_playing)
          {
          sprintf(output_buffer, "%c%c", DLE, CAN);
          lprintf(L_JUNK, "%s: <DLE> <CAN>", program_name);
          }
     else
          {
          sprintf(output_buffer, "%c%c", DLE, ETX);
          lprintf(L_JUNK, "%s: <DLE> <ETX>", program_name);
          };

     if (voice_write_raw(output_buffer, strlen(output_buffer)) != OK)
          return(FAIL);

     if ((voice_command("", "OK|VCON") & VMA_USER) != VMA_USER)
          return(FAIL);

     if (voice_write_raw("ATE1\r",5) != OK)
          return(FAIL);

     if ((voice_command("", "OK|VCON") & VMA_USER) != VMA_USER)
          return(FAIL);

     tio_set(voice_fd, &tio_save);

     if (voice_command("AT", "OK") != VMA_USER_1)
          return(FAIL);

     voice_modem_state = IDLE;
     voice_check_events();

     if (stop_playing)
          return(INTERRUPTED);

     return(OK);
     }

static int USR_record_file (int fd)
     {
     TIO tio_save;
     TIO tio;
     time_t timeout;
     char input_buffer[USR_BUFFER_SIZE];
     char output_buffer[USR_BUFFER_SIZE];
     int i = 0;
     int bytes_in = 0;
     int bytes_out;
     int got_DLE_ETX = FALSE;
     int was_DLE = FALSE;
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     voice_check_events();
     timeout = time(NULL) + cvd.rec_max_len.d.i;
     stop_recording = FALSE;
     voice_modem_state = RECORDING;
     tio_get(voice_fd, &tio);
     tio_save = tio;

     if (cvd.do_hard_flow.d.i)
          tio_set_flow_control(voice_fd, &tio, FLOW_HARD);
     else
          tio_set_flow_control(voice_fd, &tio, FLOW_HARD | FLOW_XON_IN);

     tio.c_cc[VMIN] = (buffer_size > 0xff) ? 0xff : buffer_size;
     tio.c_cc[VTIME] = 1;
     tio_set(voice_fd, &tio);

     /*
      * Set silence threshold and length. Must be in voice mode to do this.
      */

     sprintf(buffer, "AT#VSD=1#VSS=%d#VSP=%d",
      cvd.rec_silence_threshold.d.i * 3 / 100,
      cvd.rec_silence_len.d.i);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN,"setting recording preferences didn't work");

     if (voice_command("AT#VRX", "CONNECT") != VMA_USER_1)
          return(FAIL);

     while (!got_DLE_ETX)
          {
          reset_watchdog(10);

          if (timeout < time(NULL))
               voice_stop_recording();

          if ((bytes_in = read(voice_fd, input_buffer, buffer_size)) <= 0)
               {
               lprintf(L_ERROR, "%s: could not read byte from voice modem",
                program_name);
               return(FAIL);
               };

          bytes_out = 0;

          for (i = 0; (i < bytes_in) && !got_DLE_ETX; i++)
               {
               if (was_DLE)
                    {
                    was_DLE = FALSE;

                    switch (input_buffer[i])
                         {
                         case DLE:
                              output_buffer[bytes_out++] = DLE;
                              break;
                         case ETX:
                              got_DLE_ETX = TRUE;
                              lprintf(L_JUNK, "%s: <DATA %d bytes>",
                               voice_modem_name, bytes_out);
                              lprintf(L_JUNK, "%s: <DLE> <ETX>",
                               voice_modem_name);
                              break;
                         default:
                              lprintf(L_JUNK, "%s: <DLE> <%c>",
                               voice_modem_name, input_buffer[i]);
                              voice_modem->handle_dle(input_buffer[i]);
                         };

                    }
               else
                    {

                    if (input_buffer[i] == DLE)
                         was_DLE = TRUE;
                    else
                         output_buffer[bytes_out++] = input_buffer[i];

                    };

               };

          write(fd, output_buffer, bytes_out);

          if (!got_DLE_ETX)
               lprintf(L_JUNK, "%s: <DATA %d bytes>", voice_modem_name,
                bytes_out);

          voice_check_events();
          };

     tio_set(voice_fd, &tio_save);

     if ((voice_analyze(&input_buffer[i], "\r\nOK|\r\nVCON") & VMA_USER) ==
      VMA_USER)
          lprintf(L_JUNK, "%s: OK|VCON", voice_modem_name);
     else
          {

          /*
           * Fixme: The datas in the buffer should be checked
           * for DLE shielded codes
           */

          int j;

          lprintf(L_JUNK, "%s: data left in buffer: ",
           voice_modem_name);

          for (j = i; j < bytes_in ; j++)
               lputc(L_JUNK, input_buffer[j]);

          };

     if (voice_command("AT", "OK") != VMA_USER_1)
          return(FAIL);

     voice_modem_state = IDLE;
     voice_check_events();
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
           (voice_command("AT#VSM=129,8000","OK") == VMA_USER_1)))
               lprintf(L_ERROR,"%s: Ignore the above error!",program_name);
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

               buffer_size = GSM_BUFFER_SIZE;
               return(OK);

          case COMPRESSION_ADPCM_2:
               /* This is the adpcm 2 bits per sample mode */
          case COMPRESSION_ADPCM_3:
               /* This is the adpcm 3 bits per sample mode */
          case COMPRESSION_ADPCM_4:
               /* This is the adpcm 4 bits per sample mode */


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
                    {
                    buffer_size = USR_BUFFER_SIZE * (*compression) / 4;
                    return(OK);
                    }
               else
                    return(VMA_FAIL);

          }

     lprintf(L_WARN,"Illegal voice compression method (%d)", *compression);
     return(FAIL);
     }

static int USR_set_device(int device)
     {
     reset_watchdog(0);
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

static int USR_stop_recording (void)
     {
     stop_recording = TRUE;

     if (voice_write_raw(&stop_recording_char, 1) != OK)
          return(FAIL);

     if (voice_write_raw(&stop_recording_char, 1) != OK)
          return(FAIL);

     return(OK);
     }

static int USR_voice_mode_off (void)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);

     if (in_voice_mode)
          {
          sprintf(buffer, "AT#CLS=%s", mode_save);

          if (voice_command(buffer, "OK") != VMA_USER_1)
               return(FAIL);

          in_voice_mode = FALSE;
          }

     return(OK);
     }

static int USR_voice_mode_on (void)
     {
     reset_watchdog(0);

     if (!in_voice_mode)
          {

          if (voice_command("AT#CLS?", "") != OK)
               return(FAIL);

          do
               {

               if (voice_read(mode_save) != OK)
                    return(FAIL);

               }
          while (strlen(mode_save) == 0);

          if (voice_command("", "OK") != VMA_USER_1)
               return(FAIL);

          if (voice_command("AT#CLS=8", "OK") != VMA_USER_1)
               return(FAIL);

          in_voice_mode = TRUE;
          }

     return(OK);
     }

static int USR_stop_playing (void)
     {
     stop_playing = TRUE;
     return(OK);
     }

static int USR_switch_to_data_fax (char *mode)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     USR_voice_mode_off();
     sprintf(buffer, "AT+FCLASS=%s", mode);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }


voice_modem_struct US_Robotics =
     {
     "US Robotics",
     "US Robotics",
     &USR_answer_phone,
     &USR_beep,
     &IS_101_dial,
     &USR_handle_dle,
     &USR_init,
     &IS_101_message_light_off,
     &IS_101_message_light_on,
     &USR_play_file,
     &USR_record_file,
     &USR_set_compression,
     &USR_set_device,
     &IS_101_stop_dialing,
     &USR_stop_playing,
     &USR_stop_recording,
     &IS_101_stop_waiting,
     &USR_switch_to_data_fax,
     &USR_voice_mode_off,
     &USR_voice_mode_on,
     &IS_101_wait
     };
