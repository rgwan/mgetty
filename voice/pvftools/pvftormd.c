/*
 * pvftormd.c
 *
 * pvftormd converts from the pvf (portable voice format) format to the
 * rmd (raw modem data) format.
 *
 * $Id: pvftormd.c,v 1.3 1998/03/25 23:06:05 marc Exp $
 *
 */

#include "../include/voice.h"

char *program_name;

static void usage (void)
     {
     fprintf(stderr, "\n%s %s\n\n", program_name, vgetty_version);
     fprintf(stderr, "usage:\n");
     fprintf(stderr, "\t%s <modem type> <compression method> \\\n",
      program_name);
     fprintf(stderr, "\t  [options] [<pvffile> [<rmdfile>]]\n");
     fprintf(stderr, "\noptions:\n");
     fprintf(stderr, "\t-h     this help message\n");
     fprintf(stderr, "\t-L     list of supported raw modem data formats\n");
     fprintf(stderr, "\t       and compression methods\n\n");
     exit(ERROR);
     }

static void supported_formats (void)
     {
     fprintf(stderr, "\n%s %s\n\n", program_name, vgetty_version);
     fprintf(stderr, "supported raw modem data formats:\n\n");
     fprintf(stderr, " - Elsa        2, 3 and 4 bit Rockwell ADPCM\n");
     fprintf(stderr, " - ISDN4Linux  2, 3 and 4 bit ZyXEL ADPCM\n");
     fprintf(stderr, " - MT_2834     4 bit IMA ADPCM\n");
     fprintf(stderr, " - Rockwell    2, 3 and 4 bit Rockwell ADPCM\n");
     fprintf(stderr, " - US_Robotics 1 and 4 (GSM and G.721 ADPCM)\n");
     fprintf(stderr, " - ZyXEL_1496  2, 3 and 4 bit ZyXEL ADPCM\n");
     fprintf(stderr, " - ZyXEL_2864  2, 3 and 4 bit ZyXEL ADPCM\n");
     fprintf(stderr, "\nexample:\n\t%s Rockwell 4 infile.pvf outfile.rmd\n\n",
      program_name);
     exit(ERROR);
     }

int main (int argc, char *argv[])
     {
     int option;
     FILE *fd_in = stdin;
     FILE *fd_out = stdout;
     char *name_in = "stdin";
     char *name_out = "stdout";
     pvf_header header_in;
     rmd_header header_out = init_rmd_header;
     char *modem_type;
     int compression;

     check_system();
     program_name = argv[0];

     if (argc < 3)
          {

          if ((argc == 2) && (strcmp(argv[1], "-L") == 0))
               supported_formats();
          else
               usage();

          };

     modem_type = argv[1];
     compression = atoi(argv[2]);
     optind = 3;

     while ((option = getopt(argc, argv, "hL")) != EOF)
          {

          switch (option)
               {
               case 'L':
                    supported_formats();
               default:
                    usage();
               };

          };

     if (strcmp(modem_type, "MT_2834") == 0)
          modem_type = "Multitech2834";

     if (strcmp(modem_type, "US_Robotics") == 0)
          modem_type = "US Robotics";

     if (strcmp(modem_type, "ZyXEL_1496") == 0)
          modem_type = "ZyXEL 1496";

     if (strcmp(modem_type, "ZyXEL_2864") == 0)
          modem_type = "ZyXEL 2864";

     if ((strcmp(modem_type, "Elsa") == 0) ||
      (strcmp(modem_type, "ISDN4Linux") == 0) ||
      (strcmp(modem_type, "Multitech2834") == 0) ||
      (strcmp(modem_type, "Rockwell") == 0) ||
      (strcmp(modem_type, "US Robotics") == 0) ||
      (strcmp(modem_type, "ZyXEL 1496") == 0) ||
      (strcmp(modem_type, "ZyXEL 2864") == 0))
          strcpy(header_out.voice_modem_type, modem_type);
     else
          {
          fprintf(stderr, "%s: Invalid modem type (%s)\n", program_name,
           modem_type);
          supported_formats();
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

     if (read_pvf_header(fd_in, &header_in) != OK)
          exit(ERROR);

     header_out.speed = htons(header_in.speed);
     header_out.compression = htons(compression);

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

     if ((strcmp(modem_type, "Elsa") == 0) && ((compression == 2) ||
      (compression == 3) || (compression == 4)))
          {
          header_out.bits = compression;

          if (header_in.speed != 7200)
               {
               fprintf(stderr, "%s: Unsupported sample speed (%d)\n",
                program_name, header_in.speed);
               fprintf(stderr,
                "%s: The Elsa Microlink only supports 7200 samples per second\n",
                program_name);
               fclose(fd_out);

               if (fd_out != stdout)
                    unlink(name_out);

               exit(FAIL);
               };

          if (write_rmd_header(fd_out, &header_out) != OK)
               {
               fclose(fd_out);

               if (fd_out != stdout)
                    unlink(name_out);

               exit(ERROR);
               };

          if (pvftorockwell(fd_in, fd_out, compression, &header_in) == OK)
               exit(OK);

          };

     if ((strcmp(modem_type, "ISDN4Linux") == 0) &&
      ((compression == 2) || (compression == 3) || (compression == 4)))
          {
          header_out.bits = compression;

          if (write_rmd_header(fd_out, &header_out) != OK)
               {
               fclose(fd_out);

               if (fd_out != stdout)
                    unlink(name_out);

               exit(ERROR);
               };

          if (pvftozyxel(fd_in, fd_out, compression, &header_in) == OK)
               exit(OK);

          };

     if ((strcmp(modem_type, "Multitech2834") == 0) && (compression == 4))
          {
          header_out.bits = compression;

          if (write_rmd_header(fd_out, &header_out) != OK)
               {
               fclose(fd_out);

               if (fd_out != stdout)
                    unlink(name_out);

               exit(ERROR);
               }

          if (pvftoimaadpcm(fd_in, fd_out, &header_in) == OK)
               exit(OK);

          }

     if ((strcmp(modem_type, "ZyXEL 1496") == 0) &&
      ((compression == 2) || (compression == 3) || (compression == 4)))
          {
          header_out.bits = compression;

          if (header_in.speed != 9600)
               {
               fprintf(stderr, "%s: Unsupported sample speed (%d)\n",
                program_name, header_in.speed);
               fprintf(stderr,
                "%s: The ZyXEL 1496 only supports 9600 samples per second\n",
                program_name);
               fclose(fd_out);

               if (fd_out != stdout)
                    unlink(name_out);

               exit(FAIL);
               };

          if (write_rmd_header(fd_out, &header_out) != OK)
               {
               fclose(fd_out);

               if (fd_out != stdout)
                    unlink(name_out);

               exit(ERROR);
               };

          if (pvftozyxel(fd_in, fd_out, compression, &header_in) == OK)
               exit(OK);

          };

     if ((strcmp(modem_type, "ZyXEL 2864") == 0) &&
      ((compression == 2) || (compression == 3) || (compression == 4)))
          {
          header_out.bits = compression;

          if ((header_in.speed != 7200) && (header_in.speed != 8000) &&
           (header_in.speed != 9600) && (header_in.speed != 11025))
               {
               fprintf(stderr, "%s: Unsupported sample speed (%d)\n",
                program_name, header_in.speed);
               fprintf(stderr,
                "%s: The ZyXEL 2864 supports the following sample rates:\n",
                program_name);
               fprintf(stderr,
                "%s: 7200, 8000, 9600 and 11025\n", program_name);
               fclose(fd_out);

               if (fd_out != stdout)
                    unlink(name_out);

               exit(FAIL);
               };

          if (write_rmd_header(fd_out, &header_out) != OK)
               {
               fclose(fd_out);

               if (fd_out != stdout)
                    unlink(name_out);

               exit(ERROR);
               };

          if (pvftozyxel(fd_in, fd_out, compression, &header_in) == OK)
               exit(OK);

          };

     if ((strcmp(modem_type, "Rockwell") == 0) && ((compression == 2) ||
      (compression == 3) || (compression == 4)))
          {
          header_out.bits = compression;

          if (header_in.speed != 7200)
               {
               fprintf(stderr, "%s: Unsupported sample speed (%d)\n",
                program_name, header_in.speed);
               fprintf(stderr,
                "%s: Rockwell modems only support 7200 samples per second\n",
                program_name);
               fclose(fd_out);

               if (fd_out != stdout)
                    unlink(name_out);

               exit(FAIL);
               };

          if (write_rmd_header(fd_out, &header_out) != OK)
               {
               fclose(fd_out);

               if (fd_out != stdout)
                    unlink(name_out);

               exit(ERROR);
               };

          if (pvftorockwell(fd_in, fd_out, compression, &header_in) == OK)
               exit(OK);

          };

     if ((strcmp(modem_type, "US Robotics") == 0) && ((compression == 1) ||
         (compression == 4)))
          {
          header_out.bits = compression;

          if (write_rmd_header(fd_out, &header_out) != OK)
               {
               fclose(fd_out);

               if (fd_out != stdout)
                    unlink(name_out);

               exit(ERROR);
               };

          if (pvftousr(fd_in, fd_out, compression, &header_in) == OK)
               exit(OK);

          };

     fclose(fd_out);

     if (fd_out != stdout)
          unlink(name_out);

     fprintf(stderr, "%s: Unsupported compression method\n", program_name);
     exit(FAIL);
     }
