/*
 * flush.c
 *
 * Read input from the voice modem device until (timeout * 0.1 seconds)
 * have passed without a new character arrived.
 *
 */

#include "../include/voice.h"

char *libvocice_flush_c = "$Id: flush.c,v 1.2 1998/01/21 10:24:58 marc Exp $";

void voice_flush(int timeout)
     {
     int modem_byte;
     int first_char = TRUE;

     do
          {

          while ((modem_byte = voice_read_byte()) >= 0)
               {

               if ((modem_byte == 0x0a) || (modem_byte == 0x0d))
                    first_char = TRUE;
               else

                    if (first_char)
                         {
                         first_char = FALSE;
                         lprintf(L_JUNK, "%s: %c", voice_modem_name, modem_byte);
                         }
                    else
                         lputc(L_JUNK, modem_byte);

               }

          if (timeout > 0)
               delay(100 * timeout);

          }
     while (voice_check_for_input());

     }
