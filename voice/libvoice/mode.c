/*
 * mode.c
 *
 * Contains the functions voice_mode_on and voice_mode_off.
 *
 */

#include "../include/voice.h"

char *libvoice_mode_c = "$Id: mode.c,v 1.1 1997/12/16 12:21:10 marc Exp $";

int voice_mode_on _P0(void)
     {
     lprintf(L_NOISE, "%s: entering voice mode", program_name);
     voice_install_signal_handler();
     voice_modem->voice_mode_on();
     return(OK);
     }

int voice_mode_off _P0(void)
     {
     lprintf(L_NOISE, "%s: leaving voice mode", program_name);
     voice_modem->voice_mode_off();
     voice_restore_signal_handler();
     return(OK);
     }


int enter_fax_mode() {
  char *fax_mode = NULL;
  int bit_order = 0;

  lprintf(L_JUNK, "%s: trying fax connection", program_name);

  if (modem_type == Mt_class2) {
    bit_order = 0;
    fax_mode = "2";
  };

  if (modem_type == Mt_class2_0) {
    bit_order = 1;
    fax_mode = "2.0";
  };

  if (voice_switch_to_data_fax(fax_mode) == FAIL)
                    return(FAIL);

  if (voice_command("AT+FAA=0", "OK") != VMA_USER_1)
                    return(FAIL);

  fax_set_bor(voice_fd, bit_order);

  voice_restore_signal_handler();

  return(OK);
}

