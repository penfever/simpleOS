#include <stdio.h>
#include <stdlib.h>

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
#define NULLCHAR '\0'
#define HELPBLOCK "Welcome to the simple shell!\n"\
                  "Here are a few commands you can try: \n"\
                  "exit will exit this shell. \n"\
                  "help will show you a list of commands and how they work. \n"\
                  "echo will echo back whatever words follow the command itself. \n"\
                  "date will print the current date and time (GMT). \n"\
                  "clockdate tests the date function. \n"\
                  "malloc allocates memory. \n"\
                  "free frees memory. \n"\
                  "memset sets an allocated memory region to a value. \n"\
                  "memchk checks that an allocated memory region is set to a value. \n"\
                  "memorymap prints a map of all allocated memory. \n"\
                  "fopen opens a file or device. \n"\
                  "fclose closes a file or device. \n"\
                  "fgetc and fgets retrieve characters or strings from a file or device. \n"\
                  "fputc and fputs send characters or strings to a file or device. \n"\
                  "fseek sets the file cursor to a particular position in a file. \n"

#define SECYEAR 31536000
#define SECDAY 86400
#define SECHOUR 3600
#define SECMIN 60
#define TIMESTAMP "%02d:%02d:%02d.%06.ld"
#define NUMCOMMANDS (int)(sizeof(commands)/sizeof(commands[0]))
#define NUMESCAPES (int)(sizeof(escapechars)/sizeof(escapechars[0]))
#define BACKSLASH 92
#ifndef CONSOLEIO
#define CONSOLEIO 0
#endif
#ifndef UARTIO
#define UARTIO 1
#endif

extern int g_noFS;

struct date_time {
  char* month;
  int day;
  int year;
  int hour;
  int minute;
  int second;
  char* clock;
};

struct escape_chars {
    char c;
    int ascii;
};

extern struct escape_chars escapechars[];

struct months {
  char *month;
  int order;
  int offset;
};

extern struct months months[];

int cmd_date(int argc, char *argv[]);
int cmd_echo(int argc, char *argv[]);
int cmd_exit(int argc, char *argv[]);
int cmd_help(int argc, char *argv[]);
int cmd_clockdate(int argc, char *argv[]);
int cmd_malloc(int argc, char *argv[]);
int cmd_free(int argc, char *argv[]);
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

struct commandEntry {
  char *name;
  int (*functionp)(int argc, char *argv[]);
};

int shell(void);

int check_digit(char c);

int check_hex (char c);

int check_digit_all(char* str);

int check_hex_all(char* str);

int string_cmp(const char *first, const char *second);

int isleapyear(int inyear);

size_t hex_dec_oct(char* str);

void check_overflow(unsigned long my_num);

#endif