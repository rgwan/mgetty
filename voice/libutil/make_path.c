/*
 * make_path.c
 *
 * Builds a complete file path from a directory name and a filename.
 *
 */

#include "../include/voice.h"

char *libutil_make_path_c = "$Id: make_path.c,v 1.2 1998/01/21 10:24:40 marc Exp $";

void make_path _P3((result, path, name), char *result, char *path,
 char *name)
     {

     if (name[0] == '/')
          {
          strcpy(result, name);
          }
     else
          {
          strcpy(result, path);
          strcat(result, "/");
          strcat(result, name);
          };

     }
