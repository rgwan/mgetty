/*
 * pvffile.c
 *
 * pvffile prints out some useful information about .pvf files.
 *
 */

#include "../include/voice.h"

char *pvffile_c = "$Id: pvffile.c,v 1.2 1998/01/21 10:25:11 marc Exp $";
char *program_name;

static void usage (void)
     {
     fprintf(stderr, "\n%s %s\n\n", program_name, vgetty_version);
     fprintf(stderr, "usage:\n");
     fprintf(stderr, "\t%s [options] [pvffile]\n", program_name);
     fprintf(stderr, "\noptions:\n");
     fprintf(stderr, "\t-h     this help message\n\n");
     exit(ERROR);
     }

int main (int argc, char *argv[])
     {
     int option;
     FILE *fd_in = stdin;
     char *name_in = "stdin";
     pvf_header header;

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

     if (read_pvf_header(fd_in, &header) != OK)
          exit(FAIL);

     if (header.ascii)
          printf("%s: PVF2 (ascii)\n", name_in);
     else
          printf("%s: PVF1 (binary)\n", name_in);

     printf("channels: %d\n", header.channels);
     printf("sample speed: %d\n", header.speed);
     printf("bits per sample: %d\n", header.nbits);
     exit(OK);
     }
