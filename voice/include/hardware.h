/*
 * voice_hardware.h
 *
 * Defines the structure with data and routines for the hardware drivers.
 *
 */

#ifdef MAIN
char *voice_hardware_h = "$Id: hardware.h,v 1.1 1997/12/16 11:49:17 marc Exp $";
#endif

/*
 * Structure with voice modem hardware informations and functions
 */

typedef struct
     {
     char *name;
     char *rmd_name;
     int (*answer_phone) (void);
     int (*beep) (int frequency, int duration);
     int (*dial) (char* number);
     int (*handle_dle) (char code);
     int (*init) (void);
     int (*message_light_off) (void);
     int (*message_light_on) (void);
     int (*play_file) (int fd);
     int (*record_file) (int fd);
     int (*set_compression) (int *compression, int *speed, int *bits);
     int (*set_device) (int device);
     int (*stop_dialing) (void);
     int (*stop_playing) (void);
     int (*stop_recording) (void);
     int (*stop_waiting) (void);
     int (*switch_to_data_fax) (char* mode);
     int (*voice_mode_off) (void);
     int (*voice_mode_on) (void);
     int (*wait) (int timeout);
     } voice_modem_struct;

/*
 * Global variables
 */

extern voice_modem_struct *voice_modem;
#define voice_modem_name voice_modem->name
#define voice_modem_rmd_name voice_modem->rmd_name
extern int voice_modem_state;
extern int rom_release;

/*
 * Hardware handle event functions
 */

extern voice_modem_struct no_modem;
extern voice_modem_struct Cirrus_Logic;
extern voice_modem_struct Dolphin;
extern voice_modem_struct Dr_Neuhaus;
extern voice_modem_struct Elsa;
extern voice_modem_struct ISDN4Linux;
extern voice_modem_struct Rockwell;
extern voice_modem_struct Sierra;
extern voice_modem_struct UMC;
extern voice_modem_struct US_Robotics;
extern voice_modem_struct ZyXEL_1496;
extern voice_modem_struct ZyXEL_2864;
