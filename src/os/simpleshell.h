#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifndef _SIMPLE_SHELL_H
#define _SIMPLE_SHELL_H
#ifndef TRUE
#define TRUE 1
#endif 
#ifndef FALSE
#define FALSE 0
#endif 
#define MAXLEN 256 //accepts chars 0->255, plus newline 256
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
                  "memorymap prints a map of all allocated memory. \n"
#define SECYEAR 31536000
#define SECDAY 86400
#define SECHOUR 3600
#define SECMIN 60
#define TIMESTAMP "%02d:%02d:%02d.%06.ld"
#define NUMCOMMANDS (int)(sizeof(commands)/sizeof(commands[0]))
#define NUMESCAPES (int)(sizeof(escapechars)/sizeof(escapechars[0]))
#define BACKSLASH 92

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
} escapechars[] = {
    {'0', 0},
    {'a', 7},
    {'b', 34},
    {'e', 27},
    {'f', 12},
    {'n', 10},
    {'r', 13},
    {'t', 9},
    {'v', 11},
    {'"', 34}
};

struct months {
  char *month;
  int order;
  int offset;
} months[] = {{"January", 0, 31}, 
              {"February", 1, 28}, 
              {"March", 2, 31},  
              {"April", 3, 30},  
              {"May", 4, 31},  
              {"June", 5, 31},  
              {"July", 6, 31}, 
              {"August", 7, 31}, 
              {"September", 8, 30}, 
              {"October", 9, 31}, 
              {"November", 10, 30}, 
              {"December", 11, 31}, 
              {"February", 12, 29}};

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

struct commandEntry {
  char *name;
  int (*functionp)(int argc, char *argv[]);
} commands[] = {{"date", cmd_date},
                {"echo", cmd_echo},
                {"exit", cmd_exit},
                {"help", cmd_help},
                {"clockdate", cmd_clockdate},
                {"malloc", cmd_malloc},
                {"free", cmd_free},
                {"memorymap", cmd_memorymap},
                {"memset", cmd_memset},
                {"memchk", cmd_memchk}};

int check_digit(char c);

int string_cmp(const char *first, const char *second);

int isleapyear(int inyear);

size_t hex_dec_oct(char* str);

void print_time(const struct date_time curr_date, const struct timeval my_time);

struct date_time get_time(time_t sec_now);

#endif