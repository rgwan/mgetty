/*
 * Elsa.c
 *
 * This file contains the Elsa 28.8 TQV and TKR Tristar 28.8
 * specific hardware stuff.
 * it was written by Karlo Gross kg@orion.ddorf.rhein-ruhr.de
 * by using the old version from Stefan Froehlich and the
 * help from Marc Eberhard.
 * This is the 1. alpha release from 1996/11/14
 * You have set port_timeout in voice.conf to a minimum of 15
 * if you use 38400 Baud
 *
 */

#include "../include/voice.h"

char *libvoice_Elsa_c = "$Id: Elsa.c,v 1.1 1997/12/16 12:20:58 marc Exp $";

#define ELSA_BUFFER_SIZE 400

static int stop_recording,stop_playing;
static int elsa_buffer_size = ELSA_BUFFER_SIZE;
static char mode_save[VOICE_BUF_LEN] = "";

static int Elsa_answer_phone (void)
     {
     reset_watchdog(0);
     lprintf(L_JUNK, "Elsa.c: answer phone");

     if ((voice_command("ATA", "VCON|+VCON") & VMA_USER) != VMA_USER)
          {
          lprintf(L_ERROR, "Elsa.c: Error answer phone");
          return(VMA_ERROR);
          }

     return(VMA_OK);
     }

static int Elsa_init (void)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     voice_modem_state = INITIALIZING;
     lprintf(L_MESG, "initializing Elsa voice modem");
     voice_mode_on();

     sprintf(buffer, "AT#VSP=%1u", cvd.rec_silence_len.d.i);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set silence_len VSP");

     sprintf(buffer, "AT#VSD=0");

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set VSD=0");

     sprintf(buffer, "AT#VBS=4");                 /* for 38400 */

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set VBS=4");

     sprintf(buffer, "AT#BDR=16");                /* for 38400 */

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set BDR=16");

     sprintf(buffer, "AT#VTD=3F,3F,3F");

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set VTD=3F");

     sprintf(buffer, "AT#VSS=%1u", cvd.rec_silence_threshold.d.i * 3 / 100);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set silence threshold VSS");

     sprintf(buffer, "ATS30=60");       /* fuer 38400 */

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set S30");

     if ((cvd.do_hard_flow.d.i) && (voice_command("AT\\Q3", "OK") ==
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
     voice_mode_off();
     return(OK);
     }

static int Elsa_beep (int frequency, int length)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     sprintf(buffer, "AT#VTS=[%d,0,%d]" , frequency, length / 10);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

static int Elsa_set_compression (int *compression, int *speed, int *bits)
     {
     reset_watchdog(0);

     if (*compression == 0)
          *compression = 2;

     if (*speed == 0)
          *speed = 7200;

     if (*speed != 7200)
          {
          lprintf(L_WARN, "%s: Illeagal sample speed (%d)",
           voice_modem_name, *speed);
          return(FAIL);
          };

     if (*compression == 2)
          {
          voice_command("AT#VBS=2", "OK");
          elsa_buffer_size = ELSA_BUFFER_SIZE * 2 / 4;
          *bits = 2;
          return(OK);
          }
     if (*compression == 3)
          {
          voice_command("AT#VBS=3", "OK");
          elsa_buffer_size = ELSA_BUFFER_SIZE * 3 / 4;
          *bits = 3;
          return(OK);
          }
     if (*compression == 4)
          {
          voice_command("AT#VBS=4", "OK");
          elsa_buffer_size = ELSA_BUFFER_SIZE * 4 / 4;
          *bits = 4;
          return(OK);
          }

     lprintf(L_WARN, "%s: Illeagal voice compression method (%d)",
      voice_modem_name, *compression);
     return(FAIL);
     }

static int Elsa_set_device (int device)
     {
     reset_watchdog(0);

     switch (device)
          {
          case NO_DEVICE:
               lprintf(L_JUNK, "%s: _NO_DEV: (%d)", voice_modem_name, device);
               voice_command("AT#VLS=0", "OK");
               return(OK);
          case DIALUP_LINE:
               lprintf(L_JUNK, "%s: _DIALUP: (%d)", voice_modem_name, device);
               voice_command("AT#VLS=4", "OK");
               return(OK);
          case INTERNAL_MICROPHONE:
               lprintf(L_JUNK, "%s: _INT_MIC: (%d)", voice_modem_name, device);
               voice_command("AT#VLS=3", "VCON");
               return(OK);
          case INTERNAL_SPEAKER:
               lprintf(L_JUNK, "%s: _INT_SEAK: (%d)", voice_modem_name, device);
               voice_command("AT#VLS=2", "VCON");
               return(OK);
          }

     lprintf(L_WARN, "%s: Unknown device (%d)", voice_modem_name, device);
     return(FAIL);
     }

static int Elsa_play_file (int fd)
     {
     TIO tio_save;
     TIO tio;
     char input_buffer[ELSA_BUFFER_SIZE];
     char output_buffer[2 * ELSA_BUFFER_SIZE];
     int i;
     int bytes_in;
     int bytes_out;
     char buffer[VOICE_BUF_LEN];

     stop_playing = FALSE;
     voice_modem_state = PLAYING;
     voice_check_events();
     reset_watchdog(0);
     tio_get(voice_fd, &tio);
     tio_save = tio;
     tio_set_flow_control(voice_fd, &tio, FLOW_HARD | FLOW_XON_OUT);
     tio_set(voice_fd, &tio);

     if (cvd.transmit_gain.d.i == -1)
          cvd.transmit_gain.d.i = 50;

     sprintf(buffer, "AT#VGT=%d", cvd.transmit_gain.d.i * 127 / 100 +
      128);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set speaker volume");

     if (voice_command("AT#VTX", "CONNECT") != VMA_USER_1)
          return(FAIL);

     while (!stop_playing)
          {
          reset_watchdog(10);

          if ((bytes_in = read(fd, input_buffer, elsa_buffer_size)) <= 0)
               break;

          bytes_out = 0;

          for(i = 0; i < bytes_in; i++)
               {
               output_buffer[bytes_out] = input_buffer[i];

               if (output_buffer[bytes_out++] == DLE)
                    output_buffer[bytes_out++] = DLE;

               }

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

                    lprintf(L_MESG, "%s: <DLE> <%c>", voice_modem_name,
                     modem_byte);
                    voice_modem->handle_dle(modem_byte);
                    }
               else
                    lprintf(L_WARN, "%s: unexpected byte %c from voice modem",
                     program_name, modem_byte);

               }

          voice_check_events();
          }

     if (stop_playing)
          {
          sprintf(output_buffer, "%c%c", DLE, CAN);
          lprintf(L_MESG, "%s: <DLE> <CAN>", program_name);
          }
     else
          {
          sprintf(output_buffer, "%c%c", DLE, ETX);
          lprintf(L_MESG, "%s: <DLE> <ETX>", program_name);
          }

     if (voice_write_raw(output_buffer, strlen(output_buffer)) != OK)
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

static int Elsa_record_file (int fd)
     {
     TIO tio_save;
     TIO tio;
     time_t timeout;
     char input_buffer[ELSA_BUFFER_SIZE];
     char output_buffer[ELSA_BUFFER_SIZE];
     int i = 0;
     int bytes_in = 0;
     int bytes_out;
     int got_DLE_ETX = FALSE;
     int was_DLE = FALSE;

     lprintf(L_JUNK, "Elsa.c:record_file ");
     timeout = time(NULL) + cvd.rec_max_len.d.i;
     stop_recording = FALSE;
     voice_modem_state = RECORDING;
     voice_check_events();
     reset_watchdog(0);
     tio_get(voice_fd, &tio);
     tio_save = tio;

     if ((cvd.do_hard_flow.d.i) && (voice_command("AT\\Q3", "OK") !=
      VMA_USER_1))
          {
          lprintf(L_WARN, "Elsa.c: can't set flow control");
          return(FAIL);
          }

     if (voice_command("AT#VGR=255", "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set record volume");

     tio_set_flow_control(voice_fd, &tio, FLOW_HARD | FLOW_XON_IN);
     tio.c_cc[VMIN] = (elsa_buffer_size > 0xff) ? 0xff : elsa_buffer_size;
     tio.c_cc[VTIME] = 1;
     tio_set(voice_fd, &tio);

     if (voice_command("AT#VRX", "CONNECT") != VMA_USER_1)
          {
          lprintf(L_MESG, "can't set VRX");
          return(FAIL);
          }

     while (!got_DLE_ETX)
          {
          reset_watchdog(10);

          if (timeout < time(NULL))
               voice_stop_recording();

          if ((bytes_in = read(voice_fd, input_buffer, elsa_buffer_size)) <= 0)
               {
               lprintf(L_ERROR, "%s: could not read from voice modem",
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

                    }
               }

          write(fd, output_buffer, bytes_out);

          if (!got_DLE_ETX)
               lprintf(L_JUNK, "%s: <DATA %d bytes>", voice_modem_name,
                bytes_out);

          voice_check_events();
          }

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

          lprintf(L_JUNK, "%s: data left in buffer: ", voice_modem_name);

          for (j = i; j < bytes_in ; j++)
               lputc(L_JUNK, input_buffer[j]);

          if (voice_command("AT", "OK") != VMA_USER_1)
               return(FAIL);

          }

     if (voice_command("AT", "OK") != VMA_USER_1)
          return(FAIL);

     voice_modem_state = IDLE;
     voice_check_events();
     return(OK);
     }

static int Elsa_stop_playing (void)
     {
     stop_playing = TRUE;
     return(OK);
     }

static int Elsa_stop_recording (void)
     {
     char buffer[VOICE_BUF_LEN];

     stop_recording = TRUE;
     sprintf(buffer, "!");
     lprintf(L_JUNK, "%s: <!>", program_name);

     if (voice_write_raw(buffer, strlen(buffer)) != OK)
          return(FAIL);

     return(OK);
     }


static int Elsa_voice_mode_off (void)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     sprintf(buffer, "AT#CLS=%s", mode_save);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          return(FAIL);

     voice_command("AT", "OK");
     return(OK);
     }

static int Elsa_voice_mode_on (void)
     {
     reset_watchdog(0);

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

     if (voice_command("AT", "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

int Elsa_handle_dle (char data)
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
               return(queue_event(create_event(HANDSET_ON_HOOK)));

          /*
           * Local handset goes off hook
           */

            case 't':
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

int Elsa_switch_to_data_fax (char *mode)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     sprintf(buffer, "AT#CLS=%s", mode);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

voice_modem_struct Elsa =
    {
    "Elsa MicroLink",
    "Elsa",
    &Elsa_answer_phone,
    &Elsa_beep,
    &IS_101_dial,
    &Elsa_handle_dle,
    &Elsa_init,
    &IS_101_message_light_off,
    &IS_101_message_light_on,
    &Elsa_play_file,
    &Elsa_record_file,
    &Elsa_set_compression,
    &Elsa_set_device,
    &IS_101_stop_dialing,
    &Elsa_stop_playing,
    &Elsa_stop_recording,
    &IS_101_stop_waiting,
    &Elsa_switch_to_data_fax,
    &Elsa_voice_mode_off,
    &Elsa_voice_mode_on,
    &IS_101_wait
    };
