/*
 * detect.c
 *
 * autodetect the modemtype we are connected to.
 *
 */

#include "../include/voice.h"

char *libvoice_detect_c = "$Id: detect.c,v 1.1 1997/12/16 12:21:08 marc Exp $";

struct modem_type_struct
     {
     char *ati_code;
     voice_modem_struct *modem_type;
     };

static const struct modem_type_struct modem_database[] =
     {
     {"1.04", &Cirrus_Logic},
     {"144", &UMC},
     {"144 VOICE", &Rockwell},
     {"14400", &Rockwell},
     {"1443", &Dolphin},
     {"1445", &US_Robotics},
     {"1496", &ZyXEL_1496},
     {"248", &Sierra},
     {"249", &Rockwell},
     {"282", &Elsa},
     {"288", &ZyXEL_2864},
     {"2864", &ZyXEL_2864},
     {"28641", &ZyXEL_2864},
     {"28642", &ZyXEL_2864},
     {"28643", &ZyXEL_2864},
     {"28800", &Dr_Neuhaus},
     {"2886", &US_Robotics},
     {"3361", &US_Robotics},
     {"3366", &US_Robotics},
     {"33600", &Rockwell},
     {"3X WYSIWYF 628DBX", &Rockwell},
     {"Linux ISDN", &ISDN4Linux},
     {NULL, NULL}
     };

int voice_detect_modemtype _P0(void)
     {
     char buffer[VOICE_BUF_LEN];

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
          int i;

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
                    };

               if ((voice_command("", "OK|ATE1") & VMA_USER) == VMA_USER)
                    voice_flush(1);
               else
                    {
                    lprintf(L_FATAL, "modem detection failed");
                    exit(FAIL);
                    };

               }
          else
               {

               if (voice_write("ATE0") != OK)
                    {
                    lprintf(L_FATAL, "modem detection failed");
                    exit(FAIL);
                    };

               voice_flush(1);
               }

          /*
           * What does the modem answer to the ATI command?
           */

          voice_command("ATI", "");

          do
               {
               char *s;

               if (voice_read(buffer) != OK)
                    {
                    lprintf(L_FATAL, "modem detection failed");
                    exit(FAIL);
                    };

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

               for (i = 0; ((modem_database[i].ati_code != NULL) &&
                (voice_modem == &no_modem)); i++)

                    if (strcmp(s, modem_database[i].ati_code) == 0)
                         voice_modem = modem_database[i].modem_type;

               }
          while ((voice_modem == &no_modem) && (voice_analyze(buffer,
           "") != VMA_FAIL));

          voice_flush(1);
          };

     /*
      * Fix me! Ugly hack to distinguish between Dr. Neuhaus and
      * Rockwell modems reporting both 28800 on ATI
      */

     if ((voice_modem == &Dr_Neuhaus) &&
      (voice_command("ATI6", "OK|RCV288") == VMA_USER_2))
          {
          voice_flush(1);
          voice_modem = &Rockwell;
          };

     if (voice_modem->init == NULL)
          {
          errno = ENOSYS;
          lprintf(L_ERROR, "%s detected, but driver is not updated",
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
