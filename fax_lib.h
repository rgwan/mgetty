#ident "$Id: fax_lib.h,v 1.3 1993/03/21 10:40:23 gert Exp $ Gert Doering"

/* fax_lib.h
 * 
 * declare protopes for all the fax functions in faxrec.c and faxlib.c
 * declare global variables set by functions in faxlib.c
 * declare all the constants required for Class 2 faxing
 */

int fax_send( char * s, int fd );	/* write to fd, with logging */
int fax_wait_for( char * s, int fd );	/* expect string, handle fax msgs */
int fax_command( char * send, char * expect, int fd );

int fax_get_pages( int fd, int * pagenum, char * directory );
int fax_get_page_data( int modem_fd, int pagenum, char * directory );

unsigned char swap_bits(unsigned char c);

typedef	struct	{ short vr, br, wd, ln, df, ec, bf, st; } fax_param_t;

extern	char	fax_remote_id[];		/* remote FAX id +FTSI */
extern	char	fax_param[];			/* transm. parameters +FDCS */
extern	char	fax_hangup;
extern	int	fax_hangup_code;
extern	fax_param_t	fax_par_d;
extern	boolean	fax_to_poll;			/* there's something to poll */

/* fax_hangup_code gives the reason for failure, normally it's a positive
 * number returned by the faxmodem in the "+FHNG:iii" response. If the
 * modem returned BUSY or NO_CARRIER or ERROR, we use negative numbers to
 * signal what has happened
 */

#define	FHUP_BUSY	-2
#define FHUP_ERROR	-3

#define ETX	003
#define DLE	020
#define DC2	022
#define XON	021
#define XOFF	023

#define	ERROR	-1
#define NOERROR	0
