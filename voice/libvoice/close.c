/*
 * close.c
 *
 * Close the voice device.
 *
 */

#include "../include/voice.h"

char *libvoice_close_c = "$Id: close.c,v 1.1 1997/12/16 12:21:07 marc Exp $";

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
