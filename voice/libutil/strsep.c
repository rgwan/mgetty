/*
 * strsep.c
 *
 * Parses a string into tokens. Should be equivalent to the Linux strsep function.
 *
 */

#include "../include/voice.h"

char *libutil_strsep_c = "$Id: strsep.c,v 1.2 1998/01/21 10:24:41 marc Exp $";

char *voice_strsep(char **stringp, const char *delim)
     {
     char *s = stringp[0];
     char *c = stringp[0];
     int i;

     if (*stringp == NULL)
          return(NULL);

     while (*c != '\000')
          {

          for (i = 0; i < strlen(delim); i++)

               if (*c == delim[i])
                    {
                    *c++ = '\000';
                    *stringp = c;
                    return(s);
                    };

          c++;
          }

     *stringp = NULL;
     return(s);
     }
