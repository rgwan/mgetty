/*
 * init.c
 *
 * Initialize the open port to some sane defaults, detect the
 * type of voice modem connected and initialize the voice modem.
 *
 */

#include "../include/voice.h"

char *libvoice_init_c = "$Id: init.c,v 1.1 1997/12/16 12:21:10 marc Exp $";

int voice_init _P0(void)
     {
     TIO vtio;

     /*
      * initialize baud rate, software or hardware handshake, etc...
      */

     tio_get(voice_fd, &vtio);
     tio_mode_sane(&vtio, TRUE);

     if (tio_check_speed(cvd.port_speed.d.i) >= 0)
          {
          tio_set_speed(&vtio, cvd.port_speed.d.i);
          }
     else
          {
          lprintf(L_FATAL, "invalid port speed: %d", cvd.port_speed.d.i);
          close(voice_fd);
          rmlocks();
          exit(FAIL);
          }

     tio_default_cc(&vtio);
     tio_mode_raw(&vtio);
     tio_set_flow_control(voice_fd, &vtio, DATA_FLOW);
     vtio.c_cc[VMIN] = 0;
     vtio.c_cc[VTIME] = 5;

     if (tio_set(voice_fd, &vtio) == FAIL)
          {
          lprintf(L_FATAL, "error in tio_set");
          close(voice_fd);
          rmlocks();
          exit(FAIL);
          };

     if (voice_detect_modemtype() == OK)
          {
          voice_flush(1);

          if (voice_modem->init() == OK)
               return(OK);

          }

     close(voice_fd);
     rmlocks();
     return(FAIL);
     }
