/*
 * close.c
 *
 * Close the voice device.
 *
 */

#include "../include/voice.h"

char *libvoice_close_c = "$Id: close.c,v 1.2 1998/01/21 10:24:55 marc Exp $";

int voice_close_device _P0(void)
     {
     lprintf(L_MESG, "closing voice modem device");

     if (voice_fd == NO_VOICE_FD)
          {
          lprintf(L_WARN, "no voice modem device open");
          return(FAIL);
          };

     voice_flush(1);
     close(voice_fd);
     voice_fd = NO_VOICE_FD;
     rmlocks();
     return(OK);
     }
