/*
 * V253modem.c
 *
 * This file contains the commands for V253 complaient modems
 *
  For this File I used the ELSA.c as template and replaced all pre-V.253 commands
  with the commands defined in the ITU V.253 specification.
  Newer ELSA-modems follow this specification.
  So some ELSA-modems like the "ML 56k pro" could be used with the ELSA.c (and the old commands)
  AND with this V253modem (But for these modems you should set sample rate in your voice.conf to 7200
  otherwise fax/data-callingtonedetection will fail).
  The "ML 56k FUN" and future ELSA-modems work only with this V253modem.

  Because there are only V.253 commands here, this IS a GENERIC-DRIVER!

  Hint: Recorded voice files are in .ub format (refer to the sox manpage about this) except the header.
        So you can use this files with sox.
 *
 * $Id: V253modem.c,v 1.2 2000/09/09 08:49:34 marcs Exp $
 *
 */

#include "../include/voice.h"

static int V253modem_set_device (int device);

static int V253modem_init (void)
     {
     char buffer[VOICE_BUF_LEN];

     reset_watchdog();
     voice_modem_state = INITIALIZING;
     lprintf(L_MESG, "initializing V253 voice modem");


     /* enabling voicemode */
     sprintf(buffer, "AT+FCLASS=8");
     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "could not set +FCLASS=8");


     /* set silence-sensitvity-level and silence length */
/*
first value of the +vsd command means:
 0: silence detection of/ or silencecompression with the +vsm command
128: manufacturer default
<128: less aggressive [more sensitive, lower noise levels considered to
be silence].
>128: more aggressive [less sensitive, higher noise levels considered to be silence].

 */
#if 0 /* enable this when cvd.rec_silence_threshold.d.i  is set as an absoult value
(with default 128) instead of percent */
     sprintf(buffer, "AT+VSD=%d,%d", cvd.rec_silence_threshold.d.i , cvd.rec_silence_len.d.i);
#else /* until this, the sensitvity is harcoded with manufaturer default! */
    sprintf(buffer, "AT+VSD=%d,%d", 128 , cvd.rec_silence_len.d.i);
#endif
     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set silence threshold VSD");

/* set transmit-gain manufacturer default is 128 so the vgetty default of 127 isn't far away */
     if (cvd.transmit_gain.d.i == -1)
          cvd.transmit_gain.d.i = 50;

     sprintf(buffer, "AT+VGT=%d", cvd.transmit_gain.d.i * 255 / 100 );

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set speaker volume");

/* set recieve-gain manufacturer default is 128 so the vgetty default of 127 isn't far away */

     if (cvd.receive_gain.d.i == -1)
          cvd.receive_gain.d.i = 50;

     sprintf(buffer, "AT+VGR=%d", cvd.receive_gain.d.i * 255 / 100 );

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "can't set record volume");

     /* Delay after ringback or before any ringback
      * before modem assumes phone has been answered.
      */
     sprintf(buffer,
             "AT+VRA=%d;+VRN=%d",
             cvd.ringback_goes_away.d.i,     /* 1/10 seconds */
             cvd.ringback_never_came.d.i/10); /* seconds */

     if (voice_command(buffer, "OK") != VMA_USER_1)
          lprintf(L_WARN, "setting ringback delay didn't work");


/* set hardflow */
     if ((cvd.do_hard_flow.d.i) && (voice_command("AT+IFC=2,2", "OK") ==
      VMA_USER_1) )
          {
          TIO tio;
          tio_get(voice_fd, &tio);
          tio_set_flow_control(voice_fd, &tio, FLOW_HARD);
          tio_set(voice_fd, &tio);
          }
     else
          lprintf(L_WARN, "can't turn on hardware flow control");


     /* enable callerid (if supported) and setformat */
     voice_command("AT+VCID=1", "OK");
     /* mgetty supports formated callerid output */

     /* set hangupcontroll */
     voice_command("AT+VNH=0", "OK");
     /* this means modem will hangup after is has switched to data/faxmode as normal
        with +VNH=1 the modem doesn't make an automatic disconnect if connect fails in
	data/faxmode so mgetty could switch back to voicemode */

     /* set inactivity timer to 60seconds */
     voice_command("AT+VIT=60", "OK");

     /* enable distinctivering in pulse/pauseformat
       this will look like this:
DRON=20
RING
DROF=40
DRON=20
RING */
     voice_command("AT+VDR=1,5", "OK");

     voice_modem_state = IDLE;
     return(OK);
     }

static int V253modem_set_compression (int *compression, int *speed, int *bits)
     {
     char buffer[VOICE_BUF_LEN];
     int Kompressionmethod; /* a littlebit germinglish :-) */
     reset_watchdog();

     switch (*compression)
     {
       case 0:
       case 1:
       case 8:
       {
          Kompressionmethod = 1;
          *bits=8;
          break;
       }
     /*  default is 8 Bit PCM/8 bit linear which should be supported by most modems
         and be mapped to the V.253 defined AT+VSM=1 .
         On the otherside the the compressionmodes from the Elsa.c
         are mapped to the manufacturer specific +VSM values (above 127)
         so voice files recorded with the &Elsa driver can be played with
         this &V253modem driver (and the same modem of course) */
       case 2:       /*  2bit ADPCM for some ELSA-modems */
       {
         *bits = 2;
         if (voice_command("AT+VSM=140,7200", "OK")!= VMA_USER_1)
         {
         /* there are two diffrent implementations trying one first,
            if this fails we try the other one later */
           Kompressionmethod = 129;
         }
         *speed=7200;
         break;
        }
       case 4:           /* 4bit ADPCM for some ELSA-modems */
       {
         if (voice_command("AT+VSM=141,7200", "OK")!= VMA_USER_1)
         {
         /* there are two diffrent implementations trying one first,
            if this fails we try the other one later */

           Kompressionmethod = 131;
         }
         *bits=4;
         *speed=7200;
         break;
       }
       case 5:
       {
         Kompressionmethod = 129;
         *bits = 4;      /* 129 ->4bit ADPCM for the ML 56k Fun*/
         break;
       }
       case 6:
       {
         Kompressionmethod = 131;
         *bits = 8;      /* 8bit uLAW for the ML 56k Fun*/
         *speed=8000;
         break;
       }
       case 7:
       {
         Kompressionmethod = 132;
         *bits = 8;      /* 8bit aLAW for the ML 56k Fun*/
         *speed=8000;
         break;
       }
       case 9:        /* ITU defined unsigned PCM */
       {
          Kompressionmethod = 0;
          *bits=8;
          break;
       }
       case 10:        /* ITU defined uLaw */
       {
          Kompressionmethod = 4;
          *bits=8;
          break;
       }
       case 11:        /* ITU defined aLaw */
       {
          Kompressionmethod = 5;
          *bits=8;
          break;
       }


       default:
       {
          lprintf(L_WARN, "compression method %d is not supported -> edit voice.conf",*compression);
        /*  return(FAIL);  */
          Kompressionmethod = 1;
       }
     }
     if (*speed == 0)
          *speed = 8000; /* default value for the PCM voiceformat is 8000 */

     sprintf(buffer, "AT+VSM=%d,%d",Kompressionmethod, *speed); /* only no compression is common! */
       /* ATTENTION the AT+VSM=? output is diffrent from the AT+VSM=<Parameter> */
     if (voice_command(buffer, "OK")!= VMA_USER_1)
      {
         lprintf(L_WARN, "can't set compression and speed");
         /*return(FAIL);*/   /* when we don't say FAIL here,
           the modem can still record the message with its
           last/default compression-setting */
         voice_command("AT+VSM?", "OK");
         /* write the actual setting to the logfile */
      }

     lprintf(L_NOISE, "Just for info: port_speed should be greater than %d bps",(*bits)*(*speed)*10/8);
     /* 8 Databits + 1 Stopbit +1 startbit, generel: (*bits)*(*speed)*9/8,  */
     return(OK) ;
     }

static int V253modem_set_device (int device)
     {
     reset_watchdog();

     switch (device)
          {
          case NO_DEVICE:
               lprintf(L_JUNK, "%s: _NO_DEV: (%d)", voice_modem_name, device);
               if (voice_command("AT+VLS=0", "OK|VCON")!= VMA_USER_1)   /* valid answer for +vls is only "OK" but to keep this more generic... */
               {
                 lprintf(L_WARN, "can't set No_Device");
                 /* I don't know what returnvalue should be used in this case of failure */
               }
               return(OK);
          case DIALUP_LINE:
               lprintf(L_JUNK, "%s: _DIALUP: (%d)", voice_modem_name, device);
               if (voice_command("AT+VLS=1", "OK")!= VMA_USER_1)
               {
                 lprintf(L_WARN, "can't set No_Device");
                 /* I don't know what returnvalue should be used in this case of failure */
               }
               return(OK);
          case EXTERNAL_MICROPHONE:
               lprintf(L_JUNK, "%s: _External_Microphone: (%d)", voice_modem_name, device);
               if (voice_command("AT+VLS=11", "OK")!= VMA_USER_1)
               {
                 lprintf(L_WARN, "can't set External_Microphone");
                 /* I don't know what returnvalue should be used in this case of failure */
               }
               return(OK);
          case INTERNAL_MICROPHONE:
               lprintf(L_JUNK, "%s: _INT_MIC: (%d)", voice_modem_name, device);
               if (voice_command("AT+VLS=6", "OK|VCON")!= VMA_USER_1)
               {
                 lprintf(L_WARN, "can't set Internal_Microphone");
                 /* I don't know what returnvalue should be used in this case of failure */
               }
               return(OK);
          case INTERNAL_SPEAKER:
               lprintf(L_JUNK, "%s: _INT_SEAK: (%d)", voice_modem_name, device);
               if (voice_command("AT+VLS=4", "OK|VCON")!= VMA_USER_1)
               {
                 lprintf(L_WARN, "can't set INTERNAL_SPEAKER");
                 /* I don't know what returnvalue should be used in this case of failure */
               }
               return(OK);
          case EXTERNAL_SPEAKER:
               lprintf(L_JUNK, "%s: _EXTERNAL_SPEAKER: (%d)", voice_modem_name, device);
               if (voice_command("AT+VLS=8", "OK|VCON")!= VMA_USER_1)
               {
                 lprintf(L_WARN, "can't set External_SPEAKER");
                 /* I don't know what returnvalue should be used in this case of failure */
               }
               return(OK);
          case LOCAL_HANDSET :
               lprintf(L_JUNK, "%s: _LOCAL_HANDSET: (%d)", voice_modem_name, device);
               if (voice_command("AT+VLS=2", "OK|VCON") != VMA_USER_1)
               {
                 lprintf(L_WARN, "can't set LOCAL_HANDSET");
                 /* I don't know what returnvalue should be used in this case of failure */
               }
               return(OK);
          case DIALUP_WITH_EXT_SPEAKER :
               lprintf(L_JUNK, "%s: _DIALUP_WITH_EXT_SPEAKER: (%d)", voice_modem_name, device);
               if (voice_command("AT+VLS=9", "OK|VCON") != VMA_USER_1)
               {
                 lprintf(L_WARN, "can't set DIALUP_WITH_EXT_SPEAKER");
                 /* I don't know what returnvalue should be used in this case of failure */
               }
               return(OK);


          case DIALUP_WITH_INT_SPEAKER :
               lprintf(L_JUNK, "%s: _DIALUP_WITH_INT_SPEAKER: (%d)", voice_modem_name, device);
               if (voice_command("AT+VLS=5", "OK|VCON") != VMA_USER_1)
               {
                 lprintf(L_WARN, "can't set DIALUP_WITH_INT_SPEAKER");
                 /* I don't know what returnvalue should be used in this case of failure */
               }
               return(OK);

          case DIALUP_WITH_LOCAL_HANDSET :
               lprintf(L_JUNK, "%s: _DIALUP_WITH_LOCAL_HANDSET: (%d)", voice_modem_name, device);
               if (voice_command("AT+VLS=3", "OK|VCON") != VMA_USER_1)
               {
                 lprintf(L_WARN, "can't set DIALUP_WITH_LOCAL_HANDSET");
                 /* I don't know what returnvalue should be used in this case of failure */
               }
               return(OK);

          case DIALUP_WITH_EXTERNAL_MIC_AND_SPEAKER:
               lprintf(L_JUNK, "%s: _DIALUP_WITH_EXTERNAL_MIC_AND_External_SPEAKER: (%d)", voice_modem_name, device);
               if (voice_command("AT+VLS=13", "OK|VCON") != VMA_USER_1)
               {
                 lprintf(L_WARN, "can't set DIALUP_WITH_EXTERNAL_MIC_AND_External_SPEAKER");
                 /* I don't know what returnvalue should be used in this case of failure */
               }
               return(OK);

          case DIALUP_WITH_INTERNAL_MIC_AND_SPEAKER:
               lprintf(L_JUNK, "%s: _DIALUP_WITH_INTERNAL_MIC_AND_Internal_SPEAKER: (%d)", voice_modem_name, device);
               if (voice_command("AT+VLS=7", "OK|VCON") != VMA_USER_1)
               {
                 lprintf(L_WARN, "can't set DIALUP_WITH_INTERNAL_MIC_AND_Internal_SPEAKER");
                 /* I don't know what returnvalue should be used in this case of failure */
               }
               return(OK);

          }

     lprintf(L_WARN, "%s: Unknown device (%d)", voice_modem_name, device);
     return(FAIL);
     }

/* Only verifies the RMD name */
#define V253modem_RMD_NAME "V253modem"
#define ELSA_RMD_NAME "ELSA"
int V253_check_rmd_adequation(char *rmd_name) {
   /* We use hardware values so that this function can be
    * inherited from 2864 too.
    */
   return !strncmp(rmd_name,
                   V253modem_RMD_NAME,
                   sizeof(V253modem_RMD_NAME))
          || !strncmp(rmd_name,
                      ELSA_RMD_NAME,
                      sizeof(ELSA_RMD_NAME));
}








static char V253modem_pick_phone_cmnd[] = "AT+FCLASS=8";  /* because this will be followed by a
                                                             V253modem_set_device (DIALUP_LINE)
                                                             -> this picks up the line!*/
static char V253modem_pick_phone_answr[] = "VCON|OK";


static char V253modem_hardflow_cmnd[] = "AT+IFC=2,2";
static char V253modem_softflow_cmnd[] = "AT+IFC=1,1";

static char V253modem_beep_cmnd[] = "AT+VTS=[%d,,%d]";


voice_modem_struct V253modem =
    {
    "V253 modem",
    V253modem_RMD_NAME,
     (char *) V253modem_pick_phone_cmnd,
     (char *) V253modem_pick_phone_answr,
     (char *) V253modem_beep_cmnd,
     (char *) IS_101_beep_answr,
              IS_101_beep_timeunit,
     (char *) V253modem_hardflow_cmnd,
     (char *) IS_101_hardflow_answr,
     (char *) V253modem_softflow_cmnd,
     (char *) IS_101_softflow_answr,
     (char *) IS_101_start_play_cmnd,
     (char *) IS_101_start_play_answer,
     (char *) IS_101_reset_play_cmnd,
     (char *) IS_101_intr_play_cmnd,
     (char *) IS_101_intr_play_answr,
     (char *) IS_101_stop_play_cmnd,
     (char *) IS_101_stop_play_answr,
     (char *) IS_101_start_rec_cmnd,
     (char *) IS_101_start_rec_answr,
     (char *) IS_101_stop_rec_cmnd,
     (char *) IS_101_stop_rec_answr,
     (char *) IS_101_switch_mode_cmnd,
     (char *) IS_101_switch_mode_answr,
     (char *) IS_101_ask_mode_cmnd,
     (char *) IS_101_ask_mode_answr,
     (char *) IS_101_voice_mode_id,
     (char *) IS_101_play_dtmf_cmd,
     (char *) IS_101_play_dtmf_extra,
     (char *) IS_101_play_dtmf_answr,
    &IS_101_answer_phone,
    &IS_101_beep,
    &IS_101_dial,
    &IS_101_handle_dle,
    &V253modem_init,
    &IS_101_message_light_off,
    &IS_101_message_light_on,
    &IS_101_start_play_file,
    NULL,
    &IS_101_stop_play_file,
    &IS_101_play_file,
    &IS_101_record_file,
    &V253modem_set_compression,
    &V253modem_set_device,
    &IS_101_stop_dialing,
    &IS_101_stop_playing,
    &IS_101_stop_recording,
    &IS_101_stop_waiting,
    &IS_101_switch_to_data_fax,
    &IS_101_voice_mode_off,
    &IS_101_voice_mode_on,
    &IS_101_wait,
    &IS_101_play_dtmf,
    &V253_check_rmd_adequation,
    0
    };
