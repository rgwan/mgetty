/*
 * au.c
 *
 * Conversion pvf <--> au.
 *
 * $Id: au.c,v 1.3 1998/03/25 23:05:16 marc Exp $
 *
 */

#include "../include/voice.h"

typedef long Word; /* must be 32 bits */

typedef struct
     {
     Word magic;               /* magic number SND_MAGIC */
     Word dataLocation;        /* offset or pointer to the data */
     Word dataSize;            /* number of bytes of data */
     Word dataFormat;          /* the data format code */
     Word samplingRate;        /* the sampling rate */
     Word channelCount;        /* the number of channels */
     Word info;                /* optional text information */
     } SNDSoundStruct;

#define SND_MAGIC             (0x2e736e64L)
#define SND_HEADER_SIZE       28
#define SND_UNKNOWN_SIZE      ((Word)(-1))

#ifdef PRINT_INFO
static char sound_format[][30] =
     {
     "unspecified",
     "uLaw_8",
     "linear_8",
     "linear_16",
     "linear_24",
     "linear_32",
     "float",
     "double",
     "fragmented",
     };
#endif

/*
 * This routine converts from linear to ulaw.
 *
 * Craig Reese: IDA/Supercomputing Research Center
 * Joe Campbell: Department of Defense
 * 29 September 1989
 *
 * References:
 * 1) CCITT Recommendation G.711  (very difficult to follow)
 * 2) "A New Digital Technique for Implementation of Any
 *     Continuous PCM Companding Law," Villeret, Michel,
 *     et al. 1973 IEEE Int. Conf. on Communications, Vol 1,
 *     1973, pg. 11.12-11.17
 * 3) MIL-STD-188-113,"Interoperability and Performance Standards
 *     for Analog-to_Digital Conversion Techniques,"
 *     17 February 1987
 *
 * Input: Signed 16 bit linear sample
 * Output: 8 bit ulaw sample
 */

#define ZEROTRAP    /* turn on the trap as per the MIL-STD */
#undef ZEROTRAP
#define BIAS 0x84   /* define the add-in bias for 16 bit samples */
#define CLIP 32635

unsigned char linear2ulaw (int sample)
     {
     static int exp_lut[256] =
          {
          0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
          4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
          5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
          5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6 ,6,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
          6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
          };
     int sign;
     int exponent;
     int mantissa;
     unsigned char ulawbyte;

     /*
      * Get the sample into sign-magnitude.
      */

     sign = (sample >> 8) & 0x80;   /* set aside the sign */

     if (sign != 0)                 /* get magnitude */
          sample = -sample;

     if (sample > CLIP)             /* clip the magnitude */
          sample = CLIP;

     /*
      * Convert from 16 bit linear to ulaw.
      */

     sample = sample + BIAS;
     exponent = exp_lut[(sample >> 7 ) & 0xff];
     mantissa = (sample >> (exponent + 3)) & 0x0f;
     ulawbyte = ~(sign | (exponent << 4) | mantissa);

#ifdef ZEROTRAP

     if (ulawbyte == 0)
          ulawbyte = 0x02;          /* optional CCITT trap */

#endif

     return(ulawbyte);
     }

/*
 * This routine converts from ulaw to 16 bit linear.
 *
 * Craig Reese: IDA/Supercomputing Research Center
 * 29 September 1989
 *
 * References:
 * 1) CCITT Recommendation G.711  (very difficult to follow)
 * 2) MIL-STD-188-113,"Interoperability and Performance Standards
 *     for Analog-to_Digital Conversion Techniques,"
 *     17 February 1987
 *
 * Input: 8 bit ulaw sample
 * Output: signed 16 bit linear sample
 */

int ulaw2linear (unsigned char ulawbyte)
     {
     static int exp_lut[8] = {0, 132, 396, 924, 1980, 4092, 8316, 16764};
     int sign;
     int exponent;
     int mantissa;
     int sample;

     ulawbyte = ~ulawbyte;
     sign = (ulawbyte & 0x80);
     exponent = (ulawbyte >> 4) & 0x07;
     mantissa = ulawbyte & 0x0f;
     sample = exp_lut[exponent] + (mantissa << (exponent + 3));

     if (sign != 0)
          sample = -sample;

     return(sample);
     }

static Word read_word (FILE *in)
     {
     Word w;

     w = getc(in);
     w = (w << 8) | getc(in);
     w = (w << 8) | getc(in);
     w = (w << 8) | getc(in);
     return(w);
     }

static void write_word (Word w, FILE *out)
     {
     putc((w & 0xff000000) >> 24, out);
     putc((w & 0x00ff0000) >> 16, out);
     putc((w & 0x0000ff00) >> 8, out);
     putc((w & 0x000000ff), out);
     }

int pvftoau (FILE *fd_in, FILE *fd_out, pvf_header *header_in,
 int dataFormat)
     {
     SNDSoundStruct hdr;
     int sample;

     hdr.magic = SND_MAGIC;
     hdr.dataLocation = SND_HEADER_SIZE;
     hdr.dataSize = SND_UNKNOWN_SIZE;
     hdr.dataFormat = dataFormat;
     hdr.samplingRate = header_in->speed;
     hdr.channelCount = 1;
     hdr.info = 0;

     write_word(hdr.magic, fd_out);
     write_word(hdr.dataLocation, fd_out);
     write_word(hdr.dataSize, fd_out);
     write_word(hdr.dataFormat, fd_out);
     write_word(hdr.samplingRate, fd_out);
     write_word(hdr.channelCount, fd_out);
     write_word(hdr.info, fd_out);

     switch (hdr.dataFormat)
          {
          case SND_FORMAT_MULAW_8:

               while (!feof(fd_in))
                    {
                    sample = header_in->read_pvf_data(fd_in) >> 8;

                    if (sample > 0x7fff)
                         sample = 0x7fff;

                    if (sample < -0x8000)
                         sample = -0x8000;

                    putc(linear2ulaw(sample) & 0xff, fd_out);
                    }

               break;
          case SND_FORMAT_LINEAR_8:

               while (!feof(fd_in))
                    {
                    sample = header_in->read_pvf_data(fd_in) >> 16;

                    if (sample > 0x7f)
                         sample = 0x7f;

                    if (sample < -0x80)
                         sample = -0x80;

                    putc(sample & 0xff, fd_out);
                    }

               break;
          case SND_FORMAT_LINEAR_16:

               while (!feof(fd_in))
                    {
                    sample = header_in->read_pvf_data(fd_in) >> 8;

                    if (sample > 0x7fff)
                         sample = 0x7fff;

                    if (sample < -0x8000)
                         sample = -0x8000;

                    putc((sample >> 8) & 0xff, fd_out);
                    putc(sample & 0xff, fd_out);
                    }

               break;
          default:
               fprintf(stderr, "%s: unsupported sound file format requested",
                program_name);
               return(ERROR);
          }

     return(OK);
     }

int autopvf (FILE *fd_in, FILE *fd_out, pvf_header *header_out)
     {
     SNDSoundStruct hdr;
     int i;
     int sample;

     hdr.magic = read_word(fd_in);
     hdr.dataLocation = read_word(fd_in);
     hdr.dataSize = read_word(fd_in);
     hdr.dataFormat = read_word(fd_in);
     hdr.samplingRate = read_word(fd_in);
     hdr.channelCount = read_word(fd_in);
     /* hdr.info = read_word(fd_in); */ /* this is sometimes missing */

     if (hdr.magic != SND_MAGIC)
          {
          fprintf(stderr, "%s: illegal magic number for an .au file",
           program_name);
          return(ERROR);
          }

#ifdef PRINT_INFO
          {
          Word fmt=hdr.dataFormat;

     if ((hdr.dataFormat >= 0) && (hdr.dataFormat < (sizeof(sound_format) /
      sizeof(sound_format[0]))))
          printf("%s: Data format: %s\n", program_name,
           sound_format[hdr.dataFormat]);
     else
          printf("%s: Data format unknown, code=%ld\n", prgoram_name,
           (long) hdr.dataFormat);

     fprintf(stderr, "Sampling rate: %ld\n", (long) hdr.samplingRate);
     fprintf(stderr, "Number of channels: %ld\n", (long) hdr.channelCount);
     fprintf(stderr, "Data location: %ld\n", (long) hdr.dataLocation);
     fprintf(stderr, "Data size: %ld\n", (long) hdr.dataSize);
#endif

     if (hdr.channelCount != 1)
          {
          fprintf(stderr, "%s: number of channels (%ld) is not 1\n",
           program_name, hdr.channelCount);
          return(ERROR);
          }

     header_out->speed = hdr.samplingRate;

     if (write_pvf_header(fd_out, header_out) != OK)
          {
          fprintf(stderr, "%s: could not write pvf header\n",
           program_name);
          return(ERROR);
          };

     for (i = SND_HEADER_SIZE; i < hdr.dataLocation; i++)

          if (getc(fd_in) == EOF)
               {
               fprintf(stderr, "%s: unexpected end of file\n",
                program_name);
               return(ERROR);
               }

     switch (hdr.dataFormat)
          {
          case SND_FORMAT_MULAW_8:

               while ((sample = getc(fd_in)) != EOF)
                    header_out->write_pvf_data(fd_out,
                     ulaw2linear(sample) << 8);

               break;
          case SND_FORMAT_LINEAR_8:

               while ((sample = getc(fd_in)) != EOF)
                    {
                    sample &= 0xff;

                    if (sample > 0x7f)
                         sample -= 0x100;

                    header_out->write_pvf_data(fd_out, (sample << 16));
                    }

               break;
          case SND_FORMAT_LINEAR_16:

               while ((sample = getc(fd_in)) != EOF)
                    {
                    sample &= 0xffff;

                    if (sample > 0x7fff)
                         sample -= 0x10000;

                    header_out->write_pvf_data(fd_out, (sample << 8));
                    }

               break;
          default:
               fprintf(stderr, "%s: unsupported or illegal sound encoding\n",
                program_name);
               return(ERROR);
          }

     return(OK);
     }

int pvftobasic (FILE *fd_in, FILE *fd_out, pvf_header *header_in)
     {
     int sample;

     if (header_in->speed != 8000)
          {
          fprintf(stderr, "%s: sample speed (%d) must be 8000\n",
           program_name, header_in->speed);
          return(ERROR);
          };

     while (!feof(fd_in))
          {
          sample = header_in->read_pvf_data(fd_in) >> 8;
          putc(linear2ulaw(sample), fd_out);
          }

     return(OK);
     }

int basictopvf (FILE *fd_in, FILE *fd_out, pvf_header *header_out)
     {
     int sample;

     if (header_out->speed != 8000)
          {
          fprintf(stderr, "%s: sample speed (%d) must be 8000\n",
           program_name, header_out->speed);
          return(ERROR);
          };

     while ((sample = getc(fd_in)) != EOF)
          header_out->write_pvf_data(fd_out, ulaw2linear(sample) << 8);

     return(OK);
     }
