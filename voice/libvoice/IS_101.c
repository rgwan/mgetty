/*
 * IS_101.c
 *
 * This file contains generic hardware driver functions for modems
 * that follow the IS-101 interim standard for voice modems.
 *
 */

#include "../include/voice.h"

char *libvoice_IS_101_c = "$Id: IS_101.c,v 1.1 1997/12/16 12:21:00 marc Exp $";

/*
 * Here we save the current mode of operation of the voice modem when
 * switching to voice mode, so that we can restore it afterwards.
 */

static char mode_save[VOICE_BUF_LEN] = "";

/*
 * Internal status variables for aborting some voice modem actions.
 */

static int stop_dialing;
static int stop_playing;
static int stop_recording;
static int stop_waiting;

/*
 * We expect a maximum of 8 bit with 44100 samples per second, thus we need
 * a buffer size of 4410 bytes
 */

#define IS_101_BUFFER_SIZE 4410
static int buffer_size = IS_101_BUFFER_SIZE;

int IS_101_answer_phone (void)
     {
     reset_watchdog(0);

     if (voice_command("AT+VLS=2", "OK") != VMA_USER_1)
          return(VMA_ERROR);

     return(VMA_OK);
     }

int IS_101_beep (int frequency, int length)
     {
     char buffer[VOICE_BUF_LEN];
     time_t timeout;

     reset_watchdog(0);
     timeout = time(NULL) + length / 100 + cvd.port_timeout.d.i;
     sprintf(buffer, "AT+VTS=[%d,0,%d]", frequency, length);

     if (voice_command(buffer, "") != OK)
          return(FAIL);

     while ((!check_for_input(voice_fd)) && (timeout > time(NULL)))
          {
          reset_watchdog(100);
          delay(10);
          };

     if (voice_command("", "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

int IS_101_dial (char *number)
     {
     char command[VOICE_BUF_LEN];
     char buffer[VOICE_BUF_LEN];
     time_t timeout;
     int result = FAIL;

     voice_check_events();
     voice_modem_state = DIALING;
     stop_dialing = FALSE;
     reset_watchdog(0);
     timeout = time(NULL) + cvd.dial_timeout.d.i;
     sprintf(command, "ATD%s", (char*) number);

     if (voice_write(command) != OK)
          return(FAIL);

     while ((!stop_dialing) && (timeout >= time(NULL)))
          {
          reset_watchdog(10);

          if (check_for_input(voice_fd))
               {

               if (voice_read(buffer) != OK)
                    return(FAIL);

               result = voice_analyze(buffer, command);

               switch (result)
                    {
                    case VMA_BUSY:
                         stop_dialing = TRUE;
                         result = OK;
                         queue_event(create_event(BUSY_TONE));
                         break;
                    case VMA_EMPTY:
                         break;
                    case VMA_NO_ANSWER:
                         stop_dialing = TRUE;
                         result = OK;
                         queue_event(create_event(NO_ANSWER));
                         break;
                    case VMA_NO_DIAL_TONE:
                         stop_dialing = TRUE;
                         result = OK;
                         queue_event(create_event(NO_DIAL_TONE));
                         break;
                    case VMA_RINGING:
                    case VMA_USER_1:
                         break;
                    case VMA_OK:
                    case VMA_VCON:
                         stop_dialing = TRUE;
                         result = OK;
                         break;
                    default:
                         stop_dialing = TRUE;
                         result = FAIL;
                         break;
                    };

               }
          else
               delay(100);

          voice_check_events();
          };

     voice_modem_state = IDLE;
     return(result);
     }

int IS_101_handle_dle (char data)
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

int IS_101_init (void)
     {
     errno = ENOSYS;
     lprintf(L_ERROR, "%s: init called", voice_modem_name);
     return(FAIL);
     }

int IS_101_message_light_off (void)
     {
     reset_watchdog(0);

     if (voice_command("ATS0=0", "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

int IS_101_message_light_on (void)
     {
     reset_watchdog(0);

     if (voice_command("ATS0=254", "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

int IS_101_play_file (int fd)
     {
     TIO tio_save;
     TIO tio;
     char input_buffer[IS_101_BUFFER_SIZE];
     char output_buffer[2 * IS_101_BUFFER_SIZE];
     int i;
     int bytes_in;
     int bytes_out;

     reset_watchdog(0);
     stop_playing = FALSE;
     voice_modem_state = PLAYING;
     voice_check_events();
     tio_get(voice_fd, &tio);
     tio_save = tio;

     if (cvd.do_hard_flow.d.i)
          {

          if (voice_command("AT+FLO=2", "OK") != VMA_USER_1)
               return(FAIL);

          tio_set_flow_control(voice_fd, &tio, FLOW_HARD | FLOW_XON_OUT);
          }
     else
          {

          if (voice_command("AT+FLO=1", "OK") != VMA_USER_1)
               return(FAIL);

          tio_set_flow_control(voice_fd, &tio, FLOW_XON_OUT);
          };

     tio_set(voice_fd, &tio);

     if (voice_command("AT+VTX", "CONNECT") != VMA_USER_1)
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

          if (voice_modem == &ZyXEL_1496)
               {
               sprintf(output_buffer, "%c%c", DLE, DC4);
               lprintf(L_JUNK, "%s: <DLE> <DC4>", program_name);
               }
          else
               {
               sprintf(output_buffer, "%c%c%c%c", DLE, CAN, DLE, ETX);
               lprintf(L_JUNK, "%s: <DLE> <CAN> <DLE> <ETX>", program_name);
               };

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

     tio_set(voice_fd, &tio_save);

     if (voice_command("AT", "OK") != VMA_USER_1)
          return(FAIL);

     voice_check_events();
     voice_modem_state = IDLE;

     if (stop_playing)
          return(INTERRUPTED);

     return(OK);
     }

int IS_101_record_file (int fd)
     {
     TIO tio_save;
     TIO tio;
     time_t timeout;
     char input_buffer[IS_101_BUFFER_SIZE];
     char output_buffer[IS_101_BUFFER_SIZE];
     int i = 0;
     int bytes_in = 0;
     int bytes_out;
     int got_DLE_ETX = FALSE;
     int was_DLE = FALSE;

     reset_watchdog(0);
     timeout = time(NULL) + cvd.rec_max_len.d.i;
     stop_recording = FALSE;
     voice_modem_state = RECORDING;
     voice_check_events();
     tio_get(voice_fd, &tio);
     tio_save = tio;

     if (cvd.do_hard_flow.d.i)
          {

          if (voice_command("AT+FLO=2", "OK") != VMA_USER_1)
               return(FAIL);

          tio_set_flow_control(voice_fd, &tio, FLOW_HARD | FLOW_XON_IN);
          }
     else
          {

          if (voice_command("AT+FLO=1", "OK") != VMA_USER_1)
               return(FAIL);

          tio_set_flow_control(voice_fd, &tio, FLOW_XON_IN);
          };

     tio.c_cc[VMIN] = (buffer_size > 0xff) ? 0xff : buffer_size;
     tio.c_cc[VTIME] = 1;
     tio_set(voice_fd, &tio);

     if (voice_command("AT+VRX", "CONNECT") != VMA_USER_1)
          return(FAIL);

     while (!got_DLE_ETX)
          {
          reset_watchdog(10);

          if (timeout < time(NULL))
               voice_stop_recording();

          if ((bytes_in = voice_read_raw(input_buffer, buffer_size)) < 0)
               return(FAIL);

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

     voice_check_events();
     voice_modem_state = IDLE;
     return(OK);
     }

int IS_101_set_buffer_size (int size)
     {

     if (size > IS_101_BUFFER_SIZE)
          return(FAIL);

     buffer_size = size;
     return(OK);
     }

int IS_101_set_compression (int *compression, int *speed)
     {
     errno = ENOSYS;
     lprintf(L_ERROR, "%s: set_compression called", voice_modem_name);
     return(FAIL);
     }

int IS_101_set_device (int device)
     {
     errno = ENOSYS;
     lprintf(L_ERROR, "%s: set_device called", voice_modem_name);
     return(FAIL);
     }

int IS_101_stop_dialing (void)
     {
     stop_dialing = TRUE;
     return(OK);
     }

int IS_101_stop_playing (void)
     {
     stop_playing = TRUE;
     return(OK);
     }

int IS_101_stop_recording (void)
     {
     char buffer[VOICE_BUF_LEN];

     stop_recording = TRUE;
     sprintf(buffer, "%c!", DLE);
     lprintf(L_JUNK, "%s: <DLE> <!>", program_name);

     if (voice_write_raw(buffer, strlen(buffer)) != OK)
          return(FAIL);

     return(OK);
     }

int IS_101_stop_waiting (void)
     {
     stop_waiting = TRUE;
     return(OK);
     }

int IS_101_switch_to_data_fax (char *mode)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     sprintf(buffer, "AT+FCLASS=%s", mode);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

int IS_101_voice_mode_off (void)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     sprintf(buffer, "AT+FCLASS=%s", mode_save);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

int IS_101_voice_mode_on (void)
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

int IS_101_wait (int wait_timeout)
     {
     time_t timeout;

     reset_watchdog(0);
     stop_waiting = FALSE;
     voice_modem_state = WAITING;
     voice_check_events();
     timeout = time(NULL) + wait_timeout;

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

     voice_check_events();
     voice_modem_state = IDLE;
     return(OK);
     }
