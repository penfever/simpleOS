#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <sys/time.h>
#include <string.h>
#include "simpleshell.h"
#include "mymalloc/helpers/myerror.h"
#include "mymalloc/mymalloc.h"

//TODO: test weird memset values, weird pointers, hex conversions
//TODO: implement notes on pset 1

extern struct escape_chars escapechars[] = {
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

extern struct months months[] = {{"January", 0, 31}, 
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

struct commandEntry commands[] = {{"date", cmd_date},
                {"echo", cmd_echo},
                {"exit", cmd_exit},
                {"help", cmd_help},
                {"clockdate", cmd_clockdate},
                {"malloc", cmd_malloc},
                {"free", cmd_free},
                {"memorymap", cmd_memorymap},
                {"memset", cmd_memset},
                {"memchk", cmd_memchk}};

char escape_char(char* user_cmd, int* str_len){
  //Checks a string from stdin for escape characters and processes it accordingly
  char c = fgetc(stdin);
  for (int i = 0; i < NUMESCAPES; i++){
    if (c == escapechars[i].c){
      char* user_cmd_ptr = user_cmd + *str_len;
      char escape = escapechars[i].ascii;
      sprintf(user_cmd_ptr, &escape);
      *str_len = *str_len + 1;
      return fgetc(stdin);
    }
  }
  return fgetc(stdin);
}

char quote_string(char* user_cmd, int* str_len, int* argc, int left_pos, int* this_len){
  //Checks a string from stdin for quotation marks and processes it accordingly
  int right_pos = *str_len;
  char* user_cmd_ptr = user_cmd + *str_len;
  char c = '"';
  sprintf(user_cmd_ptr, &c);
  *str_len = *str_len + 1;
  c = fgetc(stdin);
  while (c != '"') {
    //check if too long
    if (c == '\r'){
      c = fgetc(stdin);
    }
    if (c == '\n'){
      return E_NOINPUT;
      //TODO: something sensible
    }
    user_cmd_ptr = user_cmd + *str_len;
    sprintf(user_cmd_ptr, &c);
    *str_len = *str_len + 1;
    c = fgetc(stdin);
  }
  *this_len = right_pos - left_pos;
  left_pos = right_pos;
  *argc = *argc + 1;
  user_cmd_ptr = user_cmd + *str_len;
  sprintf(user_cmd_ptr, &c);
  *str_len = *str_len + 1;
  return fgetc(stdin);
}

int get_string(char* user_cmd, int arg_len[]){
    //Takes raw input from stdin and creates a string, user_cmd. Also calculates argc and the length of the string
    char c;
    char* user_cmd_ptr = NULL;
    int left_pos = 0;
    int str_len = 0;
    int argc = 0;
    char buffer[2];
    buffer[0] = buffer[1] = NULLCHAR;
    while (str_len < MAXLEN - 1){
      if (buffer[1] != NULLCHAR){
        buffer[1] = NULLCHAR;
      }
      else{
        c = fgetc(stdin);
      }
      if (c == '\n'){
        break;
      }
      if (c == '\033'){
        //attempt to handle special shell escape char
        c = fgetc(stdin);
      }
      //if it finds a backslash, check escapechar function
      if (c == BACKSLASH){
        c = escape_char(user_cmd, &str_len);
      }
      if (c == '"'){
        int* this_len = &arg_len[argc + 1];
        c = quote_string(user_cmd, &str_len, &argc, left_pos, this_len);
        if (c == E_NOINPUT){
          return E_NOINPUT;
        }
      }
      if (c == ' ' || c == '\t'){
        int right_pos = str_len;
        buffer[0] = c;
        while( buffer[0] == ' ' || buffer[0] == '\t' ){
          buffer[1] = buffer[0];
          buffer[0] = fgetc(stdin);
        }
        c = buffer[0];
        arg_len[argc] = right_pos - left_pos;
        left_pos = right_pos;
        argc++;
        if (argc == MAXARGS){
          return E_NUMARGS;
        }
        continue;
      }
      if (c == '\n'){
        break;
      }
    user_cmd_ptr = user_cmd + str_len;
    sprintf(user_cmd_ptr, &c);
    str_len++;
  }
  if (user_cmd == NULL || *user_cmd == '\n'){
    return E_NOINPUT;
  }
  if (str_len == MAXLEN - 1){
    return E_TOO_LONG;
  }
  else{
    int right_pos = str_len;
    arg_len[argc] = right_pos - left_pos;
    argc ++;
    user_cmd[str_len] = NULLCHAR;
    arg_len[MAXARGS] = str_len;
    arg_len[MAXARGS+1] = argc;
  }
  return 0;
}

int cmd_echo(int argc, char *argv[]){
  for (int i = 1; i < argc - 1; i++){
    fprintf(stdout, "%s ", argv[i]);
  }
  fprintf(stdout, "%s\n", argv[argc - 1]);
  return 0;
}

int cmd_exit(int argc, char *argv[]){
  if (argc != 1){
    return -4;
  }
  for (int i = 0; i <= argc; i ++){
    if (argv[i] != NULL){
      free(argv[i]);
    }
  }
  free(argv);
  free(first);
  first = NULL;
  exit(0);
}

int cmd_help(int argc, char *argv[]){
  if (argc > 1){
    return E_NUMARGS;
  }
  fprintf(stdout, HELPBLOCK);
  return 0;
}

struct date_time get_time(time_t sec_now){
  //converts an integer value of seconds into a populated date_time struct
  struct date_time curr_date;
  int year_count = 0;
  while (sec_now > SECYEAR){
    year_count += 1;
    sec_now -= SECYEAR;
  }
  //check leap years
  for (int i = 1970; i < 1970 + year_count; i++){
    if (isleapyear(i)){
      sec_now -= SECDAY;
      if (sec_now < 0){
        sec_now += SECYEAR;
        year_count -= 1;
      }
    }
  }
  curr_date.year = 1970 + year_count;
  curr_date.day = 1;
  curr_date.hour = 0;
  curr_date.minute = 0;
  curr_date.clock = "";

  while (sec_now > SECDAY){
    sec_now -= SECDAY;
    curr_date.day += 1;
  }
  while (sec_now > SECHOUR){
    sec_now -= SECHOUR;
    curr_date.hour += 1;
  }
  while (sec_now > SECMIN){
    sec_now -= SECMIN;
    curr_date.minute += 1;
  }
  curr_date.second = sec_now;
  for (int i = 0; i < 11; i ++){
    if (curr_date.day > months[i].offset){
      curr_date.day -= months[i].offset;
      if (i == 1 && isleapyear(curr_date.year)){
        curr_date.day -= 1;
        if (curr_date.day <= 0) {
          curr_date.day += months[i].offset;
          i++;
          curr_date.month = months[i].month;
          break;
        }
      }
    }
    else {
      curr_date.month = months[i].month;
      break;
    }
  }
  return curr_date;
}

void print_time(const struct date_time curr_date, const struct timeval my_time){
  /*formats and prints a date and time from get_time
  (4) "date" will output to stdout the current date and time in the format "January
   23, 2014 15:57:07.123456".  "date" will call the POSIX system call "gettimeofday" to determine the time and date.  "gettimeofday"
   returns the number of seconds and microseconds since midnight (zero
   hours) on January 1, 1970 -- this time is referred to as the Unix
   Epoch. 
  */
  fprintf(stdout, "%s %d, %d " TIMESTAMP "\n", curr_date.month, curr_date.day, curr_date.year, curr_date.hour, curr_date.minute, curr_date.second, my_time.tv_usec);
}

void check_overflow(unsigned long my_num){
  if (my_num == 0){
    perror("strtoul");
  }
  return;
}

int cmd_date(int argc, char *argv[]){
  if (argc != 1){
    return E_NUMARGS;
  }
  struct timeval my_time;
  gettimeofday(&my_time, NULL);
  if (my_time.tv_sec <= 0){
    return E_SYSCALL;
  }
  struct date_time curr_date = get_time(my_time.tv_sec);
  print_time(curr_date, my_time);
  return 0;
}

int cmd_clockdate(int argc, char *argv[]){
  if (argc != 2){
    return E_NUMARGS;
  }
  unsigned long test_sec = 0;
  int i = 0;
  test_sec = strtoul(argv[1], NULL, 10);
  check_overflow(test_sec);
  struct timeval my_time = {test_sec, 0};
  struct date_time curr_date = get_time(test_sec);
  print_time(curr_date, my_time);
  return 0;
}

size_t hex_dec_oct(char* str){
  if (!check_digit(str[0])){ //TODO: other error checking?
    return 0;
  }
  if (str[0] == '0'){
    if (str[1] == 'x' || 'X'){
      return strtoul(str, NULL, 16); //return hex
    }
    return strtoul(str, NULL, 8); //return octal
  }
  return strtoul(str, NULL, 10); //return decimal
}

int cmd_malloc(int argc, char *argv[]){
  if (argc != 2){
    return E_NUMARGS;
  }
  size_t mal_size;
  void* mal_val = NULL;
  mal_size = hex_dec_oct(argv[1]);
  if ((mal_val = myMalloc(mal_size)) == NULL){
    return E_MALLOC;
  }
  else{
    fprintf(stdout, "%p \n", mal_val);
    return 0;
  }
}

int cmd_free(int argc, char *argv[]){
  if (argc != 2){
    return E_NUMARGS;
  }
  size_t ptr_val = 0;
  int err_val = 0;
  ptr_val = hex_dec_oct(argv[1]);
  check_overflow(ptr_val);
  err_val = myFreeErrorCode((void *)ptr_val);
  if (err_val == 0){
    fprintf(stdout, "Free successful \n");
  }
  return err_val;
}

int cmd_memorymap(int argc, char *argv[]){
  if (argc != 1){
    return E_NUMARGS;
  }
  memoryMap(first);
  return 0;
}

int cmd_memset(int argc, char *argv[]){
//   The memset command should accept three arguments.  The first is the
// start address of a memory region, the second is the value to which
// each byte in the specified region will be set, and the third is the
// length (in bytes) of the memory region.
  if (argc != 4){
    return E_NUMARGS;
  }
  size_t ptr_val = 0;
  size_t reg_len = 0;
  unsigned int set_val = 0;
  ptr_val = hex_dec_oct(argv[1]);
  check_overflow(ptr_val);
  set_val = hex_dec_oct(argv[2]);
  check_overflow(set_val);
  reg_len = hex_dec_oct(argv[3]);
  check_overflow(reg_len);
  unsigned int my_bound = bounds((void *)ptr_val); //TODO: fix bounds so it can handle the middle of a region
  if (my_bound == 0 || my_bound < reg_len || set_val > 255){ //TODO: error check? Correct value?
    return E_NOINPUT;
  }
  char *write_val = (char *)ptr_val;
  for (int i = 0; i < reg_len; i++){
    write_val[i] = set_val;
  }
  return 0;
}

int cmd_memchk(int argc, char *argv[]){
  if (argc != 4){
    return E_NUMARGS;
  }
  size_t ptr_val = 0;
  size_t reg_len = 0;
  unsigned int set_val = 0;
  ptr_val = hex_dec_oct(argv[1]);
  check_overflow(ptr_val);
  set_val = hex_dec_oct(argv[2]);
  check_overflow(set_val);
  reg_len = hex_dec_oct(argv[3]);
  check_overflow(reg_len);
  unsigned int my_bound = bounds((void *)ptr_val); //TODO: fix bounds so it can handle the middle of a region
  if (my_bound == 0 || my_bound < reg_len || set_val > 255){ //TODO: error check? Correct value?
    return E_NOINPUT;
  }
  char *write_val = (char *)ptr_val;
  for (int i = 0; i < reg_len; i++){
    if (write_val[i] != set_val){
      return E_MEMCHK;
    }
  }
  fprintf(stdout, "memchk successful \n");
  return 0;
}

int shell(void){
    //command line shell accepts user input and executes basic commands
    char cmd_array[MAX];
    char* user_cmd = &cmd_array;
    while(TRUE){
        fprintf(stdout, "$ ");
        if (user_cmd == NULL) {
          error_checker(E_MALLOC);
          return E_MALLOC;
        }
        int arg_len[MAXARGS+2] = {0};
        //get argc, create string itself
        if ((error_checker(get_string(user_cmd, arg_len))) != 0){
          return -99;
        }
        int argc = arg_len[MAXARGS+1];
        char** argv = (char **)malloc((argc + 1) * sizeof(char *));
        if (argv == NULL) {
          error_checker(E_MALLOC);
          return E_MALLOC;
        }
        argv[argc] = NULL;
        int user_cmd_offset = 0;
        //parse string into argv
        for (int i = 0; i < argc; i++){
          argv[i]=(char*)malloc(sizeof(char)*(arg_len[i]+1));
          if (argv[i] == NULL) {
            error_checker(E_MALLOC);
            return E_MALLOC;
          }
          for (int j = 0; j < arg_len[i]; j++){
            argv[i][j] = user_cmd[user_cmd_offset + j];
          }
          argv[i][arg_len[i]] = NULLCHAR;
          user_cmd_offset += arg_len[i];
        }
        //check if command exists in struct
        for (int i = 0; i < NUMCOMMANDS; i++){
          if (argv[0] == NULL){
            break;
          }
          if (strncmp(argv[0], commands[i].name, strlen(commands[i].name)) == 0){
            //execute command
            int return_value = commands[i].functionp(argc, argv);
            error_checker(return_value);
            break;
          }
          if (i == NUMCOMMANDS - 1){
            error_checker(E_CMD_NOT_FOUND);
          }
        }
        //free memory and repeat
        for (int i = 0; i <= argc; i ++){
          if (argv[i] != NULL){
            free(argv[i]);
          }
        }
        free(argv);
    }
    error_t err_code = E_INF;
    error_checker(err_code);
    return err_code;
}

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

int main(void){
    shell();
}