/*
 * message.c
 *
 * Some external modems have a auto-answer LED. This LED is used
 * by vgetty to indicate that new voice messages have arrived.
 *
 * If a new message arrives, the file with the name stored in
 * cvd.message_flag_file, is created. After listening to the new
 * messages, the file is again removed.
 *
 */

#include "../include/voice.h"

char *vgetty_message_c = "$Id: message.c,v 1.2 1998/01/21 10:25:32 marc Exp $";

void vgetty_message_light _P0(void)
     {

     if (cvd.do_message_light.d.p && (strlen(cvd.message_flag_file.d.p) != 0))
          {
          char flag_file_name[VOICE_BUF_LEN];
          FILE *flag_file;

          make_path(flag_file_name, cvd.voice_dir.d.p,
           cvd.message_flag_file.d.p);
          lprintf(L_JUNK, "%s: checking for message flag file %s",
           program_name, flag_file_name);
          flag_file = fopen(flag_file_name, "r");

          if (flag_file == NULL)
               voice_message_light_off();
          else
               {
               fclose(flag_file);
               voice_message_light_on();
               };

          };

     }

void vgetty_create_message_flag_file _P0(void)
     {

     if (strlen(cvd.message_flag_file.d.p) != 0)
          {
          char flag_file_name[VOICE_BUF_LEN];
          int flag_file;

          make_path(flag_file_name, cvd.voice_dir.d.p,
           cvd.message_flag_file.d.p);
          lprintf(L_JUNK, "%s: creating message flag file %s",
           program_name, flag_file_name);
          flag_file = creat(flag_file_name, 0666);

          if (flag_file < 0)
               lprintf(L_WARN, "%s: could not creat message flag file %s",
                program_name, flag_file_name);
          else
               close(flag_file);

          };

     }
