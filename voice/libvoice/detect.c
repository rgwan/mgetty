/*
 * detect.c
 *
 * autodetect the modemtype we are connected to.
 *
 * $Id: detect.c,v 1.19 2000/08/09 07:07:22 marcs Exp $
 *
 */

#include "../include/voice.h"

struct modem_type_struct
     {
     const char *at_cmnd;
     const char *at_answr;
     const char *next_cmnd;
     voice_modem_struct *modem_type;
     };

struct pnp_modem_type_struct
     {
     const char *pnpid;
     const char *modelid;
     voice_modem_struct *modem_type;
     };

static const struct pnp_modem_type_struct pnp_modem_database[] =
     {
     {"SUP", NULL, &Supra},
     {"ELS", "0687", &Elsa}, /* ML 56k DE */
     {"ELS", "0566", &Elsa}, /* ML 56k CH */
     {"ELS", "0707", &Elsa}, /* ML 56k AT */
     {"ELS", "8318", &Elsa}, /* ML 56k pro */
     {"ELS", "0853", &Elsa}, /* 1&1 Speedmaster pro */
     {"ELS", "8548", &Elsa}, /* ML Office */
     {"ELS", "0754", &Elsa}, /* ML 56k basic */
     {"ELS", "0350", &Elsa}, /* ML 56k internet */
     {"ELS", "0503", &Elsa}, /* ML 56k internet */
     {"ELS", "0667", &Elsa}, /* ML 56k internet */
     {"ELS", "0152", &Elsa}, /* ML 56k internet c */
     {"ELS", "0363", &V253modem}, /* ML 56k fun */
     {"ELS", "8550", &V253modem}, /* coming soon ... */
     {NULL, NULL, NULL}
     };
 

const char ati[] = "ATI";
const char ati6[] = "ATI6";
const char ati4[] = "ATI4";
const char ati9[] = "ATI9";
const char ati0[] = "ATI0";


static const struct modem_type_struct modem_database[] =
     {
     {ati, "1.04",                 NULL,   &Cirrus_Logic},
     {ati, "144",                  NULL,   &UMC},
     {ati, "144 VOICE",            NULL,   &Rockwell},
     {ati, "14400",                NULL,   &Rockwell},
     {ati, "1443",                 NULL,   &Dolphin},
     {ati, "1445",                 NULL,   &US_Robotics},
     {ati, "1496",                 NULL,   &ZyXEL_1496},
     {ati, "1500",                 NULL,   &ZyXEL_Omni56K},
     {ati, "1501",                 NULL,   &ZyXEL_Omni56K},
     {ati, "247",                  NULL,   &Multitech_2834ZDXv},
     {ati, "248",                  NULL,   &Sierra},
     {ati, "249",                  NULL,   &Rockwell},
     {ati, "282",                  NULL,   &Elsa},
     {ati, "288",                  NULL,   &ZyXEL_2864},
     {ati, "2864",                 NULL,   &ZyXEL_2864},
     {ati, "28641",                NULL,   &ZyXEL_2864},
     {ati, "28642",                NULL,   &ZyXEL_2864},
     {ati, "28643",                NULL,   &ZyXEL_2864},
     {ati, "28800",                ati6, NULL},
     {ati, "2886",                 NULL,   &US_Robotics},
     {ati, "336",                  NULL,   &Rockwell},
     {ati, "3361",                 NULL,   &US_Robotics},
     {ati, "3362",                 NULL,   &US_Robotics},
     {ati, "3366",                 NULL,   &US_Robotics},
#if 0
     {ati, "33600",                NULL,   &Rockwell},
#else
     /* This could break Rockwell modems, but is needed for some
      * Neuhaus variants (Smarty). We keep it visible for some time in case.
      */
     {ati, "33600",                ati6, NULL},
#endif
     {ati, "3X WYSIWYF 628DBX",    NULL,   &Rockwell},
     {ati, "56000",                NULL,   &Rockwell},
     {ati, "5601",                 NULL,   &US_Robotics},
     {ati, "961",                  NULL,   &Rockwell},
     {ati, "Digi RAS modem 56000", NULL,   &Digi_RAS},
     {ati, "Linux ISDN",           NULL,   &ISDN4Linux},
     {ati, "MT5600ZDXV",           NULL,   &Multitech_5600ZDXv},
     {ati, "LT V.90 1.0 MT5634ZBAV Serial Data/Fax/Voice Modem Version 4.09a",
                                   NULL,   &Multitech_5634ZBAV},
     {ati4, "33600bps Voice Modem For Italy",
                                   NULL, &Rockwell},
     {ati6, "RCV336DPFSP Rev 44BC",
                                   NULL, &Rockwell},
     {ati, "ERROR", ati0, NULL}, /* it also shows up as North America,
                                  *  then OK in ATI9. Please also read
                                  * libvoice/README.lucent.
                                  */

     {ati6, "OK",      NULL, &Dr_Neuhaus},
     {ati6, "RCV288DPi Rev 05BA",  NULL,   &Rockwell},
     {ati6, "RCV288*", NULL, &Rockwell},

#if 0 /* Please read libvoice/README.lucent */
     {ati0, "ZOOM*", NULL, &Lucent},
#endif

     {ati4, "WS-3314JS3", NULL, &Rockwell},

     {NULL, NULL, NULL, NULL}
     };

int voice_detect_modemtype(void)
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
          int i;
          char *s;

          /*
           * First of all, let's see if a modem is connected and answering
           * and also initialize the modem echo command correctly.
           */

          if (cvd.enable_command_echo.d.i)
               {

               if (voice_write("ATE1") != OK)
                    {
                    lprintf(L_WARN, "modem detection failed");
                    exit(FAIL);
                    }

               if ((voice_command("", "OK|ATE1") & VMA_USER) == VMA_USER)
                    voice_flush(1);
               else
                    {
                    lprintf(L_WARN, "modem detection failed");
                    exit(FAIL);
                    }

               }
          else
               {

               if (voice_write("ATE0") != OK)
                    {
                    lprintf(L_WARN, "modem detection failed");
                    exit(FAIL);
                    }

               voice_flush(3);
               }

          /*
           * Let's detect the voice modem type.
           */

	  /* Let's try plug and play (Rob Ryan <rr2b@pacbell.net>) */

	  cmnd=(char *)ati9;
	  if (voice_command(cmnd, "") != OK)
               {
               lprintf(L_WARN, "modem detection failed");
               exit(FAIL);
               }
	  if (voice_read(buffer) != OK)
                    {
                    lprintf(L_WARN, "modem detection failed");
                    exit(FAIL);
	  }
	  s = strchr(buffer, '(');
	  if (s && s[1] > 0 )
	  	{
	  	s+=3;
                lprintf(L_NOISE, "%s", s);
	  	i = 0;
	  	while (voice_modem == &no_modem &&
                       pnp_modem_database[i].pnpid)
	           {
                        if (pnp_modem_database[i].pnpid) {
			   lprintf(L_JUNK, "checking pnpipd %s",
					   pnp_modem_database[i].pnpid);
			   if (strncmp(pnp_modem_database[i].pnpid, s, 3)
                               == 0) {
			      lprintf(L_JUNK, "checking modelid %s",
					      pnp_modem_database[i].modelid);
			      if (pnp_modem_database[i].modelid == NULL ||
				  strncmp(pnp_modem_database[i].modelid,
					  s+3,
					  4) == 0)
				   {
				   voice_modem =
					   pnp_modem_database[i].modem_type;
				   break;
				   }
			   }
                        }
			i++;
	      	   }
	     	 /* eat the OK... */
	         voice_read(buffer);
	  	}

	  voice_flush(3);

	  if (voice_modem != &no_modem)
	  	{
          	lprintf(L_NOISE, "voice modem type was set by pnp id");
         	 return(OK);
         	}

          cmnd = (char *) ati;

          if (voice_command(cmnd, "") != OK)
               {
               lprintf(L_WARN, "modem detection failed");
               exit(FAIL);
               }

          do
               {
               if (voice_read(buffer) != OK)
                    {
                    lprintf(L_WARN, "modem detection failed");
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
                                   lprintf(L_WARN, "modem detection failed");
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
          while ((voice_modem == &no_modem) &&
           (voice_analyze(buffer, "", TRUE) != VMA_FAIL));

          voice_flush(1);
          }

     if (voice_modem->init == NULL)
          {
          lprintf(L_WARN, "%s detected, but driver is not available",
           voice_modem->name);
          voice_modem = &no_modem;
          exit(FAIL);
          };

     if (voice_modem == &no_modem) {
        /* Supports the modem V253 commands? */
        voice_command("AT+FCLASS=8", "OK");
        if (voice_command("AT+VSM=1,8000", "OK")== VMA_USER) {
           voice_modem=&V253modem;
           /* if the modem answers with ok then it supports ITU V253 commands
            * and compression mode 8 bit PCM = nocompression.
            */
        }
        voice_command("AT+FCLASS=0", "OK"); /* back to normal */
     }

     if (voice_modem != &no_modem)
          {
          lprintf(L_MESG, "%s detected", voice_modem->name);
          return(OK);
          };

     voice_flush(1);
     voice_modem = &no_modem;
     lprintf(L_WARN, "no voice modem detected");
     exit(FAIL);
     }
