/* tap.c
 *
 * TAP/UCP support for calling up paging servers (experimental)
 *
 * Copyright (c) 1997 Gert Doering
 */
#ident "$Id: tap.c,v 1.4 1997/12/19 11:58:45 gert Exp $"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "policy.h"
#include "mgetty.h"
#include "tio.h"
#include "config.h"

char * Device = "/dev/cuaa1";

#define STX 0x02
#define ETX 0x03
#define EOT 0x04
#define CR  0x0d
#define ACK 0x06
#define NAK 0x15

typedef enum { Ptap, Pucp1, Pucp52, Pcityruf } prot_type;

struct prov_t { char * name;
		char * prefix;
		prot_type proto;
		char * dialup_num;
		char * modem_init;
		int msglen; } 
	prov[] = {
    { "E+", "0177", Ptap, "01771167", "ATZ", 160 },
    { "D1", "0171", Ptap, "01712092522", "ATZ", 160 }, /*!!! untested */
    { "D2", "0172", Pucp1, "01722278020", "AT+FCLASS=0;&N0S7=60", 160 },	/*!!! UCP52 */
    { "Quix", "quix-t", Pucp1, "016593", "ATZ", 80 }, /*!!! untested */
    { "CR1", "cityruf", Pcityruf, "01691", "ATZ", 80 }, /*!!! untested */
    { "CR2", "cityruf", Pcityruf, "01691", "ATZ", 80 }, /*!!! untested */
    { NULL, NULL, Ptap, NULL, NULL }};


#define MAXMSG 100
struct messi { int prov;		/* provider */
	       char * number;
	       char * msg; } messi[100] = {
/*    { 0, "2160221", "TapSent - Umlaut öäüßÖÄÜ" }, */
    { 2, "8213646", "Hallo Maex! Mal gucken, ob der Watchdog funzt..." }};
int nummsg = 1;

int tap_send_text( int fd, char * field1, char * field2 )
{
    int csum=0, bufl=1, i;

    char buf[240];

    buf[0] = STX;

    strcpy( &buf[bufl], field1 );
    bufl += strlen(field1);
    buf[bufl++] = CR;

    strcpy( &buf[bufl], field2 );
    bufl += strlen(field2);
    buf[bufl++] = CR;

    buf[bufl++] = ETX;

    for ( i=0; i<bufl; i++ ) 
    {
	lprintf( L_JUNK, "%2d: %c x=%02x d=%3d", i, buf[i], buf[i], buf[i]);
        csum += ( buf[i] & 0x7f );
    }
    lprintf ( L_JUNK, "csum: o=%03o x=%02x", csum, csum );

    buf[bufl++] = 0x30 + ( ( csum >> 8 ) &0x0f );
    buf[bufl++] = 0x30 + ( ( csum >> 4 ) &0x0f );
    buf[bufl++] = 0x30 + ( csum          & 0x0f );
    buf[bufl++] = CR;

    lprintf( L_MESG, "sending %d bytes, chksum %02x %02x %02x", 
			bufl, buf[bufl-4], buf[bufl-3], buf[bufl-2] );

    if ( fd > 0 )
    {
	if ( write( fd, buf, bufl ) != bufl ) 
	{
	    lprintf( L_ERROR, "can't send buf" );
	    perror( "can't send message buffer" );
	    exit(27);
	}
    }
}

int tap_send_message( int fd, char * field1, char * field2 )
{
int count, ack;
char ch, buf[400]; int bl;

    printf( "\nphone number: \"%s\"\nmessage: \"%s\"\n", field1, field2 );

    tap_send_text( fd, field1, field2 );

    lprintf( L_MESG, "wait for ACK, got: " );
    count=0;
    ack=0;

    bl=0;

    while( count < 5 )
    {
        if ( wait_for_input( fd, 20000 ) )
        {
	    read( fd, &ch, 1 );
	    lputc( L_MESG, ch );

	    if ( ch == CR )
	    {
		if ( bl <= 0 ) continue; 	/* empty buffer */

		if ( buf[bl-1] == ACK )		/* success!! */
		{
		    printf( "got ACK -> short message successfully sent.\n" );
		    break;
		}

		/* print text messages */
		buf[bl]=0;
		printf( "--> \"%s\"\n", buf );
		bl=0;
		continue;
	    }

	    if ( bl>sizeof(buf)-10 )		/* paranoia check */
	    {
		lprintf( L_FATAL, "buffer overrun, bl=%d\n", bl );
		return -1;
	    }
	    
	    buf[bl++]=ch;

	    if ( ch == NAK )			/* failure */
	    {
		lprintf( L_ERROR, "got NAK... huh?? anyway, going on" );
		printf( "got NAK -> message was NOT sent!\n" );
		break;
	    }
	}
	else
	{
	    lprintf( L_MESG, "got timeout, going on..." );
	    printf( "got TIMEOUT -> message was (possibly) NOT sent!\n" );
	    count++;
	    break;
	}
    }
    return 0;
}

int ucp_send_string( int fd, char * string )
{
static int trn=1;		/* transaction number */

unsigned int l, chksum;
char buf[1024];

    sprintf( buf, "%c%02d/%05d/%s/", STX, trn++, strlen(string)+12, string );
	
    chksum=0;
    for( l=1; buf[l]; l++) chksum+=(unsigned) buf[l];

    sprintf( &buf[l], "%02X%c", (chksum & 0xff), ETX );

    lprintf( L_MESG, "ucp_s_s: send '%s'.", buf );;

    if ( fd >= 0 )
    {
	if ( write( fd, buf, strlen(buf)) != strlen(buf) )
	{
	    lprintf( L_ERROR, "ucp_s_s: write failed" );
	    return ERROR;
	}
    }
    return NOERROR;
}

int ucp_send_message( int fd, char * r, char * text )
{
    char buf[800], ch;
    int l, count;

    /*!!! length checks */

    printf( "\nphone number: \"%s\"\nmessage: \"%s\"\n", r, text );

    sprintf( buf, "O/01/%s///3/", r );
    l = strlen( buf );

    while( *text && l<sizeof(buf)-10 )
    {
	ch=*(text++);
	switch( ch )			/* special rules */
	{
	    case '$': ch=0x02; break;
	    case 'ß': ch=0x1e; break;
	    case '@': ch=0x5f; break;
	    case 'Ü': ch=0x5e; break;
	    case 'ü': ch=0x7e; break;
	    case '^': ch=0x5d; break;
	}
	sprintf( &buf[l], "%02X", (unsigned) ch );
	l+=2;
    }

    if ( ucp_send_string( fd, buf ) == ERROR ) return ERROR;

    /* wait for ACK */
    lprintf( L_NOISE, "message sent, waiting for ACK, got: " );
    count=0; l=0;
    while( count<10 )		/*!!! higher timeout for QUIX */
    {
	if ( !wait_for_input( fd, 3000 ) ) 
		{ lputs( L_NOISE, "<T>" ); count++; continue; }

	if ( read( fd, &ch, 1 ) != 1 )
	{
	    lprintf( L_ERROR, "ucp_s_m: read failed" ); return ERROR;
	}

	lputc( L_NOISE, ch );

        if ( ch == STX ) l=0;		/* start of text */
	buf[l++]=ch;
	if ( ch == ETX ) break;
    }

    lprintf( "break, count=%d, ch=0x%02x", count, ch );

    if ( buf[10] != 'R' || buf[15] != 'A' )
    {
	printf( "got NAK -> message was NOT sent!\n" );
	return ERROR;
    }

    printf( "got ACK -> short message successfully sent.\n" );
    return NOERROR;
}

static int sm_timeout;
RETSIGTYPE sm_sigalarm( SIG_HDLR_ARGS )
{
    lprintf( L_WARN, "sm_sigalarm: got timeout" );
    sm_timeout = TRUE;
}

/* send data SLOW (with 1/10 sec. delay between each byte), for
 * the SMSC of Deutsche Telekom, which can't take characters back-to-back
 */
void slowsend( int fd, char * buf, int len )
{
    lprintf( L_JUNK, "slowsend: " );
    while( len > 0 )
    {
	delay(50);
	lputc( L_JUNK, buf[0] );
	write( fd, buf, 1 );
	buf++; len--;
    }
}

int cityruf_send_message( int fd, char * r, char * text )
{
    char buf[800], ibuf[800], ch;
    int l, count, rx;

    signal( SIGALRM, sm_sigalarm );
#ifdef HAVE_SIGINTERRUPT
    siginterrupt( SIGALRM, TRUE );
#endif
    sm_timeout = FALSE;
    alarm(30);

    /*!!! length checks */

    printf( "\nphone number: \"%s\"\nmessage: \"%s\"\n", r, text );
    lprintf( L_NOISE, "cityruf, got:" );

    count=0;
    rx=0;
    while( ( l = read( fd, &ch, 1 ) ) == 1 && ! sm_timeout && rx<sizeof(ibuf) )
    {
	lputc( L_NOISE, ch );
	ibuf[rx++] = ch;

	if ( ch == ' ' && ibuf[rx-2] == '>' )
	{
	    switch( count++ )
	    {
		case 0:		/* phone number */
		  sprintf( buf, "%s\r", r ); 
		  lprintf( L_NOISE, "#0: send number '%s'", buf );
		  slowsend( fd, buf, strlen(buf) );
		  break;
		case 1:		/* message */
		  sprintf( buf, "%s\r", text ); 
		  lprintf( L_NOISE, "#1: send messi (len=%d)", strlen(buf) );
		  slowsend( fd, buf, strlen(buf) );
		  break;
		case 2:		/* "absenden?" */
		  sprintf( buf, "ja\r" ); 
		  lprintf( L_NOISE, "#2: sent ACK '%s'", buf );
		  slowsend( fd, buf, strlen(buf) );
		  break;
	    }
	    lprintf( L_NOISE, "got: " );
	    rx=0;
	}

	if ( ch == '.' && rx > 6 && 
	      strncmp( &ibuf[rx-6], "Anruf.", 6 ) == 0 )
	{
	    lprintf( L_MESG, "*success*, bye" );
	    break;
	}
	if ( ch == 'R' && rx > 10 &&
	      strncmp( &ibuf[rx-10], "NO CARRIER", 10 ) == 0 )
	{
	    lprintf( L_MESG, "missed end-of-message?!?" );
	    l=0;
	    break;
	}
    }
    alarm(0);

    if ( l < 1 || sm_timeout )				/* error? */
    {
	printf( "protocol error -> message most likely  not sent.\n");
	lprintf( L_ERROR, "cityruf: read() returned error" );
	return ERROR;
    }

    printf( "got ACK -> short message successfully sent.\n" );
    return NOERROR;
}


int initial_handshake( int fd, prot_type proto )
{
int count, ack;
char buf;

    if ( proto != Ptap )
    {
	lprintf( L_MESG, "no initial handshake necessary" );
	return 0;
    }

    lprintf( L_MESG, "TAP, initial handshake, got: " );

    count=ack=0;
    while( count < 10 )
    {
        if ( wait_for_input( fd, 2000 ) )
        {
	    read( fd, &buf, 1 );
	    lputc( L_MESG, buf );

	    if ( buf == '=' ) 	/* "ID=" */
	    {
		lprintf( L_MESG, "sending <esc>PG1<cr>, got: " );
		write( fd, "\x1bPG1\x0d", 5 );
		ack=0;
	    }

	    if ( buf == '\x1b' && ack == 0 ) ack++;
	    if ( buf == '[' && ack == 1 ) ack++;
	    if ( buf == 'p' && ack == 2 ) ack++;
	    if ( buf == CR  && ack == 3 ) break;
	}
	else
	{
	    lputs( L_MESG, "<send [CR]>" );
	    write( fd, "\x0d", 1 );
	    count++;
	}
    }

    lprintf( L_MESG, "count=%d, ack=%d", count, ack );

    if ( count >= 10 ) return -1;

    return 0;
}

/* final handshake before hanging up -- only TAP needs this 
 */
int final_handshake( int fd, prot_type proto )
{
char ch;

    if ( proto == Ptap )
    {
	lprintf( L_MESG, "TAP, last message sent, send <EOT><CR>, got: " );
	write( fd, "\x04\x0d", 2 );		/* <EOT><CR> */

	while( wait_for_input( fd, 2000 ) && read( fd, &ch, 1 ) == 1 )
	{
	    lputc( L_MESG, ch );
	}
    }
    return 0;
}

/* send all jobs queued for a given provider "pt" */
int send_jobs( int pt )
{
int fd;
TIO tio;
char * l;
char dialstring[200];
int i;

    lprintf( L_MESG, "connecting to provider '%s'", prov[pt].name );
    printf( "\nConnecting to %s...\n", prov[pt].name );

    fd = open( Device, O_RDWR | O_NDELAY );

    if ( fd < 0 )
    {
	lprintf( L_ERROR, "can't open %s", Device );
	perror( "open" );
	return ERROR;
    }

    fcntl( fd, F_SETFL, O_RDWR );

    if ( tio_get( fd, &tio ) == ERROR )
    {
	perror( "tio_get" ); close(fd); return ERROR;
    }
    tio_mode_sane(&tio,TRUE);
    tio_set_speed(&tio, 38400);
    tio_mode_raw(&tio);

    /* go to 7E1 for TAP and UCP */

    if ( prov[pt].proto != Pcityruf )
    {
	tio.c_iflag |= ISTRIP | IXON | IXANY;

	tio.c_cflag &= ~(CSIZE | PARODD);
	tio.c_cflag |= CS7 | PARENB;

    }
    if ( tio_set( fd, &tio ) == ERROR )
    {
	perror( "tio_set" ); close(fd); return ERROR;
    }

    if ( mdm_command( prov[pt].modem_init, fd ) == ERROR )
    {
	fprintf( stderr, "can't initialize modem. ABORT.\n" );
	close(fd); return ERROR;
    }

    /* dial up provider... */
    sprintf( dialstring, "ATX3DT0W%s", prov[pt].dialup_num );
    mdm_send( dialstring, fd );

    /* wait for connect */  /*!!!! TIMEOUT */
    while( 1 )
    {
	l = mdm_get_line(fd);
	if ( l == NULL ) break;

	lprintf( L_NOISE, "mdm: read '%s'", l );

	if ( strcmp( l, "NO CARRIER" ) == 0 )  { l = NULL; break; }
	if ( strcmp( l, "NO DIALTONE" ) == 0 ) { l = NULL; break; }
	if ( strcmp( l, "ERROR" ) == 0 )       { l = NULL; break; }
	if ( strcmp( l, "BUSY" ) == 0 )        { l = NULL; break; }
	if ( strncmp( l, "CONNECT", 7 ) == 0 ) break;
    }

    if ( l == NULL )
    {
	fprintf( stderr, "dialup failed\n" ); close(fd); 
	return ERROR;
    }

    lprintf( L_MESG, "modem connection established." );

    /* handshake phase */
    if ( initial_handshake( fd, prov[pt].proto ) < 0 )
    {
	fprintf( stderr, "can't initiate succesful handshake with provider!\n");
	close(fd); return ERROR;
    }

    /* initial handshake successful, send SMS */
    printf( "handshake with %s service successful, now sending...\n",
	       prov[pt].proto == Ptap? "TAP": 
		 (prov[pt].proto == Pcityruf? "Cityruf" : "UCP") );

    for ( i=0; i<nummsg; i++)
    {
	if ( messi[i].prov == pt )	/* provider connected to */
	{
	    switch( prov[pt].proto )
	    {
		case Ptap:
		    tap_send_message( fd, messi[i].number, messi[i].msg );
		    break;
		case Pucp1:
		    ucp_send_message( fd, messi[i].number, messi[i].msg );
		    break;
		case Pcityruf:
		    cityruf_send_message( fd, messi[i].number, messi[i].msg );
		    break;
		default:
		    fprintf( stderr, "pt=%d, yet unsupported protocol %d\n",
			     pt, prov[pt].proto );
		    break;
	    }
	}		/* if (correct provider) */
    }			/* for (i=all messages) */

    if ( final_handshake( fd, prov[pt].proto ) < 0 )
    {
	fprintf( stderr, "final handshake failed - messages propably not sent\r\n" );
	close(fd); return ERROR;
    }
    
    close(fd);
    return SUCCESS; 
}

/*
 * read file with all messages "in queue"...
 */
int read_messages( char * name )
{
    char * line, * key;
    FILE * fp = fopen( name, "r" );

    if ( fp == NULL )
    {
	lprintf( L_ERROR, "can't open %s", name );
	perror( "error opening message file" );
	return SUCCESS;
    }

    nummsg = 0;

    while ( nummsg < MAXMSG && 
	    ( line = fgetline( fp )) != NULL )
    {
	char * p = line;
	int i;

	norm_line( &p, &key );
	for ( i=0; prov[i].name != NULL; i++ )	/* search prov. in list */
	{
	    if ( strcmp( prov[i].name, key ) == 0 )	/* found */
	    {
		norm_line( &p, &key );		/* get phone number */
		messi[nummsg].prov = i;
		messi[nummsg].number = strdup( key );
		messi[nummsg].msg    = strdup( p );
		break;
	    }
	}

	if ( prov[i].name == NULL )
	{
	    fprintf( stderr, "ERROR: can't find provider '%s' (%s %s)\n",
			key, key, p ); 
	    continue;
	}

	nummsg++;
    }
    fclose( fp );
    return NOERROR;
}


int main( int argc, char ** argv )
{
int i, pt;

    log_init_paths( argv[0], "/tmp/tap.log", NULL );
    log_set_llevel(5);

    if ( argc != 2 )
    {
	fprintf( stderr, "Usage: tap <filename>\n" ); exit(1);
    }

    /* read messages from file in argv[1] */
    if ( read_messages( argv[1] ) != SUCCESS )
    {
	fprintf( stderr, "can't process %s, exiting.\n", argv[1] ); exit(2);
    }

    if ( makelock( Device ) != SUCCESS )
    {
	fprintf( stderr, "device %s is locked, abort.\n", Device );
	exit(17);
	/* FIXME */
    }

    for ( pt=0; prov[pt].name != NULL; pt++ )
    {
	for( i=0; i<nummsg; i++ ) { if ( messi[i].prov == pt ) break; }

	if ( i<nummsg )		/* found message for that provider */
	{
	    send_jobs(pt); sleep(5);
	    /*!!! ERROR CODES!!! */
	}
    }
    
    rmlocks();
    return 0;
 
}


