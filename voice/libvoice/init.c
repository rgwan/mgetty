/*
 * init.c
 *
 * Initialize the open port to some sane defaults, detect the
 * type of voice modem connected and initialize the voice modem.
 *
 */

#include "../include/voice.h"

char *libvoice_init_c = "$Id: init.c,v 1.2 1998/01/21 10:24:58 marc Exp $";

TIO tio_save;
TIO voice_tio;

int voice_init _P0(void)
     {

     /*
      * initialize baud rate, software or hardware handshake, etc...
      */

     tio_get(voice_fd, &tio_save);
     tio_get(voice_fd, &voice_tio);
     tio_mode_sane(&voice_tio, TRUE);

     if (tio_check_speed(cvd.port_speed.d.i) >= 0)
          {
          tio_set_speed(&voice_tio, cvd.port_speed.d.i);
          }
     else
          {
          lprintf(L_FATAL, "invalid port speed: %d", cvd.port_speed.d.i);
          close(voice_fd);
          rmlocks();
          exit(FAIL);
          }

     tio_default_cc(&voice_tio);
     tio_mode_raw(&voice_tio);
     tio_set_flow_control(voice_fd, &voice_tio, DATA_FLOW);
     voice_tio.c_cc[VMIN] = 0;
     voice_tio.c_cc[VTIME] = 0;

     if (tio_set(voice_fd, &voice_tio) == FAIL)
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
