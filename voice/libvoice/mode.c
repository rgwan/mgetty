/*
 * mode.c
 *
 * Contains the functions voice_mode_on and voice_mode_off.
 *
 * $Id: mode.c,v 1.3 1998/03/25 23:05:45 marc Exp $
 *
 */

#include "../include/voice.h"

int voice_mode_on(void)
     {
     lprintf(L_NOISE, "%s: entering voice mode", program_name);
     tio_set(voice_fd, &voice_tio);
     voice_install_signal_handler();
     voice_modem->voice_mode_on();
     return(OK);
     }

int voice_mode_off(void)
     {
     lprintf(L_NOISE, "%s: leaving voice mode", program_name);
     voice_modem->voice_mode_off();
     voice_restore_signal_handler();
     tio_set(voice_fd, &tio_save);
     return(OK);
     }

int enter_fax_mode() 
     {
     char *fax_mode = NULL;
     int bit_order = 0;

     lprintf(L_JUNK, "%s: trying fax connection", program_name);

     if (modem_type == Mt_class2)
          {
          bit_order = 0;
          fax_mode = "2";
          }

     if (modem_type == Mt_class2_0)
          {
          bit_order = 1;
          fax_mode = "2.0";
          }

     if (voice_switch_to_data_fax(fax_mode) == FAIL)
          return(FAIL);

     if (voice_command("AT+FAA=0", "OK") != VMA_USER_1)
          return(FAIL);

     tio_set(voice_fd, &tio_save);
     fax_set_bor(voice_fd, bit_order);
     voice_restore_signal_handler();
     return(OK);
     }
