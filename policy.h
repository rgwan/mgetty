#ident "$Id: policy.h,v 1.1 1993/03/06 19:38:31 gert Exp $ (c) Gert Doering"

/* this is the file where all configuration for mgetty / sendfax is done
 */

/* user id of the "uucp" user. The tty device will be owned by this user,
 * so parallel dial-out of uucico will be possible
 */
#define UUCPID	5

/* system console - if a severe error happens at startup, mgetty writes
 * a message to this file and aborts
 */
#define CONSOLE "/dev/syscon"

/* System administrator - if a severe error happens later (lprintf called
 * with log_level L_FATAL - the logfile will be mailed to him
 */
#define ADMIN	"root"

/* System name - printed at login prompt
 */
#define SYSTEM	"greenie"

/* Path where the GETTY logfiles go to.
 */
#define LOG_PATH "/tmp"

/* Path for the lock files. A %s will be replaced with the device name,
 * e.g. tty2a -> /usr/spool/uucp/LCK..%s
 * Make sure that this is the same file that your uucico uses for
 * locking!
 */
#define LOCK "/usr/spool/uucp/LCK..%s"

/* the main fax spool directory
 */
#define FAX_SPOOL	"/usr/spool/fax"

/* where incoming faxes go to
 * getty needs write permissions!
 */
#define FAX_SPOOL_IN	FAX_SPOOL"/incoming"

/* name of the logfile for outgoing faxes
 */
#define FAX_LOG		FAX_SPOOL"/Faxlog"

/* local station ID
 * 20 character string, most faxmodem allow all ascii characters 32..127,
 * but some do only allow digits and blank
 * AT+FLID=? should tell you what's allowed and what not.
 */
#define FAX_STATION_ID	" +49-89-3243228 Gert"

/* the baudrate used for *sending* faxes. ZyXELs should handle 38400,
 * SUPRAs (and other rockwell-based faxmodems) do not
 */
#define FAX_SEND_BAUD B19200

/* the device used for faxing (this will change in future releases)
 * (without (!) leading /dev/)
 */
#define FAX_MODEM_TTY	"tty2a"

/* mailer that accepts destination on command line and mail text on stdin
 */
#define MAILER		"/usr/lib/sendmail"

/* where to send notify mail about incoming faxes to
 */
#define MAIL_TO		ADMIN
