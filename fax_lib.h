#ident "$Id: fax_lib.h,v 1.13 1994/05/14 16:42:56 gert Exp $ Copyright (c) Gert Doering"
;

/* fax_lib.h
 * 
 * declare protopes for all the fax functions in faxrec.c and faxlib.c
 * declare global variables set by functions in faxlib.c
 * declare all the constants required for Class 2 faxing
 */

/* data types + variables */

typedef enum { Mt_unknown, Mt_data, Mt_class2, Mt_class2_0 } Modem_type;
extern Modem_type modem_type;

typedef enum { pp_mps, pp_eom, pp_eop,
	       pp_pri_mps, pp_pri_eom, pp_pri_eop } Post_page_messages;

extern unsigned char fax_send_swaptable[];

/* function prototypes */

int fax_send _PROTO(( char * s, int fd ));	/* write to fd, with logging */
                                         /* expect string, handle fax msgs */
int fax_wait_for _PROTO(( char * s, int fd ));
int fax_command _PROTO(( char * send, char * expect, int fd ));
int mdm_command _PROTO(( char * send, int fd ));
char * fax_get_line _PROTO(( int fd ));

int fax_get_pages _PROTO(( int fd, int * pagenum, char * directory ));
int fax_get_page_data _PROTO(( int modem_fd, int pagenum, char * directory ));
int fax_read_byte _PROTO(( int fd, char * c ));

int fax_set_l_id _PROTO(( int fd, char * fax_id ));
int fax_set_fdcc _PROTO(( int fd, int fine, int maxsp, int minsp ));
int fax_set_bor  _PROTO(( int fd, int bit_order ));

#ifdef __TIO_H__
int fax_send_page _PROTO(( char * g3_file, TIO * tio,
			   Post_page_messages ppm, int fd ));
int fax_send_ppm  _PROTO(( int fd, TIO *tio, Post_page_messages ppm ));
#endif

Modem_type fax_get_modem_type _PROTO(( int fd, char * mclass ));

typedef	struct	{ short vr, br, wd, ln, df, ec, bf, st; } fax_param_t;

extern	char	fax_remote_id[];		/* remote FAX id +FTSI */
extern	char	fax_param[];			/* transm. parameters +FDCS */
extern	char	fax_hangup;
extern	int	fax_hangup_code;
extern	int	fax_page_tx_status;
extern	fax_param_t	fax_par_d;
extern	boolean	fax_to_poll;			/* there's something */
						/* to poll */
extern	boolean	fax_poll_req;			/* caller wants to poll */


/* fax_hangup_code gives the reason for failure, normally it's a positive
 * number returned by the faxmodem in the "+FHNG:iii" response. If the
 * modem returned BUSY or NO_CARRIER or ERROR, we use negative numbers to
 * signal what has happened. "-4" means something toally unexpeced.
 */

#define	FHUP_BUSY	-2
#define FHUP_ERROR	-3
#define FHUP_UNKNOWN	-4

#define ETX	003
#define DLE	020
#define SUB	032
#define DC2	022
#define XON	021
#define XOFF	023

#ifndef ERROR
#define	ERROR	-1
#define NOERROR	0
#endif

