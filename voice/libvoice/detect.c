/*
 * detect.c
 *
 * autodetect the modemtype we are connected to.
 *
 */

#include "../include/voice.h"

char *libvoice_detect_c = "$Id: detect.c,v 1.2 1998/01/21 10:24:56 marc Exp $";

struct modem_type_struct
     {
     const char *at_cmnd;
     const char *at_answr;
     const char *next_cmnd;
     voice_modem_struct *modem_type;
     };

const char ati[] = "ATI";
const char ati6[] = "ATI6";

static const struct modem_type_struct modem_database[] =
     {
     {ati, "1.04",              NULL,   &Cirrus_Logic},
     {ati, "144",               NULL,   &UMC},
     {ati, "144 VOICE",         NULL,   &Rockwell},
     {ati, "14400",             NULL,   &Rockwell},
     {ati, "1443",              NULL,   &Dolphin},
     {ati, "1445",              NULL,   &US_Robotics},
     {ati, "1496",              NULL,   &ZyXEL_1496},
     {ati, "247",               NULL,   &Multitech_2834ZDXv},
     {ati, "248",               NULL,   &Sierra},
     {ati, "249",               NULL,   &Rockwell},
     {ati, "282",               NULL,   &Elsa},
     {ati, "288",               NULL,   &ZyXEL_2864},
     {ati, "2864",              NULL,   &ZyXEL_2864},
     {ati, "28641",             NULL,   &ZyXEL_2864},
     {ati, "28642",             NULL,   &ZyXEL_2864},
     {ati, "28643",             NULL,   &ZyXEL_2864},
     {ati, "28800",             "ATI6", NULL},
     {ati, "2886",              NULL,   &US_Robotics},
     {ati, "336",               NULL,   &Rockwell},
     {ati, "3361",              NULL,   &US_Robotics},
     {ati, "3366",              NULL,   &US_Robotics},
     {ati, "33600",             NULL,   &Rockwell},
     {ati, "3X WYSIWYF 628DBX", NULL,   &Rockwell},
     {ati, "56000",             NULL,   &Rockwell},
     {ati, "5601",              NULL,   &US_Robotics},
     {ati, "Linux ISDN",        NULL,   &ISDN4Linux},

     {ati6, "OK",     NULL, &Dr_Neuhaus},
     {ati6, "RCV288", NULL, &Rockwell},

     {NULL, NULL, NULL, NULL}
     };

int voice_detect_modemtype _P0(void)
     {
     char buffer[VOICE_BUF_LEN];
     char *cmnd;

     lprintf(L_MESG, "detecting voice modem type");

     /*
      * Do we have to probe for a voice modem or was it preset?
      */

     if (voice_modem != &no_modem)
          {
          lprintf(L_NOISE, "voice modem type was set directly");
          return(OK);
          }
     else
          {

          /*
           * First of all, let's see if a modem is connected and answering
           * and also initialize the modem echo command correctly.
           */

          if (cvd.enable_command_echo.d.i)
               {

               if (voice_write("ATE1") != OK)
                    {
                    lprintf(L_FATAL, "modem detection failed");
                    exit(FAIL);
                    }

               if ((voice_command("", "OK|ATE1") & VMA_USER) == VMA_USER)
                    voice_flush(1);
               else
                    {
                    lprintf(L_FATAL, "modem detection failed");
                    exit(FAIL);
                    }

               }
          else
               {

               if (voice_write("ATE0") != OK)
                    {
                    lprintf(L_FATAL, "modem detection failed");
                    exit(FAIL);
                    }

               voice_flush(1);
               }

          /*
           * Let's detect the voice modem type.
           */

          cmnd = (char *) ati;

          if (voice_command(cmnd, "") != OK)
               {
               lprintf(L_FATAL, "modem detection failed");
               exit(FAIL);
               }

          do
               {
               char *s;
               int i;

               if (voice_read(buffer) != OK)
                    {
                    lprintf(L_FATAL, "modem detection failed");
                    exit(FAIL);
                    }

               /*
                * Strip off leading and trailing whitespaces and tabs
                */

               s = buffer + strlen(buffer) - 1;

               while ((s >= buffer) && ((*s == ' ') || (*s == '\t')))
                    *s-- = '\0';

               s = buffer;

               for (s = buffer; ((s <= buffer + strlen(buffer) - 1) &&
                ((*s == ' ') || (*s == '\t'))); s++)
                    ;

               for (i = 0; ((modem_database[i].at_cmnd != NULL) &&
                (voice_modem == &no_modem)); i++)
                    {

                    if ((strcmp(modem_database[i].at_cmnd, cmnd) == 0) &&
                     (strcmp(modem_database[i].at_answr, s) == 0))
                         {

                         if (modem_database[i].next_cmnd != NULL)
                              {
                              voice_flush(1);
                              cmnd = (char *) modem_database[i].next_cmnd;

                              if (voice_command(cmnd, "") != OK)
                                   {
                                   lprintf(L_FATAL, "modem detection failed");
                                   exit(FAIL);
                                   }

                              sprintf(buffer, "OK");
                              break;
                              }
                         else
                              voice_modem = modem_database[i].modem_type;

                         }

                    }

               }
          while ((voice_modem == &no_modem) && (voice_analyze(buffer, "") !=
           VMA_FAIL));

          voice_flush(1);
          }

     if (voice_modem->init == NULL)
          {
          errno = ENOSYS;
          lprintf(L_ERROR, "%s detected, but driver is not available",
           voice_modem->name);
          voice_modem = &no_modem;
          exit(FAIL);
          };

     if (voice_modem != &no_modem)
          {
          lprintf(L_MESG, "%s detected", voice_modem->name);
          return(OK);
          };

     voice_flush(1);
     voice_modem = &no_modem;
     lprintf(L_ERROR, "no voice modem detected");
     exit(FAIL);
     }
