/*
 * voice_default.h
 *
 * This file contains the default values for vgetty, vm and the pvf
 * tools. All of these values can be changed by the configuration
 * file and by command line arguments.
 *
 */

#ifdef MAIN
char *voice_default_h = "$Id: default.h,v 1.1 1997/12/16 11:49:15 marc Exp $";
#endif

/*
 * Common defaults
 * ---------------
 */

/*
 * Default log level for the voice programs.
 */

#define VOICE_LOG_LEVEL L_MESG

/*
 * Default shell to invoke for shell scripts. The default is "/bin/sh"
 */

#define VOICE_SHELL "/bin/sh"

/*
 * Default port speed. The bps rate must be high enough for the compression
 * mode used. Note that this is an integer, not one of the Bxxxx defines.
 * This must be set to 38400 for Rockell modems. The default value is 38400.
 */

#define PORT_SPEED 38400

/*
 * Default port timeout in seconds for a read or write operation. The
 * default value is 10 seconds.
 */

#define PORT_TIMEOUT 10

/*
 * Default timeout for a dialout in seconds. The default value is 90 seconds.
 */

#define DIAL_TIMEOUT 90

/*
 * Delay before sending a new voice command to the modem in milliseconds.
 * The default is 100 milliseconds.
 */

#define COMMAND_DELAY 100

/*
 * Minimum length of detected DTMF tones, in milliseconds. This is
 * currently only supported by ZyXel modems with a ROM release of 6.12
 * or above. The default is 30 milliseconds.
 */

#define DTMF_LEN 30

/*
 * DTMF tone detection threshold in percent (0% to 100%). Increase this
 * if the modem erroneously detects DTMF tones, decrease it if it fails to
 * detect real ones. This is currently only supported by ZyXel modems
 * with a ROM release of 6.12 or above. The default is 40%.
 */

#define DTMF_THRESHOLD 40

/*
 * Time to wait for a DTMF tone to arrive when recording or waiting
 * for DTMF input in seconds. The default is to wait for 7 seconds.
 */

#define DTMF_WAIT 7

/*
 * In Australia the frequency of the busy signal is the same as the
 * frequency of the fax calling tone. This causes problems on at least
 * some modems. They report a fax calling tone, when there is a busy
 * signal in reality. To help those user, vgetty will ignore any fax
 * calling tone detected by the modem, when this option is set.
 *
 * The following companys suffer from this problem:
 * - Telstra (formerly Telecom Australia)
 * - Optus
 * - Austel (regulatory authority)
 *
 * The default is of course off.
 */

#define IGNORE_FAX_DLE FALSE

/*
 * Output recorded voice samples without header and expect raw voice
 * data on input for playback. This feature is turned off by default.
 */

#define RAW_DATA FALSE

/*
 * This is the default compression mode for vgetty for incoming voice
 * messages and for the recording option of vm. The mode 0 is a special
 * mode, that will automatically choose a sane default value for every
 * modem. The default is 0.
 */

#define REC_COMPRESSION 0

/*
 * This is the default recording speed for vgetty for incoming voice
 * messages and for the recording option of vm. It is the number of samples
 * per second. The speed 0 is a special speed, that will automatically
 * choose a sane default value for every modem. The default is 0.
 */

#define REC_SPEED 0

/*
 * Silence detection length in 0.1 seconds. If the modem detects silence
 * for this time, it sends a silence detect to the host. Default is
 * 7 seconds (70 * 0.1 seconds).
 */

#define REC_SILENCE_LEN 70

/*
 * Silence detection threshold in percent (0% to 100%). Increase this value
 * if you have a noisy phone line and the silence detection doesn't work
 * reliably. The default is 40%.
 */

#define REC_SILENCE_THRESHOLD 40

/*
 * If REC_REMOVE_SILENCE is enabled, the trailing silence of an incoming
 * voice message as detected by the modem will be deleted. This might
 * cause you to miss parts of a message if the silence threshold is
 * high and the caller is talking very quietly. To be on the safe side,
 * don't define this. This feature is turned off by default.
 */

#define REC_REMOVE_SILENCE FALSE

/*
 * Maximum recording length in seconds. Hang up if somebody talks
 * longer than this. Default is 5 minutes (300 seconds).
 */

#define REC_MAX_LEN 300

/*
 * Minimum recording length in seconds. Some modems can not detect
 * data or fax modems, so we use the recording time, to decide,
 * what it is. This feature is by default disabled.
 */

#define REC_MIN_LEN 0

/*
 * Enable hardware flow in record and playback mode if the modem
 * supports it. This option is by default on.
 */

#define DO_HARD_FLOW TRUE

/*
 * When switching to data or fax mode, always switch to fax mode and
 * enable autodetection of data/fax. Some modems report wrong DLE codes
 * and so the predetection with DLE codes does not work.
 */

#define FORCE_AUTODETECT FALSE

/*
 * Default timeout for the voice watchdog. If this timer expires, the
 * running program will be terminated. The default is 60 seconds.
 */

#define WATCHDOG_TIMEOUT 60

/*
 * Some modems support setting the receive gain. This value can be set in
 * percent (0% to 100%). 0% is off, 100% is maximum. To use the modem
 * default value set this to -1. The default is -1.
 *
 */

#define RECEIVE_GAIN -1

/*
 * Some modems support setting the transmit gain. This value can be set in
 * percent (0% to 100%). 0% is off, 100% is maximum. To use the modem
 * default value set this to -1. The default is -1.
 *
 */

#define TRANSMIT_GAIN -1

/*
 * Usually vgetty will enable the command echo from the modem. Since some
 * modems sometimes forget this echo, vgetty complains about it. With this
 * option you can turn the command echo off, which might make things more
 * reliable, but bugs are much harder to trace. So don't ever think about
 * mailing me a bug report with command echo turned off. I will simply ignore
 * it. The default is to enable command echo.
 *
 */

#define ENABLE_COMMAND_ECHO TRUE

/*
 * Default values for vgetty
 * -------------------------
 */

/*
 * Default number of rings to wait before picking up the phone.
 *
 * Instead of a number, you can also give a file name, that contains
 * a single number with the desired number of rings. Vgetty will
 * automatically append the name of the modem device to the file name.
 * The file name must be an absolut path starting with a leading "/".
 * E.g. #define RINGS "/etc/answer" and the modem device is ttyS0, will
 * lead to the file name "/etc/answer.ttyS0".
 *
 * The default is "3"
 */

#define RINGS "3"

/*
 * Default answer mode when vgetty picks up the phone after incoming
 * rings.
 *
 * If this string starts with a "/", vgetty gets the answer mode from
 * the file name given in the string.
 *
 * The default is "voice:fax:data".
 */

#define ANSWER_MODE "voice:fax:data"

/*
 * If vgetty knows that there are new messages (the flag file exists),
 * it will turn on the AA lamp on an external modem and enable the toll
 * saver - it will answer the phone TOLL_SAVER_RINGS earlier than the
 * default. This feature is turned off by default.
 */

#define TOLL_SAVER_RINGS 0

/*
 * Should the recorded voice message file be kept even if data, fax or
 * DTMF codes were detected? If this is set, vgetty never deletes
 * a recording, if it is not set it will delete the recording, if an
 * incoming data or fax call is detected or if DTMF codes were send. Also
 * this should work in nearly every situation, it makes You loose the
 * recording, if the caller "plays" with DTMF codes to make the message
 * even more beautiful. This feature is enabled by default.
 */

#define REC_ALWAYS_KEEP TRUE

/*
 * The R_FFT_PROGRAM is responsible for distinguishing voice and
 * data calls if the line is not silent. It checks the power spectrum
 * of the sound data and, if it only contains few frequencies, sends
 * a SIGUSR2 to vgetty to make it stop recording. This is also quite
 * effective against dial tones. (Code by ulrich@gaston.westfalen.de)
 *
 * It needs to convert the voice data, i.e. it currently
 * works only for ZyXEL adpcm-2 and adpcm-3. It is automatically
 * disabled if it can't deal with the voice data.
 *
 * There are a few configurable parameters, check the top of
 * pvffft.c for details. The defaults seem to work quite well.
 *
 * Undefine R_FFT_PROGRAM if you don't want this, i.e. if you
 * have a slow machine that can't handle the additional CPU load.
 */

#define FFT_PROGRAM ""
/* #define FFT_PROGRAM "vg_fft" */

/*
 * Primary voice directory for vgetty.
 */

#define VOICE_DIR "/var/spool/voice"

/*
 * Location of the flag file for new incoming messages relative to the
 * primary voice directory.
 */

#define MESSAGE_FLAG_FILE ".flag"

/*
 * Location where vgetty stores the incoming voice messages relative to
 * the primary voice directory.
 */

#define RECEIVE_DIR "incoming"

/*
 * Directory containing the messages for vgetty (greeting, handling the
 * answering machine) relative to the primary voice directory.
 */

#define MESSAGE_DIR "messages"

/*
 * Name of the file in MESSAGE_DIR that contains the names of
 * the greeting message files (one per line, no white space).
 */

#define MESSAGE_LIST "Index"

/*
 * Filename of a backup greeting message in MESSAGE_DIR (used if
 * the random selection fails to find a message).
 */

#define BACKUP_MESSAGE "standard"

/*
 * The programs defined below get called by vgetty.
 *
 * Define an empty program name, if you want to disabled one of those
 * programs.
 */

/*
 * There are two separate uses for the Data/Voice button:
 *
 * - If a RING was detected recently, answer the phone in fax/data mode
 * - Otherwise, call an external program to play back messages
 *
 * If you don't define BUTTON_PROGRAM, vgetty will always pick up
 * the phone if Data/Voice is pressed.
 *
 * The default value is "".
 */

#define BUTTON_PROGRAM   ""

/*
 * Program called when the phone is answered, this is instead
 * of the normal behaviour. Don't define this unless you want
 * to e.g. set up a voice mailbox where the normal answering
 * machine behaviour would be inappropiate. The C code is probably
 * more stable and uses less resources.
 *
 * The default value is "".
 */

#define CALL_PROGRAM ""

/*
 * Program called when a DTMF command in the form '*digits#' is received.
 * The argument is the string of digits received (without '*' and '#').
 * The default value is "dtmf.sh".
 */

#define DTMF_PROGRAM "dtmf.sh"

/*
 * Program called when a voice message has been received.
 * The argument is the filename of the recorded message.
 * The default value is "".
 */

#define MESSAGE_PROGRAM ""

/*
 * Should vgetty use the AA LED on some modems to indicate that new
 * messages have arrived? This is done by setting the modem register
 * S0 to a value of 255. Some modems have a maximum number of rings
 * allowed and autoanswer after this, so they can not use this feature.
 * This option is by default off.
 */

#define DO_MESSAGE_LIGHT FALSE

/*
 * Default values for vm
 * ---------------------
 */

/*
 * Frequency for the beep command in Hz. The default is 933Hz.
 */

#define BEEP_FREQUENCY 933

/*
 * Length for the beep command in 0.01sec. The default is 1.5 seconds
 * (150 * 0.01 seconds).
 */

#define BEEP_LENGTH 150

/*
 * Number of tries to open a voice modem device. The default is 3.
 */

#define MAX_TRIES 3

/*
 * Delay between two tries to open a voice device in seconds. The default
 * is 5 seconds.
 */

#define RETRY_DELAY 5

/*
 * Timeout for a dialout operation in seconds. The default is 90 seconds.
 */

#define DIALOUT_TIMEOUT 90

/*
 * Default values for the pvf tools
 * --------------------------------
 */

/*
 * There are currently no defaults.
 */
