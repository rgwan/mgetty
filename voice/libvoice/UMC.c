/*
 * Umc.c
 *
 * This file contains the UMC UM92144EF specific hardware stuff.
 * (e.g. Creatix PhoneMaster 144VFi)
 *
 * This file was originally written by Ulrich Homann <ulho@uni-paderborn.de>.
 *
 * Creatix Phonemaster 144VFi for sale (cheap&nasty UMC based Modem)
 *   contact me!
 *
 * New updated driver by Jens Adner <Jens.Adner@Wirtschaft.TU-Ilmenau.DE>.
 */

#include "../include/voice.h"

char *libvoice_UMC_c = "$Id: UMC.c,v 1.1 1997/12/16 12:21:04 marc Exp $";

#define UMC_RELEASE "0.01"
#define UMC_VTS_WORKAROUND yes
/* #define UMC_EXTENDED_DETECTION yes */
#define UMC_SPEAKER_ON yes
/* workaround: it should set by vgetty
 * ! program_name must be vgetty !
 */

static char mode_save[VOICE_BUF_LEN] = "";

/*
 * <DLE><ETX> string
 */
static char dletx[2]={ DLE, ETX };

/*
 * Internal status variables for aborting some voice modem actions.
 */

static int stop_dialing;
static int stop_playing;
static int stop_recording;
static int stop_waiting;
static int current_device=-1;

/*
 * The UMC samples with 7200 samples per second with a maximum of 4 bit
 * per sample. We want to buffer voice data for 0.1 second, so we need a
 * buffer less or equal to 7200 * 0.5 * 0.1 = 360 bytes.
 */

#define UMC_BUFFER_SIZE 360
static int buffer_size = UMC_BUFFER_SIZE;

static int UMC_set_buffer_size (int size)
     {

     if (size > UMC_BUFFER_SIZE)
           return(FAIL);

     buffer_size = size;
     return(OK);
     }

static int UMC_answer_phone (void)
     {
     reset_watchdog(0);

     if ((voice_command("ATA", "VCON") & VMA_USER) != VMA_USER)
          return(FAIL);

     return(OK);
     }

static int UMC_dial (char *number)
     {
     char buffer[VOICE_BUF_LEN];
     int result = FAIL;

     reset_watchdog(0);
     voice_modem_state = DIALING;
     stop_dialing = FALSE;
     sprintf(buffer, "ATD%s", (char*) number);

     if (voice_command(buffer, "") != OK)
          return(FAIL);

     while (!stop_dialing)
          {
          reset_watchdog(10);

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
                    {
                    stop_dialing = TRUE;
                    result = FAIL;
                    break;
                    }
               case VMA_VCON:
                    {
                    stop_dialing = TRUE;
                    result = OK;
                    break;
                    }
               };

          };

     voice_modem_state = IDLE;
     return(result);
     }

int UMC_handle_dle (char data)
     {

     switch (data)
          {
          case DLE:
               lprintf(L_WARN, "%s: Shielded <DLE> received", program_name);
               return(OK);
          case ETX:
               lprintf(L_WARN, "%s: <DLE> <ETX> received", program_name);
               return(OK);
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
          case 'f':  /* BELL ANSWER 2225 Hz     */ /* This one should be: data_detected */
          case 'a':  /* CCITT V.25/T.30 2100 Hz */
               return(queue_event(create_event(DATA_OR_FAX_DETECTED)));
          case 'b':
               return(queue_event(create_event(BUSY_TONE)));
          case 'c':
               return(queue_event(create_event(FAX_CALLING_TONE)));
          case 'd':
               return(queue_event(create_event(DIAL_TONE)));
          case 'e':
               return(queue_event(create_event(DATA_CALLING_TONE)));
          case 'h':
               return(queue_event(create_event(HANDSET_ON_HOOK)));
          case 't':
               return(queue_event(create_event(HANDSET_OFF_HOOK)));
          case 'o':
               lprintf(L_WARN, "%s: Buffer overrun", program_name);
               return(OK);
          case 'q':
               return(queue_event(create_event(SILENCE_DETECTED)));
          case 's':
               return(queue_event(create_event(NO_VOICE_ENERGY)));
          case 'T':
               return(OK);
          case 'u':
               lprintf(L_WARN, "%s: Buffer underrun", program_name);
               return(OK);
          };

     lprintf(L_WARN, "%s: Unknown code <DLE> <%c>", program_name, data);
     return(FAIL);
     }

int UMC_voice_mode_off (void)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     sprintf(buffer, "AT#CLS=%s", mode_save);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

static int UMC_voice_mode_on (void)
     {
     reset_watchdog(0);

     if (voice_command("AT#CLS?", "") != OK)
          return(FAIL);

     if (voice_read(mode_save) != OK)
          return(FAIL);

     if (voice_command("", "OK") != VMA_USER_1)
          return(FAIL);

     if (voice_command("AT#CLS=8", "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

static int UMC_init(void)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     lprintf(L_MESG, "initializing UMC voice modem");
     voice_modem_state = INITIALIZING;
     UMC_voice_mode_on();
     sprintf(buffer, "AT#VSP=%1u", cvd.rec_silence_len.d.i);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set silence period");

     if (voice_command("AT#VSD=0", "OK") != VMA_USER_1)
         lprintf(L_WARN, "can't disable silence deletion");

    if (voice_command("AT#VTD=3F,3F,3F", "OK") != VMA_USER_1)
         lprintf(L_WARN, "can't set DLE responses");

     if ((cvd.rec_silence_threshold.d.i > 100) ||
      (cvd.rec_silence_threshold.d.i < 0))
          {
          lprintf(L_ERROR, "Invalid threshold value.");
          return(ERROR);
          }

     sprintf(buffer, "AT#VSS=%1u", cvd.rec_silence_threshold.d.i * 3 / 100);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set silence threshold");

     UMC_voice_mode_off();

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

static int UMC_set_compression (int *compression, int *speed, int *bits)
     {
     reset_watchdog(0);

     if (*compression == 0)
          *compression = 2;

     if (*speed == 0)
          *speed = 7200;

     switch (*compression)
          {
          case 2:
               *bits=2;

               if (voice_command("AT#VBS=2", "OK") != VMA_USER_1)
                    return(FAIL);

               UMC_set_buffer_size((*speed) * (*bits) / 10 / 8);
               return (OK);
          case 3:
               *bits=3;

               if (voice_command("AT#VBS=3", "OK") != VMA_USER_1)
                    return(FAIL);

               UMC_set_buffer_size((*speed) * (*bits) / 10 / 8);
               return (OK);
          case 4:
               *bits=4;

               if (voice_command("AT#VBS=4", "OK") != VMA_USER_1)
                    return(FAIL);

               UMC_set_buffer_size((*speed) * (*bits) / 10 / 8);
               return (OK);
          }

     lprintf(L_WARN,
      "UMC handle event: Illegal voice compression method (%d)",
      *compression);
     return(FAIL);
     }

static int UMC_set_device (int device)
     {
     reset_watchdog(0);

     if ((current_device != device) && (current_device >= 0))
          voice_command("ATH0","VCON|OK");

     current_device=device;

     switch (device)
          {
          case NO_DEVICE:
               voice_command("AT#VLS=0", "OK");
               return(OK);
          case DIALUP_LINE:
               voice_command("AT#VLS=4", "OK");
               return(OK);
          case EXTERNAL_MICROPHONE:
               voice_command("AT#VLS=3", "VCON");
               return(OK);
          case INTERNAL_SPEAKER:
               voice_command("AT#VLS=2", "VCON");
               return(OK);
          case LOCAL_HANDSET:
               voice_command("AT#VLS=1","VCON");
               return(OK);
          /* case SPEAKER_PHONE_MODE:
           *    voice_command("AT#VLS=6","OK");
           *    return(OK);
           */
          }

     lprintf(L_WARN, "%s: Unknown output device (%d)", voice_modem_name,
      device);
     return(FAIL);
     }

static int UMC_beep (int frequency, int length)
     {
#ifdef UMC_VTS_WORKAROUND
       /* generate a beep with 900Hz
     * sorry: just a near miss.
     */
       TIO tio_save;
       TIO tio;

       char *sinewave="\x37\x8c\xc8\x73";
       int sinelen=4;
       int i;

       tio_get(voice_fd, &tio);
       tio_save = tio;
       tio_set_flow_control(voice_fd, &tio, FLOW_HARD);
       tio_set(voice_fd, &tio);

       voice_command("AT#VBS=4", "OK");
       voice_command("AT#VTX", "CONNECT");

       lprintf(L_JUNK, "%s->%s: sinewave", program_name, voice_modem_name);
       for (i=length; i>0; i--){
       if (write(voice_fd,sinewave,sinelen) != sinelen)
         lprintf(L_ERROR, "%s->%s: write error (errno 0x%x)",
              program_name, voice_modem_name, errno);
       }
       lprintf(L_JUNK, "%s->%s: <DLE> <ETX>", program_name, voice_modem_name);
       if (write(voice_fd, dletx , 2) != 2)
      lprintf(L_ERROR, "%s->%s: write error (errno 0x%x)",
           program_name, voice_modem_name, errno);

       tio_set(voice_fd, &tio_save);
       voice_command("", "VCON");
#else

     char buffer[VOICE_BUF_LEN];

     reset_watchdog(0);
     sprintf(buffer, "AT#VTS=[%d,0,%d]", frequency, length / 10);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          return(FAIL);
#endif
     return(OK);
     }

static int UMC_play_file(int fd)
     {
     TIO tio_save;
     TIO tio;
     char input_buffer[UMC_BUFFER_SIZE];
     char output_buffer[2 * UMC_BUFFER_SIZE];
     int i;
     int bytes_in;
     int bytes_out;

#ifdef USE_READ
     /*int speed,delay_time;*/
     int writeRequests=0,writeRequestsGranted=0;
#endif

     reset_watchdog(0);
     voice_check_events();
     stop_playing = FALSE;
     voice_modem_state = PLAYING;
     tio_get(voice_fd, &tio);
     tio_save = tio;

     if (cvd.do_hard_flow.d.i)
          {

          if (voice_command("AT\\Q3", "OK") != VMA_USER_1)
               return(FAIL);

          tio_set_flow_control(voice_fd, &tio, FLOW_HARD | FLOW_XON_OUT);
          }
     else
          {

          if (voice_command("AT\\Q1", "OK") != VMA_USER_1)
               return(FAIL);

          tio_set_flow_control(voice_fd, &tio, FLOW_XON_OUT);
          };

     tio_set(voice_fd, &tio);

#ifdef USE_READ
     /*fcntl(voice_fd, F_SETFL, O_RDWR|O_NDELAY);*/
     /*speed=tio_get_speed(&tio);*/
     /*delay_time=buffer_size*/
#endif

     if (voice_command("AT#VTX", "CONNECT") != VMA_USER_1)
          return(FAIL);

     while (!stop_playing)
          {
          reset_watchdog(10);

          if ((bytes_in = read(fd, input_buffer, buffer_size)) <= 0)
               {
               /* if interrupted, we still do the job, until handle_events */
               /* tells us otherwise */

               if(errno!=EINTR||bytes_in==0)
                    break;

               bytes_in=0;
               }

          if (bytes_in)
               {
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

#ifdef USE_READ
               {
               int result;
               char modem_byte;

               fcntl(voice_fd, F_SETFL, O_RDWR|O_NDELAY);
               result=read(voice_fd, &modem_byte, 1);
               fcntl(voice_fd, F_SETFL, O_RDWR);

               if (result != 1)
                    {

                    if (result!=0)
                         lprintf(L_ERROR, "%s: could not read byte from voice modem", program_name);

                    }
               else
                    {

                    if (modem_byte == DLE)
                         {

                         if ((modem_byte = voice_read_char()) == FAIL)
                              return(FAIL);

                         lprintf(L_JUNK, "%s: <DLE> <%c>",
                          voice_modem_name, (int)modem_byte);
                         voice_modem->handle_dle((char)modem_byte);
                         }
                    else
                         lprintf(L_WARN, "UMC:%s: !!unexpected byte %c from voice modem",
                          program_name, (int)modem_byte);

                    }

               }
#else
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
                         voice_modem->handle_dle((char)modem_byte);
                         }
                    else
                         lprintf(L_WARN, "%s: unexpected byte %c from voice modem",
                          program_name, (int)modem_byte);

               };
#endif

               }

          voice_check_events();
          };

     if (stop_playing)
          {
          lprintf(L_JUNK, "%s: flushing output queue", program_name);
          tio_flush_queue(voice_fd,TIO_Q_OUT);
          lprintf(L_JUNK, "%s: flushed output queue", program_name);
          sprintf(&output_buffer[1], "%c%c%c%c", DLE, CAN, DLE, ETX);
          output_buffer[0]=0;
          lprintf(L_JUNK, "%s: <NUL> <DLE> <CAN> <DLE> <ETX>", program_name);

          if (voice_write_raw(output_buffer, strlen(&output_buffer[1])) != OK)
               return(FAIL);

          }
     else
          {
          sprintf(output_buffer, "%c%c", DLE, ETX);
          lprintf(L_JUNK, "%s: <DLE> <ETX>", program_name);

          if (voice_write_raw(output_buffer, strlen(output_buffer)) != OK)
               return(FAIL);

          };

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

int UMC_stop_dialing (void)
     {
     stop_dialing = TRUE;
     return(OK);
     }

int UMC_stop_playing (void)
     {
     stop_playing = TRUE;
     return(OK);
     }

int UMC_stop_recording (void)
     {
     /*
      * char buffer[VOICE_BUF_LEN];
      */

     stop_recording = TRUE;

     /*
      * It seems not all UMC modems support <DLE> <!> to stop
      * the recording, so we change this back to the old way
      *
      * sprintf(buffer, "%c!", DLE);
      * lprintf(L_JUNK, "%s: <DLE> <!>", program_name);
      */

     if (voice_write("!") != OK)
          return(FAIL);

     return(OK);
     }

int UMC_stop_waiting (void)
     {
     stop_waiting = TRUE;
     return(OK);
     }

int UMC_switch_to_data_fax (char *mode)
     {
     char buffer[VOICE_BUF_LEN];
     reset_watchdog(0);
     sprintf(buffer, "AT+FCLASS=%s", mode);

     if (voice_command(buffer, "OK") != VMA_USER_1)
          return(FAIL);

     return(OK);
     }

static int UMC_wait (int wait_timeout)
     {
     time_t timeout;

     reset_watchdog(0);
     voice_check_events();
     stop_waiting = FALSE;
     voice_modem_state = WAITING;
     timeout = time(NULL) + wait_timeout;

     while ((!stop_waiting) && (timeout >= time(NULL)))
          {

#ifdef USE_READ
          int result;
          char char_read;
          char_read=0;
          reset_watchdog(10);
          fcntl(voice_fd, F_SETFL, O_RDWR|O_NDELAY);
          result=read(voice_fd, &char_read, 1);
          fcntl(voice_fd, F_SETFL, O_RDWR);

          if (result != 1)
               {

               if(result!=0)
                    {
                    lprintf(L_ERROR, "%s: could not read byte from voice modem",
                     program_name);
                    return (FAIL);
                    }
               }
          else
               {

               if (char_read == DLE)
                    {

                    if ((char_read = voice_read_char()) == FAIL)
                         return(FAIL);

                    lprintf(L_JUNK, "%s: <DLE> <%c>", voice_modem_name, char_read);
                    voice_modem->handle_dle((char)char_read);
                    }
               else
                    {
                    lprintf(L_WARN,
                     "%s: unexpected byte <%c> from voice modem",
                     program_name, char_read);
                    }

               }

#else
          reset_watchdog(10);

          while (check_for_input(voice_fd))
               {
               char char_read;

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
                    {
                    lprintf(L_WARN,
                     "%s: unexpected byte <%c> from voice modem",
                     program_name, char_read);
                    }

               };

#endif
          voice_check_events();
          delay(100);
          };

     voice_modem_state = IDLE;
     voice_check_events();
     return(OK);
     }

static int UMC_record_file (int fd)
     {
     TIO tio_save;
     TIO tio;
     time_t timeout;
     char input_buffer[UMC_BUFFER_SIZE];
     char output_buffer[UMC_BUFFER_SIZE];
     int i = 0;
     int bytes_in = 0;
     int bytes_out;
     int got_DLE_ETX = FALSE;
     int was_DLE = FALSE;

     reset_watchdog(0);
     voice_check_events();
     timeout = time(NULL) + cvd.rec_max_len.d.i;
     stop_recording = FALSE;
     voice_modem_state = RECORDING;
     tio_get(voice_fd, &tio);
     tio_save = tio;

     if (cvd.do_hard_flow.d.i)
          {

          if (voice_command("AT\\Q3", "OK") != VMA_USER_1)
               return(FAIL);

          tio_set_flow_control(voice_fd, &tio, FLOW_HARD | FLOW_XON_OUT);
          }
     else
          {

          if (voice_command("AT&K6", "OK") != VMA_USER_1)
               return(FAIL);

          tio_set_flow_control(voice_fd, &tio, FLOW_XON_OUT);
          };

     tio.c_cc[VMIN] = (buffer_size > 0xff) ? 0xff : buffer_size;
     tio.c_cc[VTIME] = 1;
     tio_set(voice_fd, &tio);

     if (voice_command("AT#VRX", "CONNECT") != VMA_USER_1)
          return(FAIL);

     while (!got_DLE_ETX)
          {
          reset_watchdog(10);

          if (timeout < time(NULL))
               voice_stop_recording();

          while ((bytes_in = read(voice_fd, input_buffer, buffer_size)) <= 0)
               {
               /* RETRY if the read failed due to a PIPE signal or whatever */
               /* EVENT will be queued, so we can do what we have to        */

               if ((bytes_in == 0) || (errno != EINTR))
                    {
                    lprintf(L_ERROR,
                     "%s: could not read byte from voice modem", program_name);
                    return(FAIL);
                    }

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
                              break;
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
          int j;

          lprintf(L_JUNK, "%s: data left in buffer: ", voice_modem_name);

          for (j = i; j < bytes_in ; j++)
               {
               lputc(L_JUNK, input_buffer[j]);
               }

          if (voice_command("AT", "OK") != VMA_USER_1)
               return(FAIL);

          };

     if (voice_command("AT", "OK") != VMA_USER_1)
          return(FAIL);

     voice_modem_state = IDLE;
     voice_check_events();
     return(OK);
     }

voice_modem_struct UMC =
     {
     "UMC",
     "UMC",
     &UMC_answer_phone,
     &UMC_beep,
     &UMC_dial,
     &UMC_handle_dle,
     &UMC_init,
     &IS_101_message_light_off,
     &IS_101_message_light_on,
     &UMC_play_file,
     &UMC_record_file,
     &UMC_set_compression,
     &UMC_set_device,
     &UMC_stop_dialing,
     &UMC_stop_playing,
     &UMC_stop_recording,
     &UMC_stop_waiting,
     &UMC_switch_to_data_fax,
     &UMC_voice_mode_off,
     &UMC_voice_mode_on,
     &UMC_wait
     };
