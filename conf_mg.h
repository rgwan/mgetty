#ident "$Id: conf_mg.h,v 3.4 1996/11/08 20:50:58 gert Exp $ Copyright (c) 1994 Gert Doering"

/* all (dynamic) mgetty configuration is contained in this structure.
 * It is initialized and loaded in conf_mg.c and accessed from mgetty.c
 */

extern struct conf_data_mgetty {
    struct conf_data
	speed,					/* port speed */
	switchbd,				/* speed switch for fax rec.*/
	direct_line,				/* direct lines */
	blocking,				/* do blocking open */

	port_owner,				/* "uucp" */
	port_group,				/* "modem" */
	port_mode,				/* "660" */

	toggle_dtr,				/* toggle DTR for modem reset */
	toggle_dtr_waittime,			/* time to hold DTR low */
	data_only,				/* no fax */
        fax_only,				/* no data */
	modem_type,				/* auto/c2.0/cls2/data */
	init_chat,				/* modem initialization */
	force_init_chat,			/* for stubborn modems */

	modem_check_time,			/* modem still alive? */
	rings_wanted,				/* number of RINGs */
	getcnd_chat,				/* get caller ID (for ELINK)*/
	answer_chat,				/* ATA...CONNECT...""...\n */
	answer_chat_timeout,			/* longer as S7! */
	autobauding,

	ringback,				/* ringback enabled */
	ringback_time,				/* ringback time */

	ignore_carrier,				/* do not clear CLOCAL */
	issue_file,				/* /etc/issue file */
	prompt_waittime,			/* ms wait before prompting */
	login_prompt,
	login_time,				/* max. time to log in */
	do_send_emsi,				/* send EMSI_REQ string */

	station_id,				/* local fax station ID */
	fax_server_file,			/* fax to send upon poll */
	diskspace,				/* min. free disk space */
	notify_mail,				/* fax mail goes to... */
	fax_owner,				/* "fax" */
	fax_group,				/* "staff" */
	fax_mode,				/* "660" */

	debug,					/* log level */
    
        statistics_chat,			/* get some call statist. */
        statistics_file,			/* default: log file */
	gettydefs_tag,
        termtype,				/* $TERM=... */
	end_of_config; } c;

int mgetty_parse_args _PROTO(( int argc, char ** argv ));
void mgetty_get_config _PROTO(( char * port ));
