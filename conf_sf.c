#ident "$Id: conf_sf.c,v 1.1 1994/12/05 20:52:13 gert Exp $ Copyright (c) Gert Doering"

/* conf_sf.c
 *
 * configuration defaults / configuration reading code for sendfax
 */

#include <string.h>
#include <unistd.h>

#include "mgetty.h"
#include "policy.h"
#include "syslibs.h"

#include "config.h"
#include "conf_sf.h"

#ifndef FAX_SEND_MAX_TRIES
#define FAX_SEND_MAX_TRIES 0
#endif

#ifndef FAX_SEND_SWITCHBD
#define FAX_SEND_SWITCHBD 0
#endif

struct conf_data_sendfax c = {
	{ "fax-devices", 0, CT_STRING, C_EMPTY },
	{ "modem-init", 0, CT_STRING, C_EMPTY },
	{ "modem-handshake", (int) FAX_MODEM_HANDSHAKE, CT_STRING, C_PRESET },
	{ "modem-type", (int) DEFAULT_MODEMTYPE, CT_STRING, C_PRESET },
	{ "max-tries", FAX_SEND_MAX_TRIES, CT_INT, C_PRESET },
	{ "speed", FAX_SEND_BAUD, CT_INT, C_PRESET },
	{ "switchbd", FAX_SEND_SWITCHBD, CT_INT, C_PRESET },
	{ "dial-prefix", (int) FAX_DIAL_PREFIX, CT_STRING, C_PRESET },
	{ "fax-id", (int)FAX_STATION_ID, CT_STRING, C_PRESET },
	{ "poll-dir", (int)".", CT_STRING, C_PRESET },
	{ "normal-res", 0, CT_BOOL, C_PRESET },
	{ "debug", LOG_LEVEL, CT_INT, C_PRESET },
	{ "verbose", FALSE, CT_BOOL, C_PRESET },
	{ "" /* polling */, FALSE, CT_BOOL, C_PRESET },
	{ "page-header", 0, CT_STRING, C_EMPTY },
	{ "" /* stdin */, FALSE, CT_BOOL, C_PRESET },
	{ NULL, 0, CT_STRING, C_EMPTY }};

int sendfax_parse_args _P2( (argc,argv), int argc, char ** argv )
{
int opt;
char * p;

    /* sanity check:
     * make sure that structs-in-struct can be handled exactly as if
     * packed in array (get_config relies on it!)
     */
conf_data c_a[2];
    if ( ( (char *)&c_a[1] - (char *)&c_a[0] )  != 
	 ( (char *)&c.modem_init - (char *)&c.ttys ) )
    {
	fprintf( stderr, "ERROR: config table size mixup. contact author\n" );
	exit(99);
    }

    /* since "ttys" has to be writable, we strdup() the default string */
    p = malloc( sizeof( FAX_MODEM_TTYS )+1 );
    if ( p == NULL )
	c.ttys.flags = C_EMPTY;
    else
    {
	strcpy( p, FAX_MODEM_TTYS );
	c.ttys.d.p = p;
	c.ttys.flags = C_CONF;
    }

    /* get command line arguments */
    while ((opt = getopt(argc, argv, "d:vx:ph:l:nm:SC:I:")) != EOF)
    {
	switch (opt) {
	  case 'd':	/* set target directory for polling */
	    conf_set_string( &c.poll_dir, optarg );
	    break;
	  case 'v':	/* verbose blurb on stdout */
	    conf_set_bool( &c.verbose, TRUE );
	    break;
	  case 'x':	/* set debug level */
	    conf_set_int( &c.debug, atoi(optarg) );
	    log_set_llevel( c_int(debug) );
	    break;
	  case 'p':	/* activate poll receive */
	    conf_set_int( &c.fax_poll_wanted, TRUE );
	    break;
	  case 'h':	/* set header page */
	    conf_set_string( &c.fax_page_header, optarg );
	    break;
	  case 'l':	/* set device(s) to use */
	    if ( optarg[0] == '/' &&
		 strncmp( optarg, "/dev/", 5 ) != 0 )
	    {
		fprintf( stderr, "%s: -l: device must be located in /dev!\n",
		                 argv[0]);
		exit(1);
	    }
	    conf_set_string( &c.ttys, optarg );
	    break;
	  case 'n':	/* set normal resolution */
	    conf_set_bool( &c.normal_res, TRUE );
	    break;
	  case 'm':	/* modem initialization string */
	    conf_set_string( &c.modem_init, optarg );
	    break;
	  case 'S':	/* modem on stdin */
	    conf_set_bool( &c.use_stdin, TRUE );
	    break;
	  case 'C':	/* modem class */
	    conf_set_string( &c.modem_type, optarg );
	    break;
	  case 'I':	/* local fax id */
	    conf_set_string( &c.station_id, optarg );
	    break;
	  case '?':	/* unrecognized parameter */
	    return ERROR;
	    break;
	}
    }

    return NOERROR;
}

/* get sendfax configuration from file (if configured)
 *
 * if "port == NULL", read "fax-devices" (c.tty), if != NULL, skip
 * c.tty (because it woudln't make sense to set it, and would break
 * fax_open())
 */
void sendfax_get_config _P1( (port), char * port )
{
#ifdef SENDFAX_CONFIG
    if ( port == NULL )
    {
	lprintf( L_NOISE, "reading default configuration" );
	get_config( makepath( SENDFAX_CONFIG, CONFDIR ),
		    (conf_data *)&c, "port", NULL );
    }
    else
    {
	lprintf( L_NOISE, "reading specific data for port '%s'", port );
	get_config( makepath( SENDFAX_CONFIG, CONFDIR ),
		    ((conf_data *)&c)+1, "port", port );
    }
#else
    lprintf( L_NOISE, "not reading config file, not configured" );
#endif
    log_set_llevel( c_int(debug) );
}
