#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifndef TRUE
#define TRUE 1
#endif 
#ifndef FALSE
#define FALSE 0
#endif 
#ifndef MAXLEN
#define MAXLEN 256 //accepts chars 0->255, plus newline 256
#endif
#ifndef MAXARGS
#define MAXARGS 32 //accepts args 0->32
#endif
#ifndef NULLCHAR
#define NULLCHAR '\0'
#endif 
#ifndef HELPBLOCK
#define HELPBLOCK "Welcome to the simple shell!\n"\
                  "Here are a few commands you can try: \n"\
                  "exit will exit this shell. \n"\
                  "help will show you a list of commands and how they work. \n"\
                  "echo will echo back whatever words follow the command itself. \n"\
                  "date will print the current date and time (GMT). \n"\
                  "clockdate tests the date function. \n"
#endif
#ifndef SECYEAR
#define SECYEAR 31536000
#endif
#ifndef SECDAY
#define SECDAY 86400
#endif
#ifndef SECHOUR
#define SECHOUR 3600
#endif
#ifndef SECMIN
#define SECMIN 60
#endif
#ifndef TIMESTAMP
#define TIMESTAMP "%02d:%02d:%02d.%06.ld"
#endif
#ifndef NUMCOMMANDS
#define NUMCOMMANDS (int)(sizeof(commands)/sizeof(commands[0]))
#endif
#ifndef NUMESCAPES
#define NUMESCAPES (int)(sizeof(escapechars)/sizeof(escapechars[0]))
#endif
#ifndef BACKSLASH
#define BACKSLASH 92
#endif

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

struct date_time {
  char* month;
  int day;
  int year;
  int hour;
  int minute;
  int second;
  char* clock;
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

int check_digit (char c) {
    //basic implementation of isdigit
    if ((c >= '0') && (c <= '9')) return 1;
    return 0;
}
                
int string_cmp(const char *first, const char *second)
//basic implementation of strcmp
{
    while(*first)
    {
        // if characters differ or end of second string is reached, break
        if (*first != *second){
          break;
        }
        // move to next pair of characters
        first++;
        second++;
    }
 
    // return the ASCII difference after converting char* to unsigned char*
    return *(const unsigned char*)first - *(const unsigned char*)second;
}

int isleapyear(int inyear){
    //checks if a given integer, assumed to be a year, is a leap year
    if(inyear % 400 == 0){
        return TRUE;
    }
    else if(inyear % 4 == 0 && inyear % 100 != 0){
        return TRUE;
    }
    else{
        return FALSE;
    }
}