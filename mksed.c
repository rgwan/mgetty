#ident "$Id: mksed.c,v 3.1 1995/08/30 12:40:54 gert Exp $ Copyright (c) Gert Doering"

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
    printf( "      -e 's;@FAX_STATION_ID@;%s;g'\\\n", FAX_STATION_ID );
    printf( "      -e 's;@FAX_LOG@;%s;g'\\\n", FAX_LOG );
    printf( "      -e 's;@LOG_PATH@;");
        printf( LOG_PATH, "ttyxx" );
        printf( ";g'\\\n" );
#ifdef SVR4
    printf( "      -e 's;@LOCK@;%s/LK.iii.jjj.kkk;g'\\\n", LOCK_PATH );
#else
    printf( "      -e 's;@LOCK@;");
        printf( LOCK, "ttyxx" );
        printf( ";g'\\\n" );
#endif
    printf( "      -e \"s;@MAILER@;%s;g\"\\\n", MAILER );
    printf( "      -e 's;@FAX_ADMIN@;%s;g'\\\n", MAIL_TO );
    printf( "      -e 's;@SPEED@;%d;g'\\\n", DEFAULT_PORTSPEED );
    printf( "      -e 's;@AWK@;%s;g'\\\n", AWK );
    printf( "      -e 's;@ECHO@;%s;g'\\\n", ECHO );
    printf( "      -e 's;@SHELL@;%s;g'\\\n", SHELL );
    printf( "      -e 's;@BINDIR@;%s;g'\\\n", BINDIR );
    printf( "      -e 's;@SBINDIR@;%s;g'\\\n", SBINDIR );
    printf( "      -e 's;@LIBDIR@;%s;g'\\\n", LIBDIR );
    printf( "      -e 's;@CONFDIR@;%s;g'\\\n", CONFDIR );
    printf( "      -e 's;@LOGIN@;%s;g'\\\n", DEFAULT_LOGIN_PROGRAM );
    printf( "      -e 's;@VOICE_DIR@;%s;g'\n", VOICE_DIR );
    return 0;
}
