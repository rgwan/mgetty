/*
 * command.c
 *
 * Execute the given command and wait for an answer.
 *
 */

#include "../include/voice.h"

char *libvoice_command_c = "$Id: command.c,v 1.2 1998/01/21 10:24:55 marc Exp $";

int voice_command _P2((command, expected_answer), char *command,
 char *expected_answer)
     {
     char buffer[VOICE_BUF_LEN];
     int result = VMA_FAIL;

     lprintf(L_NOISE, "voice command: '%s' -> '%s'", command,
      expected_answer);

     if (cvd.command_delay.d.i != 0)
          delay(cvd.command_delay.d.i);

     if (strlen(command) != 0)
          {

          if (voice_write("%s", command) != OK)
               {
               voice_flush(1);
               return(VMA_FAIL);
               };

          if (cvd.enable_command_echo.d.i)

               do
                    {

                    if (voice_read(buffer) != OK)
                         {
                         voice_flush(1);
                         return(VMA_FAIL);
                         };

                    result = voice_analyze(buffer, command);

                    if (result == VMA_FAIL)
                         {
                         errno = 0;
                         lprintf(L_ERROR,
                          "%s: Modem did not echo the command", program_name);
                         voice_flush(1);
                         return(VMA_FAIL);
                         };

                    if (result == VMA_ERROR)
                         {
                         errno = 0;
                         lprintf(L_ERROR, "%s: Modem returned ERROR",
                          program_name);
                         voice_flush(1);
                         return(VMA_FAIL);
                         };

                    }
               while (result != VMA_USER_1);

          result = OK;
          };

     if (strlen(expected_answer) != 0)
          {

          do
               {

               if (voice_read(buffer) != OK)
                    {
                    voice_flush(1);
                    return(VMA_FAIL);
                    };

               result = voice_analyze(buffer, expected_answer);

               if (result == VMA_FAIL)
                    {
                    errno = 0;
                    lprintf(L_ERROR, "%s: Invalid modem answer",
                     program_name);
                    voice_flush(1);
                    return(VMA_FAIL);
                    };

               if (result == VMA_ERROR)
                    {
                    errno = 0;
                    lprintf(L_ERROR, "%s: Modem returned ERROR",
                     program_name);
                    voice_flush(1);
                    return(VMA_FAIL);
                    };

               }
          while ((result & VMA_USER) != VMA_USER);

          };

     return(result);
     }
