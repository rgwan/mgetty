/*
 * play.c
 *
 * This command plays the given file.
 *
 */

#include "../include/voice.h"

char *libvoice_play_c = "$Id: play.c,v 1.1 1997/12/16 12:21:11 marc Exp $";

int voice_play_file (char *name)
     {
     int fd;
     int result;
     rmd_header header;
     int compression;
     int speed;
     int bits;

     lprintf(L_MESG, "playing voice file %s", name);

     fd = open(name, O_RDONLY);

     if (fd < 0)
          {
          lprintf(L_ERROR, "%s: Could not open voice file", program_name);
          return(FAIL);
          };

     if (!cvd.raw_data.d.i)
          {

          if (read(fd, &header, sizeof(rmd_header)) != sizeof(rmd_header))
               {
               lprintf(L_ERROR, "%s: Could not read header", program_name);
               return(FAIL);
               };

          if (strncmp(header.magic, "RMD1", 4) != 0)
               {
               lprintf(L_ERROR, "%s: No raw modem data header found",
                program_name);
               return(FAIL);
               }
          else
               lprintf(L_NOISE, "%s: raw modem data header found",
                program_name);

          if (strncmp(header.voice_modem_type, voice_modem_rmd_name,
           strlen(voice_modem_rmd_name)) != 0)
               {
               lprintf(L_ERROR, "%s: Wrong modem type found", program_name);
               return(FAIL);
               }
          else
               lprintf(L_NOISE, "%s: modem type %s found", program_name,
                header.voice_modem_type);

          compression = ntohs(header.compression);
          speed = ntohs(header.speed);
          bits = header.bits;
          lprintf(L_NOISE, "%s: compression method 0x%04x, speed %d, bits %d",
           program_name, compression, speed, bits);
          }
     else
          {
          compression = cvd.rec_compression.d.i;
          speed = cvd.rec_speed.d.i;
          }

     if (voice_modem->set_compression(&compression, &speed, &bits) != OK)
          {
          lprintf(L_ERROR, "%s: Illeagal compression method", program_name);
          result = FAIL;
          }
     else
          result = voice_modem->play_file(fd);

     close(fd);
     return(result);
     }
