#ident "$Id: policy.h,v 1.15 1993/07/23 21:38:21 gert Exp $ (c) Gert Doering"

/* this is the file where all configuration for mgetty / sendfax is done
 */

/* main configuration file
 * definitions in this file can override the defaults compiled into the
 * program
 * NOT USED YET.
 */
#define CONFIG_FILE "/u/gert/mgetty/config"

/* user id of the "uucp" user. The tty device will be owned by this user,
 * so parallel dial-out of uucico will be possible
 */
#define UUCPID	5

/* access mode for the line while getty has it - it should be accessible
 * by uucp / uucp, but not by others (imagine someone dialing into your
 * system and using another modem to dial to another country...)
 */
#define FILE_MODE 0660

/* system console - if a severe error happens at startup, mgetty writes
 * a message to this file and aborts
 */
#ifdef SVR4
#define CONSOLE "/dev/console"
#else
#define CONSOLE "/dev/syscon"
#endif

/* System administrator - if a severe error happens later (lprintf called
 * with log_level L_FATAL - the logfile will be mailed to him
 */
#define ADMIN	"root"

/* System name - printed at login prompt
 */
#define SYSTEM	"greenie"

/* Login prompt - "%s" will be replaced by SYSTEM
 * override with "-p <prompt>" switch
 */
#define LOGIN_PROMPT	"%s!login:"

/* Name of the mgetty log file
 * e.g. "/usr/spool/log/mgetty.log.%s"
 * a "%s" will be replaced by the device name, e.g. "tty2a"
 */
#define LOG_PATH "/tmp/log_mg.%s"

/* Path for the lock files. A %s will be replaced with the device name,
 * e.g. tty2a -> /usr/spool/uucp/LCK..%s
 * Make sure that this is the same file that your uucico uses for
 * locking!
 */

#ifdef SVR4
#define LOCK_PATH "/var/spool/locks"
#define LOCK      "/var/spool/locks/LCK..%s"
#endif

#ifndef LOCK
#define LOCK "/usr/spool/uucp/LCK..%s"
#endif
  
/* Set this to "1" if your system uses binary lock files (i.e., the pid
 * as four byte integer in host byte order written to the lock file)
 * If it is "0", HDB locking will be used - the PID will be written as
 * 10 byte ascii, with a trailing newline
 */
#define LOCKS_BINARY 0

/* the default speed used by mgetty - override it with "-s <speed>"
 *
 * WARNING: ZyXELs can do faxreceive with 38400, but I've been told
 * that supra faxmodems do only work with 19200 - could somebody try this?
 */
#define DEFAULT_PORTSPEED	B38400

/* the modem initialization string
 *
 * this sample string is for ZyXELs, for other modems you'll have to
 * replace &H3 (rts/cts flow control), &K4 (enable v42bis) and &N0 (answer
 * with all known protocols).
 * For some modems, an initial "\d" is needed
 * The modem must answer with "OK" (!!!) - otherwise, change mgetty.c
 */
#define MODEM_INIT_STRING	"ATS0=0Q0&D3&H3&N0&K4"

/* the main fax spool directory
 */
#define FAX_SPOOL	"/usr/spool/fax"

/* where incoming faxes go to
 * getty needs write permissions!
 */
#define FAX_SPOOL_IN	FAX_SPOOL"/incoming"

/* if your received faxes are corrupted (missing lines), because the
 * faxmodem does not honor RTS handshake, define this.
 * mgetty will then do XON/XOFF flow control in fax receive mode
 * (I do *not* think that it's necessary)
 */
/* #define FAX_RECEIVE_USE_IXOFF */

/* name of the logfile for outgoing faxes
 */
#define FAX_LOG		FAX_SPOOL"/Faxlog"

/* local station ID
 * 20 character string, most faxmodem allow all ascii characters 32..127,
 * but some do only allow digits and blank
 * AT+FLID=? should tell you what's allowed and what not.
 */
#define FAX_STATION_ID	" 49 89 3243328 "

/* ------ sendfax-specific stuff follows here -------- */

/* the baudrate used for *sending* faxes. ZyXELs should handle 38400,
 * SUPRAs (and other rockwell-based faxmodems) do not
 * I recommend 38400, since 19200 may be to slow for 14400 bps faxmodems!
 */
#define FAX_SEND_BAUD B38400

/* this is the command to set the modem to use the desired flow control
 * for hardware handshake, this could be &H3 for the ZyXEL, &K4 for
 * Rockwell-Based modems or \\Q3&S0 for Exar-Based Modems (i.e. some GVC's)
 */
#define FAX_MODEM_HANDSHAKE "&H3"

/* if your faxmodem insists on using XON/XOFF flow control in class 2 fax
 * mode (even when told not to), define this (ZyXELs are know to do this).
 */
#define FAX_SEND_USE_IXON

/* When sending a fax, if the other side says "page bad, retrain
 * requested", sendfax will retry the page. Specifiy here the maximum
 * number of retries (I recommend 3) before hanging up.
 */
#define FAX_SEND_MAX_TRIES 3

/* the device(s) used for faxing
 * multiple devices can be separated by ":", e.g. "tty1a:tty2a"
 * (without (!) leading /dev/)
 */
#define FAX_MODEM_TTYS	"tty1a:tty2a"

/* mailer that accepts destination on command line and mail text on stdin
 */
#ifdef SVR4
# define MAILER		"/usr/bin/mailx"
#else
# define MAILER		"/usr/lib/sendmail"
#endif

/* where to send notify mail about incoming faxes to
 */
#define MAIL_TO		ADMIN

/* after a fax has arrived, mgetty can call a program for further
 * processing of this fax.
 *
 * (e.g.: printing of the fax, sending as MIME mail, displaying in an X
 * window (I expect this to be difficult) ...)
 *
 * It will be called as:
 * <program> <result code> "<sender_id>" <#pgs> <pg1> <pg2>...
 * 
 * Define the name of this program here
 * If you don't want this type of service, do not define it at all
 */

#define FAX_NOTIFY_PROGRAM "/usr/spool/fax/incoming/new_fax"
