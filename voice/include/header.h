/*
 * voice_header.h
 *
 * Defines the header for raw modem data.
 *
 */

#ifdef MAIN
char *voice_header_h = "$Id: header.h,v 1.2 1998/01/21 10:24:09 marc Exp $";
#endif

typedef struct
     {
     char magic[4];
     char voice_modem_type[16];
     short compression;
     short speed;
     char bits;
     char reserved[7];
     } rmd_header;
