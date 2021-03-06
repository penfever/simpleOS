#include <stdio.h>
#include <stdlib.h>
#include "SDHC_FAT32_Files.h"

#ifndef _SIMPLE_SHELL_H
#define _SIMPLE_SHELL_H
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef MAXLEN
#define MAXLEN 256 //accepts chars 0->255, plus newline 256
#endif
#define MAXARGS 32 //accepts args 0->32
#define SHELLPID 1
#define NULLCHAR '\0'
#define HELPBLOCK "Welcome to the simple shell! \r\n"\
                  "For additional details on command syntax and implementation, please check the readme.md on Github \r\n"\
                  "Here are a few commands you can try: \r\n"\
                  "exit will exit this shell. \r\n"\
                  "help will show you a list of commands and how they work. \r\n"\
                  "echo will echo back whatever words follow the command itself. \r\n"\
                  "date will print the current date and time (GMT, MS-DOS EPOCH) or set the current system time in MS according to an integer input by the user. \r\n"\
                  "malloc allocates memory. \r\n"\
                  "free frees memory. \r\n"\
                  "memset sets an allocated memory region to a value. \r\n"\
                  "memchk checks that an allocated memory region is set to a value. \r\n"\
                  "memorymap prints a map of all allocated memory. \r\n"\
                  "malfree performs 100 semi-random allocations and frees of memory in order to test memory integrity. \r\n"\
                  "fopen opens a file or device. \r\n"\
                  "fclose closes a file or device. \r\n"\
                  "fgetc and fgets retrieve characters or strings from a file or device. \r\n"\
                  "fputc and fputs send characters or strings to a file or device. \r\n"\
                  "create and delete create and delete files on the MicroSD card. \r\n"\
                  "ls 0 lists the current directory contents. ls 1 lists the current directory contents with all attributes. \r\n"\
                  "seek sets the file cursor to a particular position in a file. \r\n"\
                  "touch2led activates the LEDs based on whether you are touching their corresponding touch sensor. touch all 4 to exit. \r\n"\
                  "pot2ser continuously outputs the potentiometer value to STDOUT \r\n"\
				          "therm2ser continuously outputs the thermistor value to STDOUT \r\n"\
				          "pb2LED toggles LEDs based on inputs from pushbuttons \r\n"\
				          "catfile prints the contents of a file to STDOUT \r\n"\
				          "cat2file copies characters from serial input to the specified <file> in the root directory \r\n"\
                  "spawn creates a new background process and returns to the shell prompt. The following commands support spawn: flashled, touch2led, cat2file, pb2led, busywait, uartsendmsg\r\n"\
                  "kill kills a target process, with the target indicated by its pid number \r\n"\
                  "ps lists all processes currently running on the OS, including their pids and current uptime \r\n"\
                  "uartsendmsg sends a message whenever pushbutton 2 is depressed \r\n"\
                  "busywait loops forever and does nothing \r\n"\
                  "multitask spawns cat2file, flashled and uartsendmsg. Terminate with ctrl+D \r\n"\
                  "synth launches the built-in synthesizer. a-g plays notes, w switches waveforms, q to quit \r\n"\
                  "flashled flashes an LED on and off at an interval determined by the user (1-20 in 50ms intervals) \r\n"

#define NUMCOMMANDS (int)(sizeof(commands)/sizeof(commands[0]))
#define NUMESCAPES (int)(sizeof(escapechars)/sizeof(escapechars[0]))
#define BACKSLASH 92
#ifndef CONSOLEIO
#define CONSOLEIO 0
#endif
#ifndef UARTIO
#define UARTIO 1
#endif
#define EOT 4

struct commandEntry {
    char *name;
    int (*functionp)(int argc, char *argv[]);
};

struct escape_chars {
    char c;
    int ascii;
};

extern struct escape_chars escapechars[];
extern struct commandEntry commands[];
extern int g_noFS;
extern file_descriptor io_dev;
extern uint8_t g_timerExpired;

/*shell command function prototypes*/

int cmd_date(int argc, char *argv[]);
int cmd_echo(int argc, char *argv[]);
int cmd_exit(int argc, char *argv[]);
int cmd_help(int argc, char *argv[]);
int cmd_clockdate(int argc, char *argv[]);
int cmd_malloc(int argc, char *argv[]);
int cmd_free(int argc, char *argv[]);
int cmd_malfree(int argc, char *argv[]);
int cmd_memset(int argc, char *argv[]);
int cmd_memchk(int argc, char *argv[]);
int cmd_memorymap(int argc, char *argv[]);
int cmd_fopen(int argc, char *argv[]);
int cmd_fclose(int argc, char *argv[]);
int cmd_create(int argc, char *argv[]);
int cmd_delete(int argc, char *argv[]);
int cmd_fgetc(int argc, char *argv[]);
int cmd_fgets(int argc, char *argv[]);
int cmd_fputc(int argc, char *argv[]);
int cmd_fputs(int argc, char *argv[]);
int cmd_seek(int argc, char *argv[]);
int cmd_ls(int argc, char *argv[]);
int cmd_touch2led(int argc, char* argv[]);
int cmd_pot2ser(int argc, char* argv[]);
int cmd_therm2ser(int argc, char* argv[]);
int cmd_pb2led(int argc, char* argv[]);
int cmd_catfile(int argc, char* argv[]);
int cmd_cat2file(int argc, char* argv[]);
int cmd_flashled(int argc, char* argv[]);
int cmd_shell(int argc, char* argv[]);
int cmd_spawn(int argc, char* argv[]);
int cmd_kill(int argc, char* argv[]);
int cmd_multitask(int argc, char* argv[]);
int cmd_ps(int argc, char* argv[]);
int cmd_uartsendmsg(int argc, char* argv[]);
int cmd_busywait(int argc, char* argv[]);
int cmd_synth(int argc, char* argv[]);

/*shell string processing function prototypes*/

int parse_string(char* user_cmd, char* user_cmd_clean, int arg_len[], uint16_t cmdLen);
int quote_check(char* user_cmd, uint16_t cmdLen);
void quote_char(char* user_cmd, char* user_cmd_clean, int* quote_len);
void escape_char(char* user_cmd, char* user_cmd_clean, int* cleanLen);

int shell(void);

#endif
