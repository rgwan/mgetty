/*
 * read.c
 *
 * Read data from the voice modem device.
 *
 */

#include "../include/voice.h"

char *libvoice_read_c = "$Id: read.c,v 1.1 1997/12/16 12:21:12 marc Exp $";

int voice_read _P1((buffer), char *buffer)
     {
     int char_read;
     int number_chars = 0;

     lprintf(L_JUNK, "%s: ", voice_modem_name);
     strcpy(buffer, "");

     do
          {

          if ((char_read = voice_read_char()) == FAIL)
               return(FAIL);

          if (char_read == DLE)
               {

               if ((char_read = voice_read_char()) == FAIL)
                    return(FAIL);

               lputs(L_JUNK, "<DLE> <");
               lputc(L_JUNK, char_read);
               lputc(L_JUNK, '>');
               voice_modem->handle_dle(char_read);
               lprintf(L_JUNK, "%s: ", voice_modem_name);
               return(OK);
               }
          else

               if ((char_read != NL) && (char_read != CR) &&
                (char_read != XON) && (char_read != XOFF))
                    {
                    *buffer = char_read;
                    buffer++;
                    number_chars++;
                    lputc(L_JUNK, char_read);
                    };

          }
     while (((char_read != NL) || (number_chars == 0)) &&
      (number_chars < (VOICE_BUF_LEN - 1)));

     *buffer = 0x00;
     return(OK);
     }

int voice_read_char(void)
     {
     time_t timeout;

     timeout = time(NULL) + cvd.port_timeout.d.i;

     while (timeout >= time(NULL))
          {
          char char_read;
          int result;

          result = read(voice_fd, &char_read, 1);

          if (result == 1)
               return(char_read);

          if ((result < 0) && (errno != 0) && (errno != EINTR))
               {
               lprintf(L_ERROR,
                "%s: could not read character from voice modem",
                program_name);
               return(FAIL);
               };

          };

     lprintf(L_ERROR, "%s: timeout while reading character from voice modem",
      program_name);
     return(FAIL);
     }

int voice_read_raw(char* buffer, int count)
     {
     int result;

     result = read(voice_fd, buffer, count);

     if ((result < 0) && (errno != 0) && (errno != EINTR))
          {
          lprintf(L_ERROR, "%s: could not read buffer from voice modem",
           program_name);
          return(FAIL);
          };

     if (result > 0)
          return(result);

     return(0);
     }
