/* event.c
 *
 * This is the handle event routine for the VoiceModem program.
 *
 */

#include "vm.h"

char *vm_event_c = "$Id: event.c,v 1.1 1997/12/16 12:21:51 marc Exp $";

int handle_event _P2((event, data), int event, event_data data)
     {

     if ((use_on_hook_off_hook) && (event == HANDSET_OFF_HOOK) &&
      (voice_modem_state == WAITING))
          {
          voice_stop_waiting();
          start_recording = TRUE;
          };

     if ((use_on_hook_off_hook) && (event == HANDSET_ON_HOOK) &&
      (voice_modem_state == RECORDING))
          {
          voice_stop_recording();
          };

     if ((event == HANDSET_OFF_HOOK) || (event == HANDSET_ON_HOOK))
          return(OK);

     if (event == RECEIVED_DTMF)

          switch (dtmf_mode)
               {
               case IGNORE_DTMF:
                    return(OK);
               case READ_DTMF_DIGIT:
                    printf("%c\n", data.c);
                    voice_stop_current_action();
                    return(OK);
               case READ_DTMF_STRING:

                    if (data.c == '*')
                         {
                         dtmf_string_buffer[0] = 0x00;
                         return(OK);
                         };

                    if (data.c != '#')
                         {
                         int length = strlen(dtmf_string_buffer);

                         dtmf_string_buffer[length + 1] = 0x00;
                         dtmf_string_buffer[length] = data.c;
                         return(OK);
                         };

                    printf("%s\n", dtmf_string_buffer);
                    voice_stop_current_action();
                    return(OK);
               };

     if (event == SIGNAL_SIGINT)
          return(voice_stop_current_action());

     return(UNKNOWN_EVENT);
     }
