#ident "$Id: conf_mg.h,v 4.13 2018/03/05 18:34:05 gert Exp $ Copyright (c) 1994 Gert Doering"

/* all (dynamic) mgetty configuration is contained in this structure.
 * It is initialized and loaded in conf_mg.c and accessed from mgetty.c
 *
 * $Log: conf_mg.h,v $
 * Revision 4.13  2018/03/05 18:34:05  gert
 * Add new config option "open-delay <msec>" (same thing as for sendfax)
 *
 * Feature wish from Marc Daniel Fege <marc@fege.net>, needed on some ELSAs
 *
 * Revision 4.12  2010/09/14 15:38:27  gert
 * add CVS log
 * add options for SMS processing - sms-handler, sms-handler-{user,group}
 *
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
	open_delay,			/* time time to wait for draining */
	need_dsr,				/* wait for DSR+CTS */
	data_only,				/* no fax */
        fax_only,				/* no data */
	modem_type,				/* auto/c2.0/cls2/data */
	modem_quirks,				/* strange behaviourisms */
	init_chat,				/* modem initialization */
	force_init_chat,			/* for stubborn modems */
	post_init_chat,				/* for forgetful modems */
	data_flow,				/* flow ctl. in data mode */
	fax_send_flow,				/*   '' in fax rec mode */
	fax_rec_flow,				/*   '' in fax send mode */

	modem_check_time,			/* modem still alive? */
	rings_wanted,				/* number of RINGs */
	msn_list,				/* ISDN MSNs (dist.ring) */
	getcnd_chat,				/* get caller ID (for ELINK)*/
	cnd_program,				/* accept caller? */
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
	env_ttyprompt,				/* suppress login prompt */
	login_config,				/* login.config file name */

	station_id,				/* local fax station ID */
	fax_min_speed,				/* minimum fax speed */
	fax_max_speed,				/* maximum fax sped */
	fax_server_file,			/* fax to send upon poll */
	diskspace,				/* min. free disk space */
	notify_mail,				/* fax mail goes to... */
	fax_owner,				/* "fax" */
	fax_group,				/* "staff" */
	fax_mode,				/* "660" */
	fax_spool_in,				/* "/var/fax/inc:/tmp" */

	debug,					/* log level */
    
        statistics_chat,			/* get some call statist. */
        statistics_file,			/* default: log file */
	gettydefs_tag,
        termtype,				/* $TERM=... */
#ifdef SMS
	sms_handler,				/* sms handler program */
	sms_handler_user,			/* sms handler program */
	sms_handler_group,			/* sms handler program */
#endif
	end_of_config; } c;

int mgetty_parse_args _PROTO(( int argc, char ** argv ));
void mgetty_get_config _PROTO(( char * port ));
