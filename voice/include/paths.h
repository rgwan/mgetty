/*
 * voice_paths.h
 *
 * This file contains the position of the configuration file and of
 * the logfiles. Change this for your installation.
 *
 */

#ifdef MAIN
char *voice_paths_h = "$Id: paths.h,v 1.2 1998/01/21 10:24:09 marc Exp $";
#endif

/*
 * Filename of the voice configuration file.
 */

#define VOICE_CONFIG_FILE "voice.conf"

/*
 * Filename of the logfile for vgetty. The "%s" will be replaced by
 * the device name.
 */

#define VGETTY_LOG_PATH "/var/log/vgetty.%s"

/*
 * Filename of the logfile for vm.
 */

#define VM_LOG_PATH "/var/log/vm.log"

/*
 * Filename of the logfile for the pvf tools.
 */

#define PVF_LOG_PATH "/var/log/pvf.log"
