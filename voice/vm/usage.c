/* main.c
 *
 * VoiceModem is the program for handling the voice modem functionality
 * from shell scripts.
 *
 * $Id: usage.c,v 1.5 1999/10/09 15:54:01 marcs Exp $
 *
 */

#include "vm.h"

void usage(void)
     {
     fprintf(stderr, "\n%s %s\n\n", program_name, vgetty_version);
     fprintf(stderr, "usage:\n");
     fprintf(stderr,
      "\t%s beep   [options] [<frequency> [<length in 0.1sec>]]\n",
      program_name);
     fprintf(stderr, "\t%s diagnostics <device name (e.g. ttyS2)>\n",
      program_name);
     fprintf(stderr, "\t%s dial   [options] <phone number>\n", program_name);
     fprintf(stderr, "\t%s help\n", program_name);
     fprintf(stderr, "\t%s play   [options] <file names>\n", program_name);
     fprintf(stderr, "\t%s record [options] <file name>\n", program_name);
     fprintf(stderr,
      "\t%s shell  [options] [<shell script> [shell options]]\n", program_name);
     fprintf(stderr, "\t%s wait   [options] [<time in seconds>]\n",
      program_name);
     fprintf(stderr, "\noptions:\n");
     fprintf(stderr, "\t-c <n> use compression type <n> (default is %d)\n",
      cvd.rec_compression.d.i);
     fprintf(stderr, "\t-h     this help message\n");
     fprintf(stderr, "\t-i     input from internal microphone\n");
     fprintf(stderr, "\t-l <s> set device string (e.g. -l ttyS2:ttyC0)\n");
     fprintf(stderr, "\t-m     input from external microphone\n");
     fprintf(stderr, "\t-e     output to external modem speaker\n");
     fprintf(stderr, "\t-s     output to internal modem speaker\n");
     fprintf(stderr, "\t-t     use telco line (default)\n");
     fprintf(stderr, "\t-v     verbose output\n");
     fprintf(stderr, "\t-w     use off / on hook signal from local handset\n");
     fprintf(stderr, "\t       to start and stop recording\n");
     fprintf(stderr, "\t-x <n> set debug level\n");
     fprintf(stderr, "\t-H     use local handset\n");
     fprintf(stderr, "\t-L <n> set maximum recording length in sec\n");
     fprintf(stderr, "\t-P     print first DTMF tone on stdout and exit\n");
     fprintf(stderr,
      "\t-R     read and print DTMF string on stdout and exit\n");
     fprintf(stderr,
      "\t-S <s> set default shell for shell scripts (e.g. -S /bin/sh)\n");
     fprintf(stderr, "\t-T <n> set silence timeout in 0.1sec\n");
     fprintf(stderr, "\t-V <n> set silence threshold to <n> (0-100%%)\n\n");
     exit(ERROR);
     }
