/*
 * voice_config.h
 *
 * This is the definition of the voice config file parameters.
 *
 */

#ifdef MAIN
char *voice_config_h = "$Id: config.h,v 1.1 1997/12/16 11:49:14 marc Exp $";
#endif

extern struct conf_voice_data
     {
     struct conf_data ignore1;
     struct conf_data ignore2;
     struct conf_data ignore3;
     struct conf_data ignore4;
     struct conf_data voice_log_level;
     struct conf_data voice_devices;
     struct conf_data port_speed;
     struct conf_data port_timeout;
     struct conf_data dtmf_len;
     struct conf_data dtmf_threshold;
     struct conf_data dtmf_wait;
     struct conf_data rec_compression;
     struct conf_data rec_speed;
     struct conf_data rec_silence_len;
     struct conf_data rec_silence_threshold;
     struct conf_data rec_remove_silence;
     struct conf_data rec_max_len;
     struct conf_data receive_gain;
     struct conf_data transmit_gain;
     struct conf_data rings;
     struct conf_data answer_mode;
     struct conf_data toll_saver_rings;
     struct conf_data rec_always_keep;
     struct conf_data voice_dir;
     struct conf_data message_flag_file;
     struct conf_data receive_dir;
     struct conf_data message_dir;
     struct conf_data message_list;
     struct conf_data backup_message;
     struct conf_data dialout_timeout;
     struct conf_data beep_frequency;
     struct conf_data beep_length;
     struct conf_data raw_data;
     struct conf_data max_tries;
     struct conf_data retry_delay;
     struct conf_data voice_shell;
     struct conf_data button_program;
     struct conf_data call_program;
     struct conf_data dtmf_program;
     struct conf_data message_program;
     struct conf_data do_message_light;
     struct conf_data do_hard_flow;
     struct conf_data force_autodetect;
     struct conf_data watchdog_timeout;
     struct conf_data rec_min_len;
     struct conf_data command_delay;
     struct conf_data ignore_fax_dle;
     struct conf_data dial_timeout;
     struct conf_data enable_command_echo;
     struct conf_data end_of_config;
     } cvd;
