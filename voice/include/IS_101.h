/*
 * voice_IS_101.h
 *
 * Defines the functions implemented in the generic IS 101 driver.
 *
 */

#ifdef MAIN
char *voice_IS_101_h = "$Id: IS_101.h,v 1.1 1997/12/16 11:49:10 marc Exp $";
#endif

extern int IS_101_answer_phone (void);
extern int IS_101_beep (int frequency, int duration);
extern int IS_101_dial (char* number);
extern int IS_101_handle_dle (char code);
extern int IS_101_init (void);
extern int IS_101_message_light_off (void);
extern int IS_101_message_light_on (void);
extern int IS_101_play_file (int fd);
extern int IS_101_record_file (int fd);
extern int IS_101_set_buffer_size (int size);
extern int IS_101_set_compression (int *compression, int *speed);
extern int IS_101_set_device (int device);
extern int IS_101_stop_dialing (void);
extern int IS_101_stop_playing (void);
extern int IS_101_stop_recording (void);
extern int IS_101_stop_waiting (void);
extern int IS_101_switch_to_data_fax (char* mode);
extern int IS_101_voice_mode_off (void);
extern int IS_101_voice_mode_on (void);
extern int IS_101_wait (int timeout);
