/*
 * flush.c
 *
 * Read input from the voice modem device until (timeout * 0.1 seconds)
 * have passed without a new character arrived.
 *
 */

#include "../include/voice.h"

char *libvocice_flush_c = "$Id: flush.c,v 1.1 1997/12/16 12:21:09 marc Exp $";

int voice_flush _P1((timeout), int timeout)
     {
     TIO tio;
     TIO save_tio;
     int first_char = TRUE;
     char char_read;

     tio_get(voice_fd, &tio);
     save_tio = tio;
     tio.c_lflag &= ~ICANON;
     tio.c_cc[VMIN] = 0;
     tio.c_cc[VTIME] = timeout;
     tio_set(voice_fd, &tio);

     while (read(voice_fd, &char_read, 1) == 1)

          if ((char_read == 0x0a) || (char_read == 0x0d))
               first_char = TRUE;
          else

               if (first_char)
                    {
                    first_char = FALSE;
                    lprintf(L_JUNK, "%s: %c", voice_modem_name, char_read);
                    }
               else
                    lputc(L_JUNK, char_read);

     tio_set(voice_fd, &save_tio);
     return(OK);
     }
