#ident "$Id: mksed.c,v 1.9 1994/08/08 12:34:34 gert Exp $ Copyright (c) Gert Doering"

#include <stdio.h>

#include "mgetty.h"
#include "policy.h"

int main _P0( void )
{
    printf( "sed \\\n" );
    printf( "      -e 's;@ADMIN@;%s;g'\\\n", ADMIN );
    printf( "      -e 's;@FAX_SPOOL@;%s;g'\\\n", FAX_SPOOL );
    printf( "      -e 's;@FAX_SPOOL_IN@;%s;g'\\\n", FAX_SPOOL_IN );
    printf( "      -e 's;@FAX_SPOOL_OUT@;%s;g'\\\n", FAX_SPOOL_OUT );
    printf( "      -e 's;@FAX_MODEM_TTYS@;%s;g'\\\n", FAX_MODEM_TTYS );
    printf( "      -e 's;@FAX_LOG;%s;g'\\\n", FAX_LOG );
    printf( "      -e \"s;@MAILER@;%s;g\"\\\n", MAILER );
    printf( "      -e 's;@FAX_ADMIN@;%s;g'\\\n", MAIL_TO );
    printf( "      -e 's;@AWK@;%s;g'\\\n", AWK );
    printf( "      -e 's;@ECHO@;%s;g'\\\n", ECHO );
    printf( "      -e 's;@SHELL@;%s;g'\\\n", SHELL );
    printf( "      -e 's;@BINDIR@;%s;g'\\\n", BINDIR );
    printf( "      -e 's;@SBINDIR@;%s;g'\\\n", SBINDIR );
    printf( "      -e 's;@LIBDIR@;%s;g'\\\n", LIBDIR );
    printf( "      -e 's;@VOICE_DIR@;%s;g'\n", VOICE_DIR );
    return 0;
}
