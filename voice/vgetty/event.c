/*
 * event.c
 *
 * Here is the vgetty specific voice event handler.
 *
 */

#include "../include/voice.h"

char *vgetty_event_c = "$Id: event.c,v 1.1 1997/12/16 12:21:48 marc Exp $";

int vgetty_handle_event _P2((event, data), int event, event_data data)
     {

     switch (event)
          {
          case HANDSET_ON_HOOK:
               return(OK);
          case BUSY_TONE:
          case DIAL_TONE:
          case HANDSET_OFF_HOOK:
          case SIGNAL_SIGHUP:
          case SIGNAL_SIGINT:
          case SIGNAL_SIGQUIT:
          case SIGNAL_SIGTERM:
               voice_stop_current_action();
               hangup_requested = TRUE;
               return(OK);
          case DATA_CALLING_TONE:
               voice_stop_current_action();
               answer_mode = answer_mode & ANSWER_DATA;
               switch_to_data_fax_mode = TRUE;
               return(OK);
          case FAX_CALLING_TONE:
               voice_stop_current_action();
               answer_mode = answer_mode & ANSWER_FAX;
               switch_to_data_fax_mode = TRUE;
               return(OK);
          case NO_VOICE_ENERGY:
               voice_stop_current_action();
               answer_mode = answer_mode & (ANSWER_DATA | ANSWER_FAX);
               switch_to_data_fax_mode = TRUE;
               return(OK);
          case RECEIVED_DTMF:
               {
               int length;

               if (data.c == '*')
                    {
                    first_dtmf = FALSE;
                    dtmf_code[0] = 0x00;

                    if (voice_modem_state == PLAYING)
                         voice_stop_playing();

                    return(OK);
                    };

               if (data.c == '#')
                    {
                    first_dtmf = FALSE;
                    voice_stop_current_action();

                    if (strlen(dtmf_code) == 0)
                         hangup_requested = TRUE;
                    else
                         execute_dtmf_script = TRUE;

                    return(OK);
                    };

               if ((first_dtmf) && (voice_modem_state == PLAYING))
                    {
                    first_dtmf = FALSE;
                    voice_stop_playing();
                    answer_mode = answer_mode & (ANSWER_DATA | ANSWER_FAX);
                    switch_to_data_fax_mode = TRUE;
                    return(OK);
                    };

               first_dtmf = FALSE;
               execute_dtmf_script = TRUE;
               length = strlen(dtmf_code);
               dtmf_code[length + 1] = 0x00;
               dtmf_code[length] = data.c;
               return(OK);
               };
          case SILENCE_DETECTED:
               voice_stop_current_action();
               return(OK);
          };

     return(UNKNOWN_EVENT);
     }
