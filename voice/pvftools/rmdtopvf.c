/*
 * rmdtopvf.c
 *
 * rmdtopvf converts from the rmd (raw modem data) format to the pvf
 * (portable voice format) format.
 *
 * $Id: rmdtopvf.c,v 1.14 2001/02/24 11:43:41 marcs Exp $
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
     fprintf(stderr, " - Lucent     1   8 bit linear\n");
     fprintf(stderr, " - Lucent     2   16 bit linear\n");
     fprintf(stderr, " - Lucent     4   8 bit u-law\n");
     fprintf(stderr, " - Lucent     5   4 bit IMA ADPCM\n");
     fprintf(stderr, " - Multitech 2834 4 bit IMA ADPCM\n");
     fprintf(stderr, " - Multitech 5634 4 bit IMA ADPCM\n");
     fprintf(stderr, " - Rockwell       2, 3 and 4 bit Rockwell ADPCM\n");
     fprintf(stderr, " - Rockwell       8 bit Rockwell PCM\n");
     fprintf(stderr, " - V253modem      8 bit unsigned PCM\n");
     fprintf(stderr, " - V253modem      8 bit signed PCM\n");
     fprintf(stderr, " - V253modem      4 bit IMA ADPCM\n");
     fprintf(stderr, " - V253modem      2 and 4 bit Rockwell ADPCM\n");
     fprintf(stderr, " - UMC            4 (G.721 ADPCM)\n");
     fprintf(stderr, " - US Robotics    1 and 4 (GSM and G.721 ADPCM)\n");
     fprintf(stderr, " - ZyXEL 1496     2, 3 and 4 bit ZyXEL ADPCM\n");
     fprintf(stderr, " - ZyXEL 2864     2, 3 and 4 bit ZyXEL ADPCM\n");
     fprintf(stderr, " - ZyXEL Omni 56K 4 bit Digispeech ADPCM (?)\n\n");
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
           (strcmp(modem_type, "ZyXEL 2864") == 0) ||
           (strcmp(modem_type, "ZyXEL Omni 56K") == 0))
               header_out.speed = 9600;

          if ((strcmp(modem_type, "Rockwell") == 0) ||
           (strcmp(modem_type, "Elsa") == 0))
               header_out.speed = 7200;

	  if (strcmp(modem_type, "Lucent") == 0)
	    header_out.speed = 8000;

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

     if (((strcmp(modem_type, "Multitech2834") == 0 && compression == 4))  ||
       ((strcmp(modem_type, "Lucent") == 0) && (compression == 5)))
          {

          if (imaadpcmtopvf(fd_in, fd_out, &header_out) == OK)
               exit(OK);

          }

     if (strcmp(modem_type, "Multitech5634") == 0 && compression == 4)
          {

          if (imaadpcmtopvf(fd_in, fd_out, &header_out) == OK)
               exit(OK);

          }

     if (strcmp(modem_type, "ZyXEL Omni 56K") == 0 && compression == 4)
          {

          if (zo56ktopvf(fd_in, fd_out, &header_out) == OK)
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

     if (strcmp(modem_type, "V253modem")==0)
	{
          switch(compression)
          {
          case 0:
          case 1:
          case 8:
            if (lintopvf(fd_in, fd_out, &header_out, 0, 0, 0) == OK)
               exit(OK);
            else exit(FAIL);
          case 9:
            if (lintopvf(fd_in, fd_out, &header_out, 1, 0, 0) == OK)   /* signed linear */
               exit(OK);
            else exit(FAIL);

          case 2:
          case 4:
            if (rockwelltopvf(fd_in, fd_out, compression, &header_out) == OK)   /* the &Elsa compatible formats */
               exit(OK);
            else exit(FAIL);

          case 5:
            if (imaadpcmtopvf(fd_in, fd_out, &header_out) == OK)
               exit(OK);
            else exit(FAIL);
          case 6:   /* please add here uLaw or mu-Law (thats what Zyxel calls this) */
          case 10:  /* please add here uLaw or mu-Law (thats what Zyxel calls this) */
             fprintf(stderr, "here should be support for u-Law logarithmic see sox -U \n");
             break;

          case 7:
          case 11:  /* please add here aLaw */
             fprintf(stderr, "here should be support for a-Law logarithmic see sox -A \n");
             break;

          default:
            fprintf(stderr, "%s: Unsupported compression method (%s/%d)\n",
              program_name, modem_type, compression);
            exit(FAIL);
          }
        }

     if (((strcmp(modem_type, "Rockwell") == 0 && compression != 8) ||
      (strcmp(modem_type, "Elsa") == 0)) && ((compression == 2) ||
      (compression == 3) || (compression == 4)))
          {

          if (rockwelltopvf(fd_in, fd_out, compression, &header_out) == OK)
               exit(OK);

          };

     if ((strcmp(modem_type, "UMC") == 0) && (compression == 4))
          {

          if (usrtopvf(fd_in, fd_out, compression, &header_out) == OK)
               exit(OK);

          };

     if ((strcmp(modem_type, "US Robotics") == 0) && ((compression == 1) ||
      (compression == 4)))
          {

          if (usrtopvf(fd_in, fd_out, compression, &header_out) == OK)
               exit(OK);

          };

     if (((strcmp(modem_type, "Digi RAS") == 0) && ((compression == 4) ||
      (compression == 5))) || ((strcmp(modem_type, "Lucent") == 0) && (compression == 4)))
          {
	  /*
	   * TODO: handle alaw
	   */
          if (basictopvf(fd_in, fd_out, &header_out) == OK)
               exit(OK);

          };

     if ((strcmp(modem_type, "Lucent") == 0) && (compression == 1))
       {
          if (pvftolin(fd_in, fd_out, &header_out, 0, 8, 0) == OK)
               exit(OK);
       }

     if ((strcmp(modem_type, "Lucent") == 0) && (compression == 2))
       {
          if (pvftolin(fd_in, fd_out, &header_out, 0, 16, 0) == OK)
               exit(OK);
       }

     fclose(fd_out);

     if (fd_out != stdout)
          unlink(name_out);

     fprintf(stderr, "%s: Unsupported compression method (%s/%d)\n",
      program_name, modem_type, compression);
     exit(FAIL);
     }
