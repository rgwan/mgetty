/*
 * rmdfile.c
 *
 * rmdfile prints out some useful information about .rmd files.
 *
 */

#include "../include/voice.h"

char *rmdfile_c = "$Id: rmdfile.c,v 1.2 1998/01/21 10:25:18 marc Exp $";
char *program_name;

static void usage (void)
     {
     fprintf(stderr, "\n%s %s\n\n", program_name, vgetty_version);
     fprintf(stderr, "usage:\n");
     fprintf(stderr, "\t%s [options] [rmdfile]\n", program_name);
     fprintf(stderr, "\noptions:\n");
     fprintf(stderr, "\t-h     this help message\n\n");
     exit(ERROR);
     }

int main (int argc, char *argv[])
     {
     int option;
     FILE *fd_in = stdin;
     char *name_in = "stdin";
     rmd_header header;

     check_system();
     program_name = argv[0];

     if ((option = getopt(argc, argv, "h")) != EOF)
          usage();

     if (optind < argc)
          {
          name_in = argv[optind];

          if ((fd_in = fopen(name_in, "r")) == NULL)
               {
               fprintf(stderr, "%s: Could not open file %s\n", program_name,
                name_in);
               exit(FAIL);
               };

          };

      if (read_rmd_header(fd_in, &header) != OK)
          exit(FAIL);

     printf("%s: RMD1\n", name_in);
     printf("modem type is: \"%s\"\n", header.voice_modem_type);
     printf("compression method: 0x%04x\n", ntohs(header.compression));
     printf("sample speed: %d\n", ntohs(header.speed));
     printf("bits per sample: %d\n", header.bits);
     exit(OK);
     }
