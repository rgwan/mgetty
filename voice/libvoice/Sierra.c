/*
 * Sierra.c
 *
 * This file contains specific hardware stuff for Sierra based
 * voice modems.
 *
 * The Sierra driver is written and maintained by
 * Luke Bowker <puke@suburbia.net>.
 *
 */

#include "../include/voice.h"

char *libvoice_Sierra_c = "$Id: Sierra.c,v 1.1 1997/12/16 12:21:03 marc Exp $";

#define Sierra_BUFFER_SIZE 255
#define ACK    0x06

static TIO tio;
static TIO tio_save;

static int stop_playing;
static int stop_recording;

static int buffer_size = Sierra_BUFFER_SIZE;


int Sierra_init (void)
     {
     return(voice_mode_on());
     }

int Sierra_answer_phone (void)
     {
     reset_watchdog(0);

     if ((voice_command("ATA", "#VCON") & VMA_USER) != VMA_USER)
          return(FAIL);

     return(OK);
     }

static int Sierra_voice_mode_on (void)
     {
     char buffer[Sierra_BUFFER_SIZE];

     reset_watchdog(0);
     voice_modem_state = INITIALIZING;
     lprintf(L_MESG, "initializing Sierra voice modem");
     tio_get(voice_fd, &tio);
     tio_save = tio;

     /*
      * AT#VSn - Enable voice mode at bit rate n (1 = 115.2k).
      */

     if (voice_command("AT#VS1", "OK") != VMA_USER_1)
          lprintf(L_WARN, "Voice mode didn't work");

     tio_set_speed(&tio, 115200);
     tio_set(voice_fd, &tio);
     sprintf(buffer, "ATM2L3#VL=0#VSM=2#VSC=0#VSS=3#VSI=%1u",
      cvd.rec_silence_len.d.i);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "Couldn't set voice mode options");

     sprintf(buffer, "AT#VF=0");

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "Couldn't set buffer size to %d", buffer_size);

     voice_modem_state = IDLE;
     return(OK);
     }

int Sierra_voice_mode_off (void)
     {
     reset_watchdog(0);

     if (voice_command("AT#VS0", "OK") != VMA_USER_1)
          return(FAIL);

     tio_set(voice_fd, &tio_save);
     return(OK);
     }

static int Sierra_set_compression (int *compression, int *speed, int *bits)
     {
     reset_watchdog(0);

     if (*compression == 0)
          *compression = 2;

     if (*speed == 0)
          *speed = 9600;

     if (*speed != 9600)
          {
          lprintf(L_WARN, "%s: Illegal sample rate (%d)", voice_modem_name,
           *speed);
          return(FAIL);
          };

     if (*compression != 2)
          {
          lprintf(L_WARN, "%s: Illegal voice compression method (%d)",
           voice_modem_name, *compression);
          return(FAIL);
          };

     *bits = 8;

     if (voice_command("AT#VSM=2", "OK") != VMA_USER_1)
          return(FAIL);

     IS_101_set_buffer_size((*speed) * (*bits) / 10 / 8);
     return(OK);
     }

int Sierra_play_file (int fd)
     {
     char input_buffer[16 * Sierra_BUFFER_SIZE];
     char output_buffer[32 * Sierra_BUFFER_SIZE];
     int i;
     int bytes_in;
     int bytes_out;

     reset_watchdog(0);
     stop_playing = FALSE;
     voice_modem_state = PLAYING;
     voice_check_events();

     if (voice_command("AT#VSV", "CONNECT") != VMA_USER_1)
          return(FAIL);

     while (!stop_playing)
          {
          reset_watchdog(10);

          if ((bytes_in = read(fd, input_buffer, buffer_size * 16)) <= 0)
               break;

          bytes_out = 0;

          for (i = 0; i < bytes_in; i++)
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

          if ((bytes_in = read(fd, input_buffer, buffer_size * 16)) <= 0)
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
          sprintf(output_buffer, "%c%c%c%c", DLE, CAN, DLE, ETX);
          lprintf(L_JUNK, "%s: <DLE> <CAN> <DLE> <ETX>", program_name);
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

     voice_modem_state = IDLE;
     voice_check_events();

     if (stop_playing)
          return(INTERRUPTED);

     return(OK);
     }

int Sierra_beep (int frequency, int length)
     {

     /*
      * The docs I have define a command as "AT#VB=f,t" for playing tones,
      * but I couldn't get it to work. So we'll generate one with code
      * flogged from sine.c in the libpvf code.
      */

     double freq;
     int time, i;
     char d;

     time = 96 * length;
     freq = (double) frequency;

     if (voice_command("AT#VSV", "CONNECT") != VMA_USER_1)
          return(FAIL);

     for(i = 0; i < time; i++)
          {
          d = (char) (i * freq / 9600) * 0x7f;
          voice_write_char(d);
          }

     voice_write_char(DLE);
     voice_write_char(ETX);

     if ((voice_command("", "OK|VCON") & VMA_USER) != VMA_USER)
          return(FAIL);

     return(OK);
     }

int Sierra_record_file (int fd)
     {
     TIO tio_save;
     TIO tio;
     time_t timeout;
     char input_buffer[Sierra_BUFFER_SIZE];
     char output_buffer[Sierra_BUFFER_SIZE];
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
     tio.c_cc[VMIN] = (buffer_size > 0xff) ? 0xff : buffer_size;
     tio.c_cc[VTIME] = 1;
     tio_set(voice_fd, &tio);
     tio_set_flow_control(voice_fd, &tio, FLOW_NONE);
     tio_mode_raw(&tio);

     if (voice_command("AT#VD1", "CONNECT") != VMA_USER_1)
          return(FAIL);

     /*
      * Sierra has a weird handshaking protocol. It sends the first byte,
      * then waits for an ACK. Then it sends buffer_size bytes. Then the
      * cycle starts again. What's worse, on my modem at least, I couldn't
      * get this to work properly. It would seem to lose samples near the
      * end of each block. Thus I set the initialisation sequence to turn
      * the modem's FIFO off, which is probably fine if you are using a
      * 16550A on a lightly loaded machine.
      */

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

          bytes_in++;
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
                         case '#':

                              if (input_buffer[++i] != 'V')
                                   lprintf(L_JUNK, "%s: Huh? <DLE>#%c",
                                    voice_modem_name, input_buffer[i]);
                              else
                                   {

                                   if (input_buffer[++i] == 'T')
                                        {

                                        if (input_buffer[++i] == 'H')
                                             queue_event(create_event(
                                              HANDSET_ON_HOOK));
                                        else if (input_buffer[i] == 'O')
                                             queue_event(create_event(
                                              HANDSET_OFF_HOOK));
                                        else
                                             lprintf(L_WARN,
                                              "%s, ??? Got <DLE>#VT%c",
                                              voice_modem_name,
                                              input_buffer[i]);

                                        }
                                   else if (input_buffer[i] == 'D')
                                        {

                                        if (input_buffer[++i] == 'D' ||
                                         input_buffer[i] == 'G')
                                             /*
                                              * queue_event(create_event(FAX_CALLING_TONE));
                                              */
                                             lprintf(L_WARN,
                                              "Fax calling tone received. Disabled");
                                        else
                                             {
                                             event_type *event;
                                             event_data dtmf;

                                             event = create_event(
                                              RECEIVED_DTMF);
                                             dtmf.c = input_buffer[i];
                                             event->data = dtmf;
                                             queue_event(event);
                                             }

                                        }
                                   else
                                        lprintf(L_WARN,
                                         "%s: Unsupported: <DLE>#V%c %d",
                                         voice_modem_name,
                                         input_buffer[i], i);

                                   }

                              break;
                         default:
                              lprintf(L_WARN, "%s: <DLE> <%c>",
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
               lprintf(L_JUNK, "%x", input_buffer[j]);

          if (voice_command("", "OK") != VMA_USER_1)
               return(FAIL);

          };

     if (voice_command("AT", "OK") != VMA_USER_1)
          return(FAIL);

     voice_modem_state = IDLE;
     voice_check_events();
     return(OK);
     }

int Sierra_set_device(int device)
     {
     lprintf(L_WARN,
      "%s: set_device(%d) called. Doesn't achieve anything in this implementation",
      voice_modem_name, device);
     return(OK);
     }

voice_modem_struct Sierra =
     {
     "Sierra",
     "Sierra",
     &Sierra_answer_phone,
     &Sierra_beep,
     &IS_101_dial,
     &IS_101_handle_dle,
     &Sierra_init,
     &IS_101_message_light_off,
     &IS_101_message_light_on,
     &Sierra_play_file,
     &Sierra_record_file,
     &Sierra_set_compression,
     &Sierra_set_device,
     &IS_101_stop_dialing,
     &IS_101_stop_playing,
     &IS_101_stop_recording,
     &IS_101_stop_waiting,
     &IS_101_switch_to_data_fax,
     &Sierra_voice_mode_off,
     &Sierra_voice_mode_on,
     &IS_101_wait
     };
