#ident "$Id: mksed.c,v 1.4 1994/02/11 14:50:48 gert Exp $ Copyright (c) Gert Doering"
;
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
    printf( "      -e 's;@BINDIR@;%s;g'\\\n", BINDIR );
    printf( "      -e 's;@LIBDIR@;%s;g'\n", LIBDIR );
    return 0;
}
