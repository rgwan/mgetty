#ident "$Id: conf_sf.h,v 3.3 1996/03/11 00:11:31 gert Exp $ Copyright (c) 1994 Gert Doering"

/* all (dynamic) sendfax configuration is contained in this structure.
 * It is initialized and loaded in conf_sf.c and accessed from sendfax.c
 */

extern struct conf_data_sendfax {
    struct conf_data
	ttys,
        ttys_0,				/* for "ignore" */
	modem_init,
	modem_handshake,
	modem_type,
	max_tries,
        max_tries_ctd,
	speed,
	switchbd,
	dial_prefix,
	station_id,
	poll_dir,
	normal_res,
	debug,
	verbose,
	fax_poll_wanted,	/* cli only (-p) */
	fax_page_header,
	use_stdin,		/* cli only (-S) */
        rename_files,		/* cli only (-r) */
        acct_handle,		/* cli only (-A) */
	end_of_config; } c;

int sendfax_parse_args _PROTO(( int argc, char ** argv ));
void sendfax_get_config _PROTO(( char * port ));
