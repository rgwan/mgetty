#ident "$Id: policy.h,v 1.29 1993/11/05 22:33:37 gert Exp $ Copyright (c) Gert Doering"

/* this is the file where all configuration for mgetty / sendfax is done
 */

/* main configuration file
 * definitions in this file can override the defaults compiled into the
 * program
 * NOT USED YET.
 */
#define CONFIG_FILE "/usr/local/lib/mgetty-config"

/* If you want to use /etc/gettydefs to set tty flags, define this
 */
/* #define USE_GETTYDEFS */

/* Name of the "gettydefs" file (used only if USE_GETTYDEFS is set)
 */
#define GETTYDEFS "/etc/gettydefs"

/* If no gettydefs "tag" is specified on the command line, use
 * this setting (from GETTYDEFS) as default (only if compiled with
 * USE_GETTYDEFS set)
 */
#define GETTYDEFS_DEFAULT_TAG "n"


/* user id of the "uucp" user. The tty device will be owned by this user,
 * so parallel dial-out of uucico will be possible
 */
#define UUCPID	"uucp"

/* access mode for the line while getty has it - it should be accessible
 * by uucp / uucp, but not by others (imagine someone dialing into your
 * system and using another modem to dial to another country...)
 */
#define FILE_MODE 0660

/* system console - if a severe error happens at startup, mgetty writes
 * a message to this file and aborts
 * On SCO, this may be /dev/syscon!
 */
#define CONSOLE "/dev/console"

/* Default log error level threshold. Possible error levels are
 * L_FATAL, L_ERROR, L_WARN, L_MESG, L_NOISE, L_JUNK (see mgetty.h)
 */
#define LOG_LEVEL L_MESG

/* Whether "\n"s in the modem response should start a new line
 * in the logfile
 */
/* #define LOG_CR_NEWLINE */

/* System administrator - if a severe error happens (lprintf called
 * with log_level L_FATAL) and writing to CONSOLE is not possible,
 * the logfile will be mailed to him
 */
#define ADMIN	"root"

/* System name - printed at login prompt
 * If you do not define this, the uname() call will be used
 */
/* #define SYSTEM	"greenie" */

/* Login prompt
 * The "@", "\\D" and "\\T" escapes will be replaced by SYSTEM, the
 * current date and time, respectively.
 * override with "-p <prompt>" switch
 */
#define LOGIN_PROMPT	"%s!login:"

/* On SVR4, maybe on other systems too, you can cause the 'login' program
 * to prompt with the same string as mgetty did, instead of the standard
 * "login:" prompt. The string will be passed to the 'login' program
 * in the environment variable TTYPROMPT.
 * This is done by putting "login" into a special (brain-dead) "ttymon"-
 * compatibility mode. In that mode, mgetty doesn't ask for a login name
 * at all, so mgetty won't work if you enable that feature and your
 * login program doesn't support it. (You can see if it doesn't work
 * if the user gets a double login prompt or none at all).
 * To use that feature, define ENV_TTYPROMPT.
 */
/* #define ENV_TTYPROMPT */

/* Maximum time before login name has to be entered (in seconds)
 * (after that time a warning will be issued, after that, the call is
 * dropped). To disable that feature, do not define it.
 */
#define MAX_LOGIN_TIME	240

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
#else
# ifdef sgi
# define LOCK	"/usr/spool/locks/LCK..%s"
# endif
#endif

#ifndef LOCK
#define LOCK "/usr/spool/uucp/LCK..%s"
#endif
  
/* Set this to "1" if your system uses binary lock files (i.e., the pid
 * as four byte integer in host byte order written to the lock file)
 * If it is "0", HDB locking will be used - the PID will be written as
 * 10 byte ascii, with a trailing newline
 * (Just check "LOCK" while uucico or pcomm or ... are running to find
 * out what lock files are used on your system)
 */
#define LOCKS_BINARY 0

/* the default speed used by mgetty - override it with "-s <speed>"
 *
 * WARNING: ZyXELs *can* do faxreceive with 38400, but a lot other modems,
 * especially such based on the rockwell chipset can *not*. So, if
 * your fax receive fails mysteriously, timing out waiting for "OK", try
 * setting this to B19200
 */
#define DEFAULT_PORTSPEED	B38400

/* the modem initialization string
 *
 * this sample string is for ZyXELs, for other modems you'll have to
 *     replace &H3 (rts/cts flow control), &K4 (enable v42bis) and &N0
 *     (answer with all known protocols).
 * For instructions how to setup various other modems, look into
 *     mgetty.texi ("modems" section)
 * For some modems, an initial "\d" is needed.
 * If you need a "\" in the modem command, give it as "\\\\".
 * Maybe the best "initialization" would be to setup everything 
 *     properly in the nvram profile, and just send the modem an
 *     "ATZ". I just like to make sure the most important things are
 *     always set...
 * The modem must answer with "OK" (!!!) - otherwise, change mgetty.c
 */
#define MODEM_INIT_STRING	"ATS0=0Q0&D3&H3&N0&K4"

/* the main fax spool directory
 */
#define FAX_SPOOL	"/usr/spool/fax"

/* where incoming faxes go to
 * getty needs write permissions!
 */
#ifdef __STDC__
# define FAX_SPOOL_IN	FAX_SPOOL"/incoming"
#else
# define FAX_SPOOL_IN	"/usr/spool/fax/incoming"
#endif

/* some modems are a little bit slow - after sending a response (OK)
 * to the host, it will take some time before they can accept the next
 * command - specify the amount needed in fax mode here (in
 * milliseconds). Normally, 50 ms should be sufficient. (On a slow
 * machine it may even work without any delay at all)
 */
#define FAX_COMMAND_DELAY 50

/* incoming faxes will be chown()ed to this uid and gid
 */
#define FAX_IN_OWNER	0
#define FAX_IN_GROUP	5

/* if your received faxes are corrupted (missing lines), because the
 * faxmodem does not honor RTS handshake, define this.
 * mgetty will then do XON/XOFF flow control in fax receive mode
 * (I do *not* think that it's necessary)
 */
/* #define FAX_RECEIVE_USE_IXOFF */

/* if your faxmodem switches to 19200 bps just after sending the "+FCON"
 * message to the host, define this. (Not important if you have the
 * portspeed set to 19200 anyway).
 * Some Tornado and Supra modems are know to do this.
 * ZyXELs do *not* do this, except if explicitely told to do so.
 *
 * You can see if this happens if mgetty gets the "+FCON" response,
 * starts the fax receiver, and times out waiting for OK, receiving
 * nothing or just junk.
 */
/* #define FAX_RECEIVE_USE_B19200 */


/* name of the logfile for outgoing faxes
 */
#ifdef __STDC__
# define FAX_LOG		FAX_SPOOL"/Faxlog"
#else
# define FAX_LOG		"/usr/spool/fax/Faxlog"
#endif

/* local station ID
 * 20 character string, most faxmodem allow all ascii characters 32..127,
 * but some do only allow digits and blank
 * AT+FLID=? should tell you what's allowed and what not.
 */
#define FAX_STATION_ID	"49 89 3243328"

/* ------ sendfax-specific stuff follows here -------- */

/* the baudrate used for *sending* faxes. ZyXELs can handle 38400,
 * SUPRAs (and other rockwell-based faxmodems) do not
 * I recommend 38400, since 19200 may be to slow for 14400 bps faxmodems!
 */
#define FAX_SEND_BAUD B38400

/* this is the command to set the modem to use the desired flow control
 * for hardware handshake, this could be &H3 for the ZyXEL, &K3 for
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

/* define mailer that accepts destination on command line and mail text
 * on stdin. For mailers with user friendly interfaces, (such as mail,
 * mailx, elm), include an appropriate subject line in the command
 * definition. If using a mail agent (such as sendmail), that reads
 * mail headers, define NEED_MAIL_HEADERS.
 */
#ifdef SVR4
# define MAILER		"/usr/bin/mailx -s 'Incoming facsimile message'"
#else
# define MAILER		"/usr/lib/sendmail"
# define NEED_MAIL_HEADERS
#endif

/* where to send notify mail about incoming faxes to
 * (remember to create an mail alias if no such user exists!)
 */
#define MAIL_TO		"faxadmin"

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
