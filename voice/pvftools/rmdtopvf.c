/*
 * rmdtopvf.c
 *
 * rmdtopvf converts from the rmd (raw modem data) format to the pvf
 * (portable voice format) format.
 *
 * $Id: rmdtopvf.c,v 1.6 1999/03/16 09:59:26 marcs Exp $
 *
 */

#include "../include/voice.h"

char *program_name;

static void usage (void)
     {
     fprintf(stderr, "\n%s %s\n\n", program_name, vgetty_version);
     fprintf(stderr, "usage:\n");
     fprintf(stderr, "\t%s [options] [<rmdfile> [<pvffile>]]\n", program_name);
     fprintf(stderr, "\noptions:\n");
     fprintf(stderr, "\t-h     this help message\n");
     fprintf(stderr, "\t-L     list of supported raw modem data formats\n");
     fprintf(stderr, "\t-a     output pvf ascii format\n");
     fprintf(stderr, "\t-b     output pvf binary format (default)\n");
     fprintf(stderr, "\t-8     output 8 bit samples\n");
     fprintf(stderr, "\t-16    output 16 bit samples\n");
     fprintf(stderr, "\t-32    output 32 bit samples (default)\n\n");
     exit(ERROR);
     }

static void supported_formats (void)
     {
     fprintf(stderr, "\n%s %s\n\n", program_name, vgetty_version);
     fprintf(stderr, "supported raw modem data formats:\n\n");
     fprintf(stderr, " - Digi           4 (G.711U) and 5 (G.711A)\n");
     fprintf(stderr, " - Elsa           2, 3 and 4 bit Rockwell ADPCM\n");
     fprintf(stderr, " - ISDN4Linux     2, 3 and 4 bit ZyXEL ADPCM\n");
     fprintf(stderr, " - Multitech 2834 4 bit IMA ADPCM\n");
     fprintf(stderr, " - Rockwell       2, 3 and 4 bit Rockwell ADPCM\n");
     fprintf(stderr, " - Rockwell       8 bit Rockwell PCM\n");
     fprintf(stderr, " - US Robotics    1 and 4 (GSM and G.721 ADPCM)\n");
     fprintf(stderr, " - ZyXEL 1496     2, 3 and 4 bit ZyXEL ADPCM\n");
     fprintf(stderr, " - ZyXEL 2864     2, 3 and 4 bit ZyXEL ADPCM\n\n");
     exit(ERROR);
     }

int main (int argc, char *argv[])
     {
     int option;
     FILE *fd_in = stdin;
     FILE *fd_out = stdout;
     char *name_in = "stdin";
     char *name_out = "stdout";
     rmd_header header_in;
     pvf_header header_out = init_pvf_header;
     char *modem_type;
     int compression;

     check_system();
     program_name = argv[0];

     while ((option = getopt(argc, argv, "abhL12368")) != EOF)
          {

          switch (option)
               {
               case 'a':
                    header_out.ascii = TRUE;
                    break;
               case 'b':
                    header_out.ascii = FALSE;
                    break;
               case 'L':
                    supported_formats();
               case '8':
                    header_out.nbits = 8;
                    break;
               case '1':
               case '6':
                    header_out.nbits = 16;
                    break;
               case '3':
               case '2':
                    header_out.nbits = 32;
                    break;
               default:
                    usage();
               };

          };

     if (optind < argc)
          {
          name_in = argv[optind];

          if ((fd_in = fopen(name_in, "r")) == NULL)
               {
               fprintf(stderr, "%s: Could not open file %s\n", program_name,
                name_in);
               exit(ERROR);
               };

          optind++;
          };

     if (read_rmd_header(fd_in, &header_in) != OK)
          exit(ERROR);

     header_out.speed = ntohs(header_in.speed);
     modem_type = header_in.voice_modem_type;
     compression = ntohs(header_in.compression);

     if (header_out.speed == 0)
          {

          if ((strcmp(modem_type, "ZyXEL 1496") == 0) ||
           (strcmp(modem_type, "ZyXEL 2864") == 0))
               header_out.speed = 9600;

          if ((strcmp(modem_type, "Rockwell") == 0) ||
           (strcmp(modem_type, "Elsa") == 0))
               header_out.speed = 7200;

          }

     if (optind < argc)
          {
          name_out = argv[optind];

          if ((fd_out = fopen(name_out, "w")) == NULL)
               {
               fprintf(stderr, "%s: Could not open file %s\n", program_name,
                name_out);
               exit(FAIL);
               };

          };

     if (write_pvf_header(fd_out, &header_out) != OK)
          {
          fclose(fd_out);

          if (fd_out != stdout)
               unlink(name_out);

          exit(ERROR);
          }

     if ((strcmp(modem_type, "Multitech2834") == 0) && (compression == 4))
          {

          if (imaadpcmtopvf(fd_in, fd_out, &header_out) == OK)
               exit(OK);

          }

     if (((strcmp(modem_type, "ZyXEL 1496") == 0) ||
      (strcmp(modem_type, "ZyXEL 2864") == 0) ||
      (strcmp(modem_type, "ISDN4Linux") == 0)) &&
      ((compression == 2) || (compression == 3) || (compression == 30) ||
      (compression == 4)))
          {

          if (zyxeltopvf(fd_in, fd_out, compression, &header_out) == OK)
               exit(OK);

          };

     if (strcmp(modem_type, "Rockwell") == 0 && compression == 8)
          {

	  if (rockwellpcmtopvf(fd_in, fd_out, compression, &header_out) == OK)
               exit(OK);

          };

     if (((strcmp(modem_type, "Rockwell") == 0 && compression != 8) ||
      (strcmp(modem_type, "Elsa") == 0)) && ((compression == 2) ||
      (compression == 3) || (compression == 4)))
          {

          if (rockwelltopvf(fd_in, fd_out, compression, &header_out) == OK)
               exit(OK);

          };

     if ((strcmp(modem_type, "US Robotics") == 0) && ((compression == 1) ||
      (compression == 4)))
          {

          if (usrtopvf(fd_in, fd_out, compression, &header_out) == OK)
               exit(OK);

          };

     if ((strcmp(modem_type, "Digi RAS") == 0) && ((compression == 4) ||
      (compression == 5)))
          {
	  /*
	   * TODO: handle alaw
	   */
          if (basictopvf(fd_in, fd_out, &header_out) == OK)
               exit(OK);

          };

     fclose(fd_out);

     if (fd_out != stdout)
          unlink(name_out);

     fprintf(stderr, "%s: Unsupported compression method (%s/%d)\n",
      program_name, modem_type, compression);
     exit(FAIL);
     }
