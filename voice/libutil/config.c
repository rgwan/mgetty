/*
 * config.c
 *
 * This file is responsible for setting the vgetty, vm and pvf tools
 * options to a default value. Then it parses the configuration file and
 * after that the command line options.
 *
 */

#include "../include/voice.h"

char *libvoice_config_c = "$Id: config.c,v 1.1 1997/12/16 12:20:52 marc Exp $";

/*
 * Define the global voice variables.
 */

struct conf_voice_data cvd =
     {
     {"part", {0}, CT_KEYWORD, C_IGNORE},
     {"program", {0}, CT_KEYWORD, C_IGNORE},
     {"port", {0}, CT_KEYWORD, C_IGNORE},
     {"ring_type", {0}, CT_KEYWORD, C_IGNORE},
     {"voice_log_level", {VOICE_LOG_LEVEL}, CT_INT, C_PRESET},
     {"voice_devices", {(p_int) ""}, CT_STRING, C_EMPTY},
     {"port_speed", {PORT_SPEED}, CT_INT, C_PRESET},
     {"port_timeout", {PORT_TIMEOUT}, CT_INT, C_PRESET},
     {"dtmf_len", {DTMF_LEN}, CT_INT, C_PRESET},
     {"dtmf_threshold", {DTMF_THRESHOLD}, CT_INT, C_PRESET},
     {"dtmf_wait", {DTMF_WAIT}, CT_INT, C_PRESET},
     {"rec_compression", {REC_COMPRESSION}, CT_INT, C_PRESET},
     {"rec_speed", {REC_SPEED}, CT_INT, C_PRESET},
     {"rec_silence_len", {REC_SILENCE_LEN}, CT_INT, C_PRESET},
     {"rec_silence_threshold", {REC_SILENCE_THRESHOLD}, CT_INT, C_PRESET},
     {"rec_remove_silence", {REC_REMOVE_SILENCE}, CT_BOOL, C_PRESET},
     {"rec_max_len", {REC_MAX_LEN}, CT_INT, C_PRESET},
     {"receive_gain", {RECEIVE_GAIN}, CT_INT, C_PRESET},
     {"transmit_gain", {TRANSMIT_GAIN}, CT_INT, C_PRESET},
     {"rings", {(p_int) RINGS}, CT_STRING, C_PRESET},
     {"answer_mode", {(p_int) ANSWER_MODE}, CT_STRING, C_PRESET},
     {"toll_saver_rings", {TOLL_SAVER_RINGS}, CT_INT, C_PRESET},
     {"rec_always_keep", {REC_ALWAYS_KEEP}, CT_BOOL, C_PRESET},
     {"voice_dir", {(p_int) VOICE_DIR}, CT_STRING, C_PRESET},
     {"message_flag_file", {(p_int) MESSAGE_FLAG_FILE}, CT_STRING, C_PRESET},
     {"receive_dir", {(p_int) RECEIVE_DIR}, CT_STRING, C_PRESET},
     {"message_dir", {(p_int) MESSAGE_DIR}, CT_STRING, C_PRESET},
     {"message_list", {(p_int) MESSAGE_LIST}, CT_STRING, C_PRESET},
     {"backup_message", {(p_int) BACKUP_MESSAGE}, CT_STRING, C_PRESET},
     {"dialout_timeout", {DIALOUT_TIMEOUT}, CT_INT, C_PRESET},
     {"beep_frequency", {BEEP_FREQUENCY}, CT_INT, C_PRESET},
     {"beep_length", {BEEP_LENGTH}, CT_INT, C_PRESET},
     {"raw_data", {RAW_DATA}, CT_BOOL, C_PRESET},
     {"max_tries", {MAX_TRIES}, CT_INT, C_PRESET},
     {"retry_delay", {RETRY_DELAY}, CT_INT, C_PRESET},
     {"voice_shell", {(p_int) VOICE_SHELL}, CT_STRING, C_PRESET},
     {"button_program", {(p_int) BUTTON_PROGRAM}, CT_STRING, C_PRESET},
     {"call_program", {(p_int) CALL_PROGRAM}, CT_STRING, C_PRESET},
     {"dtmf_program", {(p_int) DTMF_PROGRAM}, CT_STRING, C_PRESET},
     {"message_program", {(p_int) MESSAGE_PROGRAM}, CT_STRING, C_PRESET},
     {"do_message_light", {DO_MESSAGE_LIGHT}, CT_BOOL, C_PRESET},
     {"do_hard_flow", {DO_HARD_FLOW}, CT_BOOL, C_PRESET},
     {"force_autodetect", {FORCE_AUTODETECT}, CT_BOOL, C_PRESET},
     {"watchdog_timeout", {WATCHDOG_TIMEOUT}, CT_INT, C_PRESET},
     {"rec_min_len", {REC_MIN_LEN}, CT_INT, C_PRESET},
     {"command_delay", {COMMAND_DELAY}, CT_INT, C_PRESET},
     {"ignore_fax_dle", {IGNORE_FAX_DLE}, CT_BOOL, C_PRESET},
     {"dial_timeout", {DIAL_TIMEOUT}, CT_INT, C_PRESET},
     {"enable_command_echo", {ENABLE_COMMAND_ECHO}, CT_BOOL, C_PRESET},
     {NULL, {(p_int) ""}, CT_STRING, C_EMPTY}
     };

int rom_release = 0;
voice_modem_struct *voice_modem;
int voice_modem_state = IDLE;
char *program_name = NULL;
int voice_fd = NO_VOICE_FD;
int messages_waiting_ack = 0;
char voice_config_file[VOICE_BUF_LEN] = "";

int voice_config _P2((new_program_name, DevID), char *new_program_name,
 char *DevID)
     {
     char *log_path = NULL;

     program_name = new_program_name;
     voice_modem = &no_modem;
     make_path(voice_config_file, CONF_DIR, VOICE_CONFIG_FILE);
     lprintf(L_MESG, "reading generic configuration from config file %s",
      voice_config_file);
     get_config(voice_config_file, (conf_data *) &cvd, "part", "generic");
     log_set_llevel(cvd.voice_log_level.d.i);

     if (strcmp(program_name, "pvf") == 0)
          log_path = PVF_LOG_PATH;

     if (strcmp(program_name, "vm") == 0)
          log_path = VM_LOG_PATH;

     if (strcmp(program_name, "vgetty") != 0)
          log_init_paths(program_name, log_path, NULL);

     log_set_llevel(VOICE_LOG_LEVEL);

     if (strcmp(program_name, "vgetty") != 0)
          {
          lprintf(L_MESG, "vgetty: %s", vgetty_version);
          lprintf(L_MESG, "mgetty: %s", mgetty_version);
          };

     lprintf(L_MESG, "reading program %s configuration from config file %s",
      program_name, voice_config_file);
     get_config(voice_config_file, (conf_data *) &cvd, "program",
      program_name);
     log_set_llevel(cvd.voice_log_level.d.i);

     if (strcmp(program_name, "vgetty") == 0)
          {
          lprintf(L_MESG, "reading port %s configuration from config file %s",
           DevID, voice_config_file);
          get_config(voice_config_file, (conf_data *) &cvd, "port", DevID);
          log_set_llevel(cvd.voice_log_level.d.i);
          };

     return(OK);
     }
