/*
 * record.c
 *
 * This command records voice data from the voice modem and
 * saves them in the given file.
 *
 */

#include "../include/voice.h"

char *libvoice_record_c = "$Id: record.c,v 1.1 1997/12/16 12:21:13 marc Exp $";

int voice_record_file (char *name)
     {
     int fd;
     int result;
     rmd_header header;
     int bits;

     lprintf(L_MESG, "recording voice file %s", name);

     fd = creat(name, 0666);

     if (fd < 0)
          {
          lprintf(L_ERROR, "%s: Could not open voice file", program_name);
          return(FAIL);
          };

     if (voice_modem->set_compression(&cvd.rec_compression.d.i,
      &cvd.rec_speed.d.i, &bits) != OK)
          {
          lprintf(L_ERROR, "%s: Illeagal compression method 0x%04x, speed %d",
           program_name, cvd.rec_compression.d.i, cvd.rec_speed.d.i);
          close(fd);
          return(FAIL);
          };

     if (!cvd.raw_data.d.i)
          {
          memset(&header, 0x00, sizeof(rmd_header));
          sprintf(header.magic, "%s", "RMD1");
          sprintf(header.voice_modem_type, "%s", voice_modem_rmd_name);
          header.compression = htons(cvd.rec_compression.d.i);
          header.speed = htons(cvd.rec_speed.d.i);
          header.bits = bits;

          if (write(fd, &header, sizeof(rmd_header)) != sizeof(rmd_header))
               {
               lprintf(L_ERROR, "%s: Could not write header", program_name);
               return(FAIL);
               };

          };

     result = voice_modem->record_file(fd);
     close(fd);
     return(result);
     }
