/*
 * button.c
 *
 * Mgetty calls vgetty_button when it detects, that the Data/Voice button
 * on a ZyXEL modem was pressed.
 *
 * If the phone didn't ring, vgetty calls the button_program to play
 * the received voice messages. Otherwise vgetty leaves the voice mode
 * and tries a data or fax connection.
 *
 */

#include "../include/voice.h"

char *vgetty_button_c = "$Id: button.c,v 1.2 1998/01/21 10:25:30 marc Exp $";

void vgetty_button _P1((rings), int rings)
     {

     if ((rings == 0) && (strlen(cvd.button_program.d.p) != 0))
          {
          char file_name[VOICE_BUF_LEN];

          make_path(file_name, cvd.voice_dir.d.p,
           cvd.button_program.d.p);
          voice_install_signal_handler();
          voice_answer_phone();
          voice_execute_shell_script(file_name, NULL);
          exit(0);
          };

     voice_mode_off();
     }
