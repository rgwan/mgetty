/* main.c
 *
 * VoiceModem is the program for doing some basic tasks with your voice
 * modem including playing back and recording of voice files. It also
 * supports the shell script execution function to test vgetty scripts
 * and to build special standalone scripts.
 *
 * $Id: main.c,v 1.5 1999/01/30 14:31:17 marcs Exp $
 *
 */

#include "vm.h"

/*
 * Global variables definition
 */

int dtmf_mode = IGNORE_DTMF;
char dtmf_string_buffer[VOICE_BUF_LEN] = "";
int use_on_hook_off_hook = FALSE;
int start_action = TRUE;
char *DevID = "/dev/null";
char *Device = NULL;

/*
 * Main function
 */

int main(int argc, char *argv[])
     {
     int option;
     char *command = argv[1];
     int verbose = FALSE;
     int voice_device = DIALUP_LINE;
     int result = OK;

     check_system();
     voice_config("vm", "");
     voice_register_event_handler(handle_event);

     if ((argc < 2) || (strcmp(command, "help") == 0))
          usage();

     if ((strcmp(command, "beep") != 0) &&
      (strcmp(command, "diagnostics") != 0) &&
      (strcmp(command, "dial") != 0) && (strcmp(command, "play") != 0) &&
      (strcmp(command, "record") != 0) && (strcmp(command, "shell") != 0) &&
      (strcmp(command, "wait") != 0) && (strcmp(command, "dtmf") != 0))
 
          usage();

     optind = 2;

     while ((option = getopt(argc, argv, "c:hil:mstvwx:HL:PRS:T:V:")) != EOF)
          {

          switch (option)
               {
               case 'c':
                    conf_set_int(&cvd.rec_compression, atoi(optarg));
                    break;
               case 'i':
                    voice_device = INTERNAL_MICROPHONE;
                    break;
               case 'l':
                    conf_set_string(&cvd.voice_devices, optarg);
                    break;
               case 'm':
                    voice_device = EXTERNAL_MICROPHONE;
                    break;
               case 's':
                    voice_device = INTERNAL_SPEAKER;
                    break;
               case 't':
                    voice_device = DIALUP_LINE;
                    break;
               case 'v':
                    verbose = TRUE;
                    break;
               case 'w':
                    use_on_hook_off_hook = TRUE;
                    break;
               case 'x':
                    conf_set_int(&cvd.voice_log_level, atoi(optarg));
                    log_set_llevel(cvd.voice_log_level.d.i);
                    break;
               case 'H':
                    voice_device = LOCAL_HANDSET;
                    break;
               case 'L':
                    conf_set_int(&cvd.rec_max_len, atoi(optarg));
                    break;
               case 'P':
                    dtmf_mode = READ_DTMF_DIGIT;
                    break;
               case 'R':
                    dtmf_mode = READ_DTMF_STRING;
                    break;
               case 'S':
                    conf_set_string(&cvd.voice_shell, optarg);
                    break;
               case 'T':
                    conf_set_int(&cvd.rec_silence_len, atoi(optarg));
                    break;
               case 'V':
                    conf_set_int(&cvd.rec_silence_threshold, atoi(optarg));
                    break;
               default:
                    usage();
               };

          };

     if (strcmp(command, "diagnostics") == 0)

          if (optind == argc)
               {
               fprintf(stderr, "%s: no device name given for diagnostics\n",
                program_name);
               exit(1);
               }
          else
               {
               char *test_command[] = {"ATI", "ATI1", "ATI2", "ATI3",
                "ATI4", "ATI5", "ATI6", "ATI7", "ATI8", "ATI9",
                "AT+FMI?", "AT+FMM?", "AT+FMR?", NULL};
               char buffer[VOICE_BUF_LEN];
               char *device;
               TIO tio;
               int tries = 0;
               int i;

               device = argv[optind];
               conf_set_string(&cvd.voice_devices, device);
               voice_modem = &no_modem;
               rom_release = 0;
               printf("*\n* Diagnostics for device /dev/%s\n*\n", device);
               printf("* vgetty %s\n", vgetty_version);

               while (((result = voice_open_device()) != OK) && ((++tries) <
                cvd.max_tries.d.i))
                    sleep(cvd.retry_delay.d.i);

               if (result != OK)
                    {
                    fprintf(stderr, "%s: could not open a voice modem device\n",
                     program_name);
                    exit(FAIL);
                    };

               tio_get(voice_fd, &tio);
               printf("* port speed is set to %d baud.\n*\n\n",
                tio_get_speed(&tio));

               for (i = 0; test_command[i] != NULL; i++)
                    {
                    int first;

                    printf("%-7s --> ", test_command[i]);
                    voice_command(test_command[i], "");
                    first = TRUE;

                    do
                         {

                         do
                              {

                              if (voice_read(buffer) != OK)
                                   {
                                   printf("could not read from modem");
                                   exit(1);
                                   };

                              }
                         while (strlen(buffer) == 0);

                         if (first)
                              first = FALSE;
                         else
                              printf("%-7s     ", " ");

                         printf("%s\n", buffer);
                         }
                    while ((voice_analyze(buffer, "OK|ERROR", TRUE) &
                     VMA_USER) == 0);

                    };

               exit(0);
               };

     if (getenv("VOICE_PID") == NULL)
          {
          int tries = 0;

          while (((result = voice_open_device()) != OK) && ((++tries) <
           cvd.max_tries.d.i))
               sleep(cvd.retry_delay.d.i);

          if (result != OK)
               {
               fprintf(stderr, "%s: could not open a voice modem device\n",
                program_name);
               exit(FAIL);
               };

          voice_mode_on();

          if ((strcmp(command, "shell") != 0) &&
           (strcmp(command, "dial") != 0))

               if (voice_set_device(voice_device) != OK)
                    {
                    fprintf(stderr, "%s: could not set voice device\n",
                     program_name);
                    exit(FAIL);
                    };

          }
     else if (strcmp(command, "shell") != 0)
          {
          fprintf(stderr, "%s: can not execute %s inside a voice shell\n",
           program_name, program_name);
          exit(FAIL);
          };

     if (strcmp(command, "beep") == 0)
          {
          int frequency = cvd.beep_frequency.d.i;
          int length = cvd.beep_length.d.i;

          if (optind < argc)
               frequency = atoi(argv[optind++]);

          if (optind < argc)
               length = atoi(argv[optind]);

          if (verbose)
               fprintf(stderr,
                "%s: beep of frequency %d and length %d (in 0.1sec)\n",
                program_name, frequency, length);

          voice_beep(frequency, length);
          };

     if (strcmp(command, "dial") == 0)
          {

          if (optind == argc)
               fprintf(stderr, "%s: no phone number given\n",
                program_name);
          else
               {

               if (verbose)
                    fprintf(stderr, "%s: dialing number %s\n",
                     program_name, argv[optind]);

               voice_dial((void *) argv[optind]);
               };

          };

     if (strcmp(command, "play") == 0)
          {

          if (use_on_hook_off_hook)
               {
               start_action = FALSE;
               printf("Waiting to start playing. ");
               printf("Please pick up local handset...\n");
               voice_wait(60);
               }

          if (start_action) {

               while (optind < argc)
                    {

                    if (verbose)
                         fprintf(stderr, "%s: playing voice file %s\n",
                          program_name, argv[optind]);

                    if (voice_play_file(argv[optind]) == INTERRUPTED)
                         break;

                    optind++;
                    };

               }
          else
               printf("Phone wasn't picked up, exiting.\n");

          };

     if (strcmp(command, "record") == 0)
          {

          if (optind == argc)
               fprintf(stderr, "%s: no filename given for recording\n",
                program_name);
          else
               {

               if (verbose)
                    fprintf(stderr, "%s: recording voice file %s\n",
                     program_name, argv[optind]);

               if (use_on_hook_off_hook)
                    {
                    start_action = FALSE;
                    printf("Waiting to start recording. ");
                    printf("Please pick up local handset...\n");
                    voice_wait(60);
                    }

               if (start_action)
                    {
                    printf("Recording message...\n");
                    voice_record_file(argv[optind]);
                    printf("Recording complete.\n");
                    }
               else
                    printf("Phone wasn't picked up, exiting.\n");

               };

          };

     if (strcmp(command, "shell") == 0)
          {
          int i;
          char **shell_arguments;

          if (optind == argc)
               voice_execute_shell_script("", NULL);
          else
               {
               shell_arguments = (char**) malloc((argc - optind + 1) *
                sizeof(char*));

               for (i = 0; i < (argc - optind - 1); i++)
                    shell_arguments[i] = argv[optind + i + 1];

               shell_arguments[i] = NULL;
               voice_execute_shell_script(argv[optind], shell_arguments);
               };

          };

     if (strcmp(command, "wait") == 0)
          {
          int length = cvd.rec_silence_len.d.i / 10;

          if (optind < argc)
               length = atoi(argv[optind]);

          if (verbose)
               fprintf(stderr, "%s: waiting for %d seconds\n",
                program_name, length);

          voice_wait(length);
          };



     if (strcmp(command, "dtmf") == 0)
          {

          if (optind == argc)
               fprintf(stderr, "%s: no number given\n",
                program_name);
          else
               {


               if (verbose)
                    fprintf(stderr, "%s: playing number %s\n",
                     program_name, argv[optind]);

               voice_play_dtmf(argv[optind]);
               };

          };


     if (getenv("VOICE_PID") == NULL)
          {
          voice_set_device(NO_DEVICE);
          voice_mode_off();
          voice_close_device();
          };

     voice_unregister_event_handler();
     exit(result);
     }
