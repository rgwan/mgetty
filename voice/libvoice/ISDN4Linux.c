/*
 * ISDN4Linux.c
 *
 * This file contains the ISDN4Linux specific hardware stuff.
 *
 * Bjarne Pohlers <bjarne@math.uni-muenster.de> wrote this driver and
 * maintains it. It is based on the old driver by Fritz Elfert
 * <fritz@wuemaus.franken.de> and the generic hardware driver in
 * IS_101.c
 *
 * Release Notes: You should use a recent version of the ISDN. I
 * recommend kernel 2.0.29 with patches isdn4kernel2.0.29,
 * isdn4kernel2.0.29.1, isdn4kernel2.0.29.2, isdn4kernel2.0.29.3 and
 * isdn4kernel2.0.29.4 (see ftp://ftp.franken.de/pub/isdn4linux) or
 * any later version. Older Kernels might work, but you might run in
 * trouble because of the short RING intervals there.
 *
 * I suggested in previous releases to set
 *   rec_compression 6
 *   raw_data true
 * in voice.conf as I did not test anything else. However,
 * rec_compression works fine and (in order to save disk space) you
 * should now set rec_compression to 2,3 or 4 and raw_data to false.
 *
 * You can convert your old raw audio files with the command
 * sox -tul -r 8000 INFILE -tau - | autopvf -8 | pvftormd ISDN4Linux NR >OUTFILE
 * to the new format. NR is the compression method (2,3 or 4) which needs not
 * necessarily be the same as you selected in voice.conf.
 *
 * To play a recorded file use the command
 * rmdtopvf <FILENAME | pvftoau >/dev/audio
 *
 * In mgetty.config there should be an init-chat-string for each port
 * similar to the following one:
 *   init-chat "" ATZ\d OK AT&E<Your MSN> OK
 * If you add
 *   ATS18=1 OK
 * there your isdn-tty won't pick up data calls.
 *
 * */

#include "../include/voice.h"

char *libvoice_ISDN4Linux_c = "$Id: ISDN4Linux.c,v 1.1 1997/12/16 12:20:59 marc Exp $";

#define ISDN4LINUX_BUFFER_SIZE 800
static int isdn4linux_buffer_size = ISDN4LINUX_BUFFER_SIZE;

static int is_voicecall;
static int stop_playing;
static int stop_recording;
static int got_DLE_DC4 = FALSE;

/*
 * This function handles the <DLE> shielded codes.
 */
int ISDN4Linux_handle_dle (char data)
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
       * DC4: abort
       */

    case DC4:
      lprintf(L_WARN, "%s: <DLE> <DC4> received", program_name);
      voice_stop_current_action();
      switch (voice_command("", "OK|VCON|NO CARRIER"))
     {
     case VMA_USER_3:
       queue_event(create_event(NO_CARRIER));
       break;
     case VMA_USER_1:
     case VMA_USER_2:
       break;
     default:
       return FAIL;
     }
      got_DLE_DC4 = TRUE;
      return (OK);

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

    case '$': /* Do not know what this is for... Who cares? */
      return(queue_event(create_event(BONG_TONE)));

      /*
       * Start of DTMF shielding
       */

    case '/': /* This cannot happen with ISDN, but who cares? */
      lprintf(L_WARN, "%s: Start of DTMF shielding received",
           program_name);
      return(OK);

      /*
       * End of DTMF shielding
       */

    case '~': /* This still can't happen, I hope nobody cares... */
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

    case 'a': /* maybe one day this will be supported by the kernel */
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

static int ISDN4Linux_answer_phone (void)
{
  int result;
  reset_watchdog(0);
  /* Check call-type:
   * S20 = 1 -> voice call
   * S20 = 4 -> data call
   */
  result = voice_command("ATS20?", "0|1|2|3|4");
  is_voicecall = (result==VMA_USER_2);
  if (is_voicecall)
    {
      return(voice_command("ATA","VCON"));
    }
  return(OK);
}

static int ISDN4Linux_init (void)
{
  static char buffer[VOICE_BUF_LEN] = "";
  unsigned reg_content;

  voice_modem_state = INITIALIZING;
  lprintf(L_MESG, "initializing ISDN4Linux voice mode");
  reset_watchdog(0);

  /* Enable voice calls (set bit 1 in register S18) */
  if (voice_command("ATS18?", "") != OK)
    return(FAIL);

  if (voice_read(buffer) != OK)
    return(FAIL);

  if (voice_command("", "OK") != VMA_USER_1)
    return(FAIL);

  reg_content=atoi(buffer);

  sprintf(buffer, "ATS18=%u", reg_content|1);

  if (voice_command(buffer, "OK") != VMA_USER_1)
    return(FAIL);

  /* Enable CALLER NUMBER after first RING (set bit 4 in register S13) */
  if (voice_command("ATS13?", "") != OK)
    return(FAIL);

  if (voice_read(buffer) != OK)
    return(FAIL);

  if (voice_command("", "OK") != VMA_USER_1)
    return(FAIL);

  reg_content=atoi(buffer);

  sprintf(buffer, "ATS13=%u", reg_content|(1<<4));

  if (voice_command(buffer, "OK") != VMA_USER_1)
    return(FAIL);

#if ISDN_FUTURE
  {
    char buffer[VOICE_BUF_LEN];
    /*
     * ATS40.3=1 - Enable distincitve ring type 1 (RING)
     * ATS40.4=1 - Enable distincitve ring type 2 (RING 1)
     * ATS40.5=1 - Enable distincitve ring type 3 (RING 2)
     * ATS40.6=1 - Enable distincitve ring type 4 (RING 3)
     */

    /*
     * AT+VSD=x,y - Set silence threshold and duration.
     */
    sprintf(buffer, "AT+VSD=%d,%d",
         cvd.rec_silence_threshold.d.i * 31 / 100,
         cvd.rec_silence_len.d.i);
    if (voice_command(buffer, "OK") != VMA_USER_1)
      lprintf(L_WARN,
           "setting recording preferences didn't work");
  }
#endif /* ISDN_FUTURE */

  voice_modem_state = IDLE;
  return(OK);
}

static int ISDN4Linux_beep (int frequency, int length)
{
#ifdef ISDN_FUTURE
  reset_watchdog(0);
  char buffer[VOICE_BUF_LEN];
  sprintf(buffer, "AT+VTS=[%d,0,%d]", frequency,
       length);
  if (voice_command(buffer, "OK") != VMA_USER_1)
    return(ERROR);
#endif
  return(OK);
}
static int ISDN4Linux_set_compression (int *compression, int *speed, int *bits)
{
  char buffer[VOICE_BUF_LEN];
  reset_watchdog(0);

  if (*compression == 0)
    *compression = 2;
  *speed = 8000;

  switch (*compression) {
  case 2:
    *bits = 2;
    if (voice_command("AT+VSM=2", "OK") != VMA_USER_1)
      return(FAIL);
    isdn4linux_buffer_size =
      ISDN4LINUX_BUFFER_SIZE * 2 / 4;
    return(OK);
  case 3:
    *bits = 3;
    if (voice_command("AT+VSM=3", "OK") != VMA_USER_1)
      return(FAIL);
    isdn4linux_buffer_size =
      ISDN4LINUX_BUFFER_SIZE * 3 / 4;
    return(OK);
  case 4:
    *bits = 4;
    if (voice_command("AT+VSM=4", "OK") != VMA_USER_1)
      return(FAIL);
    isdn4linux_buffer_size =
      ISDN4LINUX_BUFFER_SIZE * 4 / 4;
    return(OK);
  case 5:
  case 6:
    *bits = 8;
    sprintf(buffer,"AT+VSM=%d",*compression);
    if (voice_command(buffer, "OK") != VMA_USER_1)
      return(FAIL);
    isdn4linux_buffer_size =
      ISDN4LINUX_BUFFER_SIZE * 8 / 4;
    return(OK);
  }
  lprintf(L_WARN,
       "ISDN4Linux handle event: Illegal voice compression method (%d)",
       *compression);
  return(FAIL);
}

static int ISDN4Linux_set_device (int device)
{
  int result;
  reset_watchdog(0);

  switch (device) {
  case NO_DEVICE:
    voice_write("AT+VLS=0");

    result = voice_command("", "OK|NO CARRIER|AT+VLS=0");
    if (result == VMA_USER_3)
      result = voice_command("", "OK|NO CARRIER");

    switch(result)
      {
      case VMA_USER_2:
     queue_event(create_event(NO_CARRIER)); /* Fall through */
      case VMA_USER_1:
     return (OK);
      }

    return(FAIL);
  case DIALUP_LINE:
    switch (voice_command("AT+VLS=2", "VCON|OK|NO CARRIER") )
      {
      case VMA_USER_3:
     queue_event(create_event(NO_CARRIER)); /* Fall through */
      case VMA_USER_1:
      case VMA_USER_2:
     return(OK);
      }
      return(FAIL);
  }
  lprintf(L_WARN,
       "ISDN4Linux handle event: Unknown output device (%d)",
       device);
  return(FAIL);
}

int ISDN4Linux_play_file (int fd)
{
  TIO tio_save;
  TIO tio;
  char input_buffer[2 * ISDN4LINUX_BUFFER_SIZE];
  char output_buffer[4 * ISDN4LINUX_BUFFER_SIZE];
  int i;
  int bytes_in;
  int bytes_out;
  int abort_playing=0;

  stop_playing = FALSE;
  voice_modem_state = PLAYING;
  voice_check_events();
  reset_watchdog(0);
  tio_get(voice_fd, &tio);
  tio_save = tio;

  if (!is_voicecall) {
    return(queue_event(create_event(DATA_CALLING_TONE)));
  }
  tio_set_flow_control(voice_fd, &tio, FLOW_HARD);

  tio_set(voice_fd, &tio);

  switch (voice_command("AT+VTX", "CONNECT|NO ANSWER"))
    {
    case VMA_USER_1:
      break;
    case VMA_USER_2:
      return(queue_event(create_event(NO_ANSWER)));
    default:
      return(FAIL);
    }

  while (!stop_playing)
    {
      reset_watchdog(10);

      if ((bytes_in = read(fd, input_buffer, isdn4linux_buffer_size)) <= 0)
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

       if (!abort_playing)
         {
           if (modem_byte == DLE)
          {

            if ((modem_byte = voice_read_char()) == FAIL)
              return(FAIL);

            switch (modem_byte)
              {
              case DC4:
                lprintf(L_WARN, "%s: <DLE> <DC4> received", program_name);
                voice_modem->stop_playing();
                switch (voice_command("", "OK|VCON|NO CARRIER"))
            {
            case VMA_USER_3:
              queue_event(create_event(NO_CARRIER));
              break;
            case VMA_USER_1:
            case VMA_USER_2:
              break;
            default:
              return FAIL;
            }

                abort_playing=stop_playing=1;
                break;
              default:
                lprintf(L_JUNK, "%s: <DLE> <%c>", voice_modem_name,
                     modem_byte);
                voice_modem->handle_dle(modem_byte);
              }
          }
           else
          lprintf(L_WARN, "%s: unexpected byte %c from voice modem",
               program_name, modem_byte);
         }
     };

      voice_check_events();
    };

  if (!abort_playing)
    {
      if (0 && stop_playing)
     {
     /* According to the documentation of isdn4k-2.0.29
        the following will stop *recording* so it is currently disabled.
        We will use <DLE> <ETX> instead. Anyway I do not see any
        reason why the final DLE-sequence should depend on stop-playing */
       sprintf(output_buffer, "%c%c", DLE, DC4);
       lprintf(L_JUNK, "%s: <DLE> <DC4>", program_name);
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
    }
  tio_set(voice_fd, &tio_save);

  i=voice_command("AT", "OK|ERROR");
  /* Workaround for a bug in the ISDN-TTY driver */
  if (i == VMA_USER_2 && abort_playing)
    {
      /* It seems the bug is present. Send <DLE> <ETX> and try it again */
      sprintf(output_buffer, "%c%c", DLE, ETX);
      lprintf(L_JUNK, "%s: <DLE> <ETX>", program_name);
      i=voice_command("AT", "OK");
    }
  if (i != VMA_USER_1)
    return(FAIL);

  voice_modem_state = IDLE;
  voice_check_events();

  if (stop_playing)
    return(INTERRUPTED);

  return(OK);
}

int ISDN4Linux_record_file (int fd)
{
  TIO tio_save;
  TIO tio;
  time_t timeout;
  char input_buffer[2*ISDN4LINUX_BUFFER_SIZE];
  char output_buffer[2*ISDN4LINUX_BUFFER_SIZE];
  int i = 0;
  int bytes_in = 0;
  int bytes_out;
  int got_DLE_ETX = FALSE;
  int was_DLE = FALSE;
  int buffer_analyze;

  timeout = time(NULL) + cvd.rec_max_len.d.i;
  stop_recording = FALSE;
  voice_modem_state = RECORDING;
  voice_check_events();
  reset_watchdog(0);
  tio_get(voice_fd, &tio);
  tio_save = tio;


  tio_set_flow_control(voice_fd, &tio, FLOW_HARD | FLOW_XON_IN);

  tio.c_cc[VMIN] = (isdn4linux_buffer_size > 0xff) ? 0xff
    : isdn4linux_buffer_size;
  tio.c_cc[VTIME] = 1;
  tio_set(voice_fd, &tio);

  switch (voice_command("AT+VRX", "CONNECT|NO ANSWER"))
    {
    case VMA_USER_1:
      break;
    case VMA_USER_2:
      return(queue_event(create_event(NO_ANSWER)));
    default:
      return(FAIL);
    }

  while (!got_DLE_ETX)
    {
      reset_watchdog(10);

      if (timeout < time(NULL))
     voice_stop_recording();

      if ((bytes_in = read(voice_fd, input_buffer, isdn4linux_buffer_size)) <=
       0)
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
  buffer_analyze = voice_analyze(&input_buffer[i], "\r\nOK|\r\nVCON|\r\nNO CARRIER");
  switch (buffer_analyze)
    {
    case VMA_USER_1:
    case VMA_USER_2:
      lprintf(L_JUNK, "%s: OK|VCON", voice_modem_name);
      break;
    case VMA_USER_3:
      queue_event(create_event(NO_CARRIER));
      break;
    default:
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

      }
      break;
    }

  if (voice_command("AT", "OK") != VMA_USER_1)
    return(FAIL);

  voice_modem_state = IDLE;
  voice_check_events();
  return(OK);
}

static int ISDN4Linux_stop_playing (void)
{
  stop_playing = TRUE;
  return(OK);
}

static int ISDN4Linux_stop_recording (void)
{
  char buffer[VOICE_BUF_LEN];

  stop_recording = TRUE;
  sprintf(buffer, "%c%c", DLE,DC4);
  lprintf(L_JUNK, "%s: <DLE> <DC4>", program_name);

  if (voice_write_raw(buffer, strlen(buffer)) != OK)
    return(FAIL);

  return(OK);
}

static int ISDN4Linux_voice_mode_off (void)
{
  reset_watchdog(0);

  if (voice_command("AT+FCLASS=0", "OK") != VMA_USER_1)
    return(FAIL);

  return(OK);
}

/* We are already in voice mode... so we just set AT+FCLASS=8 */
static int ISDN4Linux_voice_mode_on (void)
{
  reset_watchdog(0);

  if (voice_command("AT+FCLASS=8", "OK") != VMA_USER_1)
    return(FAIL);

  return(OK);
}

int ISDN4Linux_dial (char *number)
{
  int result;

  is_voicecall = 0;

  /* Set Service-Octet-1 to audio */
  if (voice_command("ATS18=1", "OK") != VMA_USER_1)
    return(FAIL);

  result = IS_101_dial (number);
  if (result == OK) is_voicecall = 1;
  return result;
}

int ISDN4Linux_wait (int wait_timeout)
{
  int ret;
  switch (voice_command("AT+VTX", "CONNECT|NO ANSWER"))
    {
    case VMA_USER_1:
      break;
    case VMA_USER_2:
      return(queue_event(create_event(NO_ANSWER)));
    default:
      return(FAIL);
    }
  got_DLE_DC4 = FALSE;
  ret=IS_101_wait (wait_timeout);
  if (!got_DLE_DC4)
    {
      char output_buffer[4 * ISDN4LINUX_BUFFER_SIZE];
      sprintf(output_buffer, "%c%c", DLE, ETX);
      lprintf(L_JUNK, "%s: <DLE> <ETX>", program_name);
      if (voice_write_raw(output_buffer, strlen(output_buffer)) != OK)
     return(FAIL);
    }

  if (voice_command("AT", "OK") != VMA_USER_1)
    return(FAIL);
  return(ret);
}


voice_modem_struct ISDN4Linux =
    {
    "Linux ISDN",
    "ISDN4Linux",
    &ISDN4Linux_answer_phone,
    &ISDN4Linux_beep,
    &ISDN4Linux_dial,
    &ISDN4Linux_handle_dle,
    &ISDN4Linux_init,
    &IS_101_message_light_off,
    &IS_101_message_light_on,
    &ISDN4Linux_play_file,
    &ISDN4Linux_record_file,
    &ISDN4Linux_set_compression,
    &ISDN4Linux_set_device,
    &IS_101_stop_dialing,
    &ISDN4Linux_stop_playing,
    &ISDN4Linux_stop_recording,
    &IS_101_stop_waiting,
    &IS_101_switch_to_data_fax,
    &ISDN4Linux_voice_mode_off,
    &ISDN4Linux_voice_mode_on,
    &ISDN4Linux_wait
    };
