/*
 * Cirrus_Logic.c
 *
 * This file contains specific hardware stuff for Cirrus Logic based
 * voice modems. As I have NO manuals whatsovere, I sat down and reverse
 * engineered this file by capturing the o/p from my supplied Windoze
 * program (which shall remain unnamed lest I get sued :-) It works quite
 * well for me so far.
 *
 * This was written for my Cirrus Logic PCMCIA Modem which is packaged as
 * a Dynalink V1414VC modem. My ROM id shows
 *   ATI0 = 1.04
 *   ATI1 = HD94-HM71-HEC17
 *   ATI3 = CL-MD1414AT/EC
 *   ATI4 = 31
 *
 * It should work with most CL modems. Please let me know if you have
 * any problems with it. - Mitch <Mitch.DSouza@NetComm.IE>
 *
 * Modifications for the new interface were made by Hitesh K. Soneji
 * email: hitesh.soneji@industry.net
 * www:   http://www.geocities.com/SiliconValley/4548
 *
 * Brian King <Brian.Knight@fal.ca> kindly sent me a Cirrus Logic Doc with
 * all the commands for CL modems and I have made some slight changes. The
 * rec_compression parameter in voice.conf can now be 1 (default), 3 or 4
 * as per the spec. The silence threshold and time was wrongly set. Fixed now.
 * - Mitch DSouza <Mitch.DSouza@Uk.Sun.COM>
 */

#include "../include/voice.h"

char *Cirrus_Logic_c = "$Id: Cirrus_Logic.c,v 1.1 1997/12/16 12:20:57 marc Exp $";

/*
 * Here we save the current mode of operation of the voice modem when
 * switching to voice mode, so that we can restore it afterwards.
 */

static char mode_save[VOICE_BUF_LEN] = "";
static char cirrus_logic_ans[VOICE_BUF_LEN] = "";

typedef struct
     {
     int min;
     int max;
     } range;

/*
 * Internal status variables for aborting some voice modem actions.
 */

static int stop_dialing;
static int stop_playing;
static int stop_recording;
static int stop_waiting;
static int current_device = -1;

/*
 * Get Range
 * This Function is only for Cirrus Logic
 */

static void get_range(char *buf, range *r)
     {

     if ((!buf) || (!r))
          return;

     sscanf(buf, "(%d-%d)", &r->min, &r->max);
     }

/*
 * The Cirrus Logic modems samples at only 9600 samples per second with a
 * maximum of 5 bits per sample. We need to buffer voice for 0.1 second, thus
 * we need a max of 9600 * 0.1 * 5/8 = 600 bytes.
 */

#define CL_BUFFER_SIZE 600
static int buffer_size = CL_BUFFER_SIZE;

int Cirrus_Logic_answer_phone (void)
     {
     reset_watchdog(0);
     lprintf(L_MESG, "Answering Call");

     if (voice_command("AT#VLN=1", "OK") != VMA_USER_1)
          return(VMA_ERROR);

     if (voice_command("AT#VIP=1", "OK") != VMA_USER_1)
          return(VMA_ERROR);

     return(VMA_OK);
     }

int Cirrus_Logic_beep (int frequency, int length)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     sprintf(buffer, "AT#VBP");
     lprintf(L_MESG, "Checking Beep");

     if (voice_command(buffer, "") != OK)
          return(FAIL);

     while (!check_for_input(voice_fd))
          {
          reset_watchdog(100);
          delay(10);
          };

     if (voice_command("", "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

int Cirrus_Logic_dial (char *number)
     {
     char buffer[VOICE_BUF_LEN];
     int result = FAIL;

     lprintf(L_MESG, "Dialing");
     reset_watchdog(0);
     voice_modem_state = DIALING;
     stop_dialing = FALSE;
     sprintf(buffer, "ATD%s", (char*) number);

     if (voice_command(buffer, "") != OK)
          return(FAIL);

     while (!stop_dialing)
          {
          reset_watchdog(0);

          if (voice_read(buffer) != OK)
               return(FAIL);

          result = voice_analyze(buffer, NULL);

          switch (result)
               {
               case VMA_BUSY:
               case VMA_FAIL:
               case VMA_ERROR:
               case VMA_NO_ANSWER:
               case VMA_NO_CARRIER:
               case VMA_NO_DIAL_TONE:
                    stop_dialing = TRUE;
                    result = FAIL;
                    break;
               case VMA_VCON:
                    stop_dialing = TRUE;
                    result = OK;
                    break;
               };

          };

     voice_modem_state = IDLE;
     return(result);
     }

int Cirrus_Logic_handle_dle (char data)
     {
     lprintf(L_MESG, "Handling DLE");
     switch (data)
          {

          /*
           * shielded <DLE> code
           */

          case DLE:
               lprintf(L_WARN, "%s: Shielded <DLE> received", program_name);
               return(OK);

          /*
           * shielded <DLE> <DLE> code
           */

          case SUB:
               lprintf(L_WARN, "%s: Shielded <DLE> <DLE> received",
                program_name);
               return(OK);

          /*
           * <ETX> code
           */

          case ETX:
               lprintf(L_WARN, "%s: <DLE> <ETX> received", program_name);
               return(OK);

          /*
           * Bong tone detected
           */

          case '$':
               return(queue_event(create_event(BONG_TONE)));

          /*
           * Start of DTMF shielding
           */

          case '/':
               lprintf(L_WARN, "%s: Start of DTMF shielding received",
                program_name);
               return(OK);

          /*
           * End of DTMF shielding
           */

          case '~':
               lprintf(L_WARN, "%s: End of DTMF shielding received",
                program_name);
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
           * Data or fax answer detected
           */

          case 'a':
               return(queue_event(create_event(DATA_OR_FAX_DETECTED)));

          /*
           * Busy tone detected
           */

          case 'b':
          case 'K':
               return(queue_event(create_event(BUSY_TONE)));

          /*
           * Fax calling tone detected
           */

          case 'c':
          case 'm':
               return(queue_event(create_event(FAX_CALLING_TONE)));

          /*
           * Dial tone detected
           */

          case 'd':
          case 'i':
               return(queue_event(create_event(DIAL_TONE)));

          /*
           * Data calling tone detected
           */

          case 'e':
          case 'f':
               return(queue_event(create_event(DATA_CALLING_TONE)));

          /*
           * Invalid voice format detected
           */

          case 'E':
               lprintf(L_WARN, "%s: Invalid voice format detected",
                program_name);
               return(OK);

          /*
           * Local handset goes on hook
           */

          case 'h':
          case 'p':
               return(queue_event(create_event(HANDSET_ON_HOOK)));

          /*
           * Local handset goes off hook
           */

          case 'H':
          case 'P':
               return(queue_event(create_event(HANDSET_OFF_HOOK)));

          /*
           * Loop current break
           */

          case 'I':
               return(queue_event(create_event(LOOP_BREAK)));

          /*
           * SIT tone detected
           */

          case 'J':
               return(queue_event(create_event(SIT_TONE)));

          /*
           * Loop current polarity reversal
           */

          case 'L':
               return(queue_event(create_event(LOOP_POLARITY_CHANGE)));

          /*
           * Buffer overrun
           */

          case 'o':
               lprintf(L_WARN, "%s: Buffer overrun", program_name);
               return(OK);

          /*
           * Modem detected silence
           */

          case 'q':
               return(queue_event(create_event(SILENCE_DETECTED)));

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
           * Ring detected
           */

          case 'R':
               return(queue_event(create_event(RING_DETECTED)));

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
           * TDD detected
           */

          case 't':
               return(queue_event(create_event(TDD_DETECTED)));

          /*
           * Timing mark will be ignored
           */

          case 'T':
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

int Cirrus_Logic_voice_mode_off (void)
     {
     char buffer[VOICE_BUF_LEN];

     lprintf(L_MESG, "Setting voice mode off");
     reset_watchdog(0);
     sprintf(buffer, "AT#VCL=0");

     if (voice_command(buffer, "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

int Cirrus_Logic_voice_mode_on (void)
     {
     reset_watchdog(0);

     lprintf(L_MESG, "Setting voice mode on");
     if (voice_command("AT#VCL?", "") != OK)
          return(FAIL);

     do
          {

          if (voice_read(mode_save) != OK)
               return(FAIL);

          }
     while (strlen(mode_save) == 0);

     if (voice_command("", "OK") != VMA_USER_1)
          return(FAIL);

     if (voice_command("AT#VCL=1", "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

int Cirrus_Logic_init (void)
     {
     char buffer[VOICE_BUF_LEN];
     range play_range = {0, 0};
     range rec_range = {0, 0};
     range rec_silence_threshold = {0, 0};

     reset_watchdog(0);
     lprintf(L_MESG, "Initializing Cirrus Logic voice modem");
     voice_modem_state = INITIALIZING;
     Cirrus_Logic_voice_mode_on();

     /* Get the record volume range available from modem */
     voice_command("AT#VRL=?", "");
     voice_read(cirrus_logic_ans);
     voice_flush(1);
     get_range(cirrus_logic_ans, &rec_range);

     if (cvd.receive_gain.d.i == -1)
          cvd.receive_gain.d.i = (rec_range.max + rec_range.min) / 2;

     sprintf(buffer, "AT#VRL=%.0f", rec_range.min + ((rec_range.max -
      rec_range.min) * cvd.receive_gain.d.i / 100.0));

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set recording volume");

     /* Get the play volume range available from modem */
     voice_command("AT#VPL=?", "");
     voice_read(cirrus_logic_ans);
     voice_flush(1);
     get_range(cirrus_logic_ans, &play_range);

     if (cvd.transmit_gain.d.i == -1)
          cvd.transmit_gain.d.i = (play_range.max + play_range.min) / 2;

     sprintf(buffer, "AT#VPL=%.0f", play_range.min + ((play_range.max -
      play_range.min) * cvd.transmit_gain.d.i / 100.0));

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set play volume");

     if ((cvd.rec_silence_threshold.d.i > 100) ||
        (cvd.rec_silence_threshold.d.i < 0))
          {
          lprintf(L_ERROR, "Invalid threshold value.");
          return(ERROR);
          }

     /* Get the silence threshold range from modem */
     voice_command("AT#VSL=?", "");
     voice_read(cirrus_logic_ans);
     voice_flush(1);
     get_range(cirrus_logic_ans, &rec_silence_threshold);

     sprintf(buffer, "AT#VSL=%.0f", rec_silence_threshold.min +
      ((rec_silence_threshold.max - rec_silence_threshold.min) *
      cvd.rec_silence_threshold.d.i / 100.0));

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set silence threshold");

     sprintf(buffer, "AT#VSQT=%1u", cvd.rec_silence_len.d.i);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set silence period record mode");

     sprintf(buffer, "AT#VSST=%1u", cvd.rec_silence_len.d.i);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set silence period record and command mode");

     Cirrus_Logic_voice_mode_off();

     if (voice_command("AT\\Q3", "OK") == VMA_USER_1)
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

int Cirrus_Logic_play_file (int fd)
     {
     TIO tio_save;
     TIO tio;
     char input_buffer[CL_BUFFER_SIZE];
     char output_buffer[2 * CL_BUFFER_SIZE];
     int i;
     int bytes_in;
     int bytes_out;
     int bytes_written;
     stop_playing = FALSE;
     voice_modem_state = PLAYING;
     reset_watchdog(0);

     voice_check_events();
     tio_get(voice_fd, &tio);
     tio_save = tio;
     tio_set(voice_fd, &tio);

     lprintf(L_MESG, "Playing");

     if (voice_command("AT#VPY", "CONNECT") != VMA_USER_1)
          return(FAIL);

     while (!stop_playing)
          {
          reset_watchdog(10);

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
          errno = 0;
          bytes_written = 0;

          while (((bytes_written += write(voice_fd,
           &output_buffer[bytes_written], bytes_out - bytes_written)) !=
           bytes_out) && (errno == 0))
               ;

          if (bytes_written != bytes_out)
               {
               lprintf(L_ERROR,
                "%s: could only write %d bytes of %d bytes (errno 0x%x)",
                  program_name, bytes_written, bytes_out, errno);
               }

          while (check_for_input(voice_fd))
               {
               char modem_byte;

               if (read(voice_fd, &modem_byte, 1) != 1)
                    lprintf(L_ERROR,
                     "%s: could not read byte from voice modem",
                     program_name);
               else
                    return(FAIL);

               }

          voice_check_events();
          };

     if (stop_playing)
          {
          sprintf(output_buffer, "%cA%c%c", DLE, DLE, ETX);
          lprintf(L_JUNK, "%s: <DLE> <DC4>", program_name);
          }
     else
          {
          sprintf(output_buffer, "%c%c", DLE, ETX);
          lprintf(L_JUNK, "%s: <DLE> <ETX>", program_name);
          };

     write(voice_fd, output_buffer, strlen(output_buffer));
     tio_set(voice_fd, &tio_save);
     voice_command("", "OK|VCON");
     voice_modem_state = IDLE;
     voice_check_events();

     if (stop_playing)
          return(INTERRUPTED);

     return(OK);
     }

int Cirrus_Logic_record_file (int fd)
     {
     TIO tio_save;
     TIO tio;
     time_t timeout;
     char input_buffer[CL_BUFFER_SIZE];
     char output_buffer[CL_BUFFER_SIZE];
     int i = 0;
     int bytes_in = 0;
     int bytes_out;
     int got_DLE_ETX = FALSE;
     int was_DLE = FALSE;

     timeout = time(NULL) + cvd.rec_max_len.d.i;
     stop_recording = FALSE;
     voice_modem_state = RECORDING;
     voice_check_events();
     reset_watchdog(0);
     tio_get(voice_fd, &tio);
     tio_save = tio;

     lprintf(L_MESG, "Recording");
     tio.c_cc[VMIN] = (buffer_size > 0xff) ? 0xff : buffer_size;
     tio.c_cc[VTIME] = 1;
     tio_set(voice_fd, &tio);

     if (voice_command("AT#VRD", "CONNECT") != VMA_USER_1)
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
                         case SUB:
                              output_buffer[bytes_out++] = DLE;
                              output_buffer[bytes_out++] = DLE;
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

          if (voice_command("AT", "OK") != VMA_USER_1)
               return(FAIL);

          };

     if (voice_command("AT", "OK") != VMA_USER_1)
          return(FAIL);

     voice_modem_state = IDLE;
     voice_check_events();
     return(OK);
     }

int Cirrus_Logic_set_buffer_size (int size)
     {
     lprintf(L_MESG, "Setting buffer size");

     if (size > CL_BUFFER_SIZE)
          return(FAIL);

     buffer_size = size;
     return(OK);
     }

/*
 * As far as my specs go the CL modem only supports a sample rate of 9600 at
 * 5, 3 or 4 bits.
 */

static int Cirrus_Logic_set_compression (int *compression, int *speed,
 int *bits)
     {
     lprintf(L_MESG, "Setting compression");
     reset_watchdog(0);

     if (*compression == 0)
          *compression = 1;

     if (*speed == 0)
          *speed = 9600;

     if (*speed != 9600)
          {
          lprintf(L_WARN, "%s: Illegal sample speed (%d)",
           voice_modem_name, *speed);
          return(FAIL);
          }

     voice_command("AT#VSR=9600", "OK");

     switch (*compression)
          {
          case 3:
               voice_command("AT#VSM=AD3", "OK");
               buffer_size = CL_BUFFER_SIZE * 3 / 5;
               *bits = 3;
               return(OK);
          case 4:
               voice_command("AT#VSM=AD4", "OK");
               buffer_size = CL_BUFFER_SIZE * 4 / 5;
               *bits = 4;
               return(OK);
          default:
               voice_command("AT#VSM=CL1", "OK");
               buffer_size = CL_BUFFER_SIZE * 5 / 5;
               *bits = 5;
               return(OK);
          }

     return(OK);
     }

int Cirrus_Logic_set_device (int device)
     {
     reset_watchdog(0);
     lprintf(L_MESG, "Setting device");
     current_device = device;

     switch (device)
          {
          case NO_DEVICE:
               voice_command("AT#VLN=0", "OK");
               return(OK);
          case DIALUP_LINE:
               voice_command("AT#VLN=1", "OK");
               return(OK);
          case EXTERNAL_MICROPHONE:
               voice_command("AT#VLN=32", "OK");
               return(OK);
          case INTERNAL_SPEAKER:
               voice_command("AT#VLN=16", "OK");
               return(OK);
          case LOCAL_HANDSET:
               voice_command("AT#VLN=2","OK");
               return(OK);
          }

     lprintf(L_WARN, "%s: Unknown output device (%d)", voice_modem_name,
      device);
     return(FAIL);
     }

int Cirrus_Logic_stop_dialing (void)
     {
     lprintf(L_MESG, "Stopping dial");
     stop_dialing = TRUE;
     return(OK);
     }

int Cirrus_Logic_stop_playing (void)
     {
     lprintf(L_MESG, "Stopping play");
     stop_playing = TRUE;
     return(OK);
     }

int Cirrus_Logic_stop_recording (void)
     {
     char buffer[VOICE_BUF_LEN];

     lprintf(L_MESG, "Stopping record");
     stop_recording = TRUE;
     sprintf(buffer, "\r");
     lprintf(L_JUNK, "%s: <DLE> <!>", program_name);

     if (voice_write_raw(buffer, strlen(buffer)) != OK)
          return(FAIL);

     return(OK);
     }

int Cirrus_Logic_stop_waiting (void)
     {
     stop_waiting = TRUE;
     lprintf(L_MESG, "Stopping wait");
     return(OK);
     }

int Cirrus_Logic_switch_to_data_fax (char *mode)
     {
     char buffer[VOICE_BUF_LEN];

     lprintf(L_MESG, "Switching to data/fax");
     reset_watchdog(0);
     sprintf(buffer, "AT+FCLASS=%s", mode);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

int Cirrus_Logic_wait (int wait_timeout)
     {
     time_t timeout;

     lprintf(L_MESG, "Waiting");
     stop_waiting = FALSE;
     voice_modem_state = WAITING;
     timeout = time(NULL) + wait_timeout;
     voice_check_events();
     reset_watchdog(0);

     while ((!stop_waiting) && (timeout >= time(NULL)))
          {
          reset_watchdog(10);

          while (check_for_input(voice_fd))
               {
               int char_read;

               if ((char_read = voice_read_char()) == FAIL)
                    return(FAIL);

               if (char_read == DLE)
                    {

                    if ((char_read = voice_read_char()) == FAIL)
                         return(FAIL);

                    lprintf(L_JUNK, "%s: <DLE> <%c>", voice_modem_name,
                     char_read);
                    voice_modem->handle_dle(char_read);
                    }
               else
                    lprintf(L_WARN,
                     "%s: unexpected byte <%c> from voice modem",
                     program_name, char_read);

               };

          voice_check_events();
          delay(100);
          };

     voice_modem_state = IDLE;
     voice_check_events();
     return(OK);
     }

voice_modem_struct Cirrus_Logic =
     {
     "Cirrus Logic",
     "Cirrus Logic",
     &Cirrus_Logic_answer_phone,
     &Cirrus_Logic_beep,
     &Cirrus_Logic_dial,
     &Cirrus_Logic_handle_dle,
     &Cirrus_Logic_init,
     &IS_101_message_light_off,
     &IS_101_message_light_on,
     &Cirrus_Logic_play_file,
     &Cirrus_Logic_record_file,
     &Cirrus_Logic_set_compression,
     &Cirrus_Logic_set_device,
     &Cirrus_Logic_stop_dialing,
     &Cirrus_Logic_stop_playing,
     &Cirrus_Logic_stop_recording,
     &Cirrus_Logic_stop_waiting,
     &Cirrus_Logic_switch_to_data_fax,
     &Cirrus_Logic_voice_mode_off,
     &Cirrus_Logic_voice_mode_on,
     &Cirrus_Logic_wait
     };
