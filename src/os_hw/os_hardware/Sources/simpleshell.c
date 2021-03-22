#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "simpleshell.h"
#include "myerror.h"
#include "mymalloc.h"
#include "devices.h"
#include "univio.h"
#include "SDHC_FAT32_Files.h"
#include "delay.h"
#include "uart.h"
#include "uartNL.h"

struct escape_chars escapechars[] = {
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

struct months months[] = {{"January", 0, 31}, 
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
                {"memchk", cmd_memchk},
                {"fopen", cmd_fopen},
                {"fclose", cmd_fclose},
                {"fgetc", cmd_fgetc},
                {"fgets", cmd_fgets},
                {"fputc", cmd_fputc},
                {"fputs", cmd_fputs},
                {"create", cmd_create},
                {"delete", cmd_delete},
                {"seek", cmd_seek}
};

/*Takes as arguments a user command and the length of that command.
It then compares the next character to a list of escape characters and processes it accordingly.*/
char escape_char(char* user_cmd, int* str_len){
  char c = uartGetchar(UART2_BASE_PTR);
  for (int i = 0; i < NUMESCAPES; i++){
    if (c == escapechars[i].c){
      char* user_cmd_ptr = user_cmd + *str_len;
      char escape = escapechars[i].ascii;
      sprintf(user_cmd_ptr, &escape);
      *str_len = *str_len + 1;
      return uartGetchar(UART2_BASE_PTR);
    }
  }
  return c;
}

/*Takes as arguments a user command, the length of a string, the number of arguments, 
the current scan position and a pointer to an updateable scan position.
It then scans the string until it encounters a close_quote and updates the scan position
as necessary. If it does not encounter a close quote, returns an error.*/
char quote_string(char* user_cmd, int* str_len, int* argc, int left_pos, int* this_len){
  int right_pos = *str_len;
  char* user_cmd_ptr = user_cmd + *str_len;
  char c = '"';
  sprintf(user_cmd_ptr, &c);
  *str_len = *str_len + 1;
  c = uartGetchar(UART2_BASE_PTR);
  while (c != '"') { //TODO: fix this error catching -- maybe memcpy to new string?
    if (c == '\r'){
      c = uartGetchar(UART2_BASE_PTR);
      if (c == '\n'){
        return E_NOINPUT;
      }
    }
    user_cmd_ptr = user_cmd + *str_len;
    sprintf(user_cmd_ptr, &c);
    *str_len = *str_len + 1;
    c = uartGetchar(UART2_BASE_PTR);
  }
  *this_len = right_pos - left_pos;
  left_pos = right_pos;
  *argc = *argc + 1;
  user_cmd_ptr = user_cmd + *str_len;
  sprintf(user_cmd_ptr, &c);
  *str_len = *str_len + 1;
  return uartGetchar(UART2_BASE_PTR);
}

/*Takes user input from stdin and sends it to a char* array representing the user's command, also 
keeping accurate track of the length of the user string. Once it encounters a newline, get_string
parses user arguments to create argc and argv. */
int get_string(char* user_cmd, int arg_len[]){
  char c;
  char* user_cmd_ptr = NULL;
  uint32_t left_pos = 0;
  uint32_t str_len = 0;
  uint32_t argc = 0;
  char buffer[2];
  buffer[0] = buffer[1] = NULLCHAR;
  while (str_len < MAXLEN - 1){
    if (buffer[1] != NULLCHAR){
      buffer[1] = NULLCHAR;
    }
    else{
      c = uartGetchar(UART2_BASE_PTR);
      uartPutchar(UART2_BASE_PTR, c);
    }
//    if (c == 0x08 || c == 0x7f){ //TODO: backspace/delete handling
//      if (str_len == 0){
//          c = uartGetchar(UART2_BASE_PTR);
//      }
//      else{
//    	  str_len --;
//      }
//    }
    if (c == '\r'){
      c = uartGetchar(UART2_BASE_PTR);
      if (c == '\n'){
        break;
      }
      else{
    	continue;
      }
    }
    if (c == '\033'){       //attempt to handle special shell escape char
      c = uartGetchar(UART2_BASE_PTR);
    }
    if (c == BACKSLASH){     //if it finds a backslash, assumes the use of escape character
      c = escape_char(user_cmd, &str_len);
    }
    /*otherwise, falls through and checks for a quotation mark
    if (c == '"'){
      int* this_len = &arg_len[argc + 1];
      c = quote_string(user_cmd, &str_len, &argc, left_pos, this_len);
      if (c == E_NOINPUT){
        return E_NOINPUT;
      }
    }*/
    if (c == ' ' || c == '\t'){
      int right_pos = str_len;
      buffer[0] = c;
      while( buffer[0] == ' ' || buffer[0] == '\t' ){
        buffer[1] = buffer[0];
        buffer[0] = uartGetchar(UART2_BASE_PTR);
        uartPutchar(UART2_BASE_PTR, buffer[0]);
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
    if (c == '\r'){
      c = uartGetchar(UART2_BASE_PTR);
      if (c == '\n'){
        break;
      }
      else{
    	  continue;
      }
    }
    user_cmd_ptr = user_cmd + str_len;
    sprintf(user_cmd_ptr, &c);
    str_len++;
  }
  if (user_cmd == NULL){
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
  if (argc == 1){
    return 0;
  }
  for (int i = 1; i < argc - 1; i++){
	uartPutsNL(UART2_BASE_PTR, argv[i]);
	uartPutchar(UART2_BASE_PTR, '\n');
  }
  uartPutsNL(UART2_BASE_PTR, argv[argc - 1]);
  uartPutchar(UART2_BASE_PTR, '\n');
  return 0;
}

int cmd_exit(int argc, char *argv[]){
  if (argc != 1){
    return -4;
  }
  for (int i = 0; i <= argc; i ++){
    if (argv[i] != NULL){
      myFree(argv[i]);
    }
  }
  myFree(argv);
  free(first);
  first = NULL;
  exit(0);
}

int cmd_help(int argc, char *argv[]){
  if (argc > 1){
    return E_NUMARGS;
  }
  uartPutsNL(UART2_BASE_PTR, HELPBLOCK);
  return 0;
}

/*formats and prints a date and time from get_time -- "date" will output to stdout 
the current date and time in the format "January 23, 2014 15:57:07.123456".  
"date" will call the POSIX system call "gettimeofday" to determine the time and date.  
"gettimeofday" returns the number of seconds and microseconds since midnight (zero
hours) on January 1, 1970 -- this time is referred to as the Unix Epoch. */

int cmd_date(int argc, char *argv[]){
  if (argc != 1){
    return E_NUMARGS;
  }
  return 0;
}

int cmd_clockdate(int argc, char *argv[]){
  if (argc != 2){
    return E_NUMARGS;
  }
  return 0;
}

int cmd_malloc(int argc, char *argv[]){
  if (argc != 2){
    return E_NUMARGS;
  }
  void* mal_val = NULL;
  long long unsigned int my_size = hex_dec_oct(argv[1]);
  if ((mal_val = myMalloc(my_size)) == NULL){
    return E_MALLOC;
  }
  else{
	char* output = myMalloc(32);
	sprintf(output, "%p \n", mal_val);
	uartPutsNL(UART2_BASE_PTR, output);
	myFree(output);
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
	uartPutsNL(UART2_BASE_PTR, "Free successful \n");
  }
  return err_val;
}

int cmd_memorymap(int argc, char *argv[]){
  if (argc != 1){
    return E_NUMARGS;
  }
  if (first == NULL){ //Initialize
      void* ptr = myMalloc(8);
      myFree(ptr);
  }
  memoryMap();
  return 0;
}

/*   The memset command should accept three arguments.  The first is the
// start address of a memory region, the second is the value to which
// each byte in the specified region will be set, and the third is the
// length (in bytes) of the memory region. */
int cmd_memset(int argc, char *argv[]){
  if (argc != 4){
    return E_NUMARGS;
  }
  size_t ptr_val = 0;
  size_t reg_len = 0;
  char set_val = 0;
  ptr_val = hex_dec_oct(argv[1]);
  check_overflow(ptr_val);
  if (strlen(argv[2]) != 1 || argv[2][0] > 255 || argv[2][0] < 0){
    return E_NOINPUT;
  }
  set_val = (char)argv[2][0];
  reg_len = hex_dec_oct(argv[3]);
  check_overflow(reg_len);
  // unsigned long* my_offset = malloc(sizeof(long*));
  unsigned int my_bound = bounds((void *)ptr_val); //TODO: fix bounds so it can handle the middle of a region
  if (my_bound == 0 || my_bound < reg_len){
    return E_NOINPUT;
  }
  char *write_val = (char *)ptr_val;
  for (int i = 0; i < reg_len; i++){
    write_val[i] = set_val;
  }
  // free(my_offset);
  return 0;
}

int cmd_memchk(int argc, char *argv[]){
  if (argc != 4){
    return E_NUMARGS;
  }
  size_t ptr_val = 0;
  size_t reg_len = 0;
  char set_val = 0;
  ptr_val = hex_dec_oct(argv[1]);
  check_overflow(ptr_val);
  if (strlen(argv[2]) != 1 || argv[2][0] > 255 || argv[2][0] < 0){
    return E_NOINPUT;
  }
  set_val = (char)argv[2][0];
  reg_len = hex_dec_oct(argv[3]);
  check_overflow(reg_len);
  unsigned int my_bound = bounds((void *)ptr_val);
  if (my_bound == 0 || my_bound < reg_len){
    return E_NOINPUT;
  }
  char *read_val = (char *)ptr_val;
  for (int i = 0; i < reg_len; i++){
    if (read_val[i] != set_val){
      return E_MEMCHK;
    }
  }
  uartPutsNL(UART2_BASE_PTR, "memchk successful \n");
  return 0;
}

/*shell interface for univio fopen (file_descriptor descr, char* filename, char mode)
 * Prints pointer directly to terminal. */
int cmd_fopen(int argc, char *argv[]){
	if (argc != 3){
		return E_NUMARGS;
	}
	if (strlen(argv[2]) != 1){
		return E_NOINPUT;
	}
	int err = 0;
	char* filename = argv[1];
	char m = argv[2][0];
	file_descriptor myfile;
	err = myfopen(myfile, filename, m);
	if (err != 0){
		return err;
	}
	char* output = myMalloc(256);
	sprintf(output, "fopen success \n FILE* is %p \n", (void*)myfile);
	free(output);
	return 0;
}

/*shell interface for univio fclose (file_descriptor descr)*/
int cmd_fclose(int argc, char *argv[]){
	if (argc != 2){
		return E_NUMARGS;
	}
	if (check_digit_all(argv[1]) != TRUE){
		return E_NOINPUT;
	}
	unsigned long descr = strtoul(argv[1], NULL, 10);
	return myfclose(descr);
}

/*shell interface for univio create (char* filename)*/
int cmd_create(int argc, char *argv[]){
	if (argc != 2){
		return E_NUMARGS;
	}
	return mycreate(argv[1]);
}

/*shell interface for univio delete (char* filename)*/
int cmd_delete(int argc, char *argv[]){
	if (argc != 2){
		return E_NUMARGS;
	}
	return mydelete(argv[1]);
}

/*shell interface for fgetc (file_descriptor descr, char bufp).
 * Prints output directly to terminal. */
int cmd_fgetc(int argc, char *argv[]){
	if (argc != 2){
		return E_NUMARGS;
	}
	int err;
	char bufp;
	unsigned long descr = strtoul(argv[1], NULL, 10);
	err = myfgetc(descr, bufp);
	if (err < 0){
		return err;
	}
	else{
		uartPutchar(UART2_BASE_PTR, bufp);
	}
	return err;
}

/*shell interface for fgets*/
int cmd_fgets(int argc, char *argv[]){
	;
}

/*shell interface for fputc (file_descriptor descr, char bufp)*/
int cmd_fputc(int argc, char *argv[]){
	if (argc != 3){
		return E_NUMARGS;
	}
	if (check_digit_all(argv[1]) != TRUE){
		return E_NOINPUT;
	}
	if (strlen(argv[2]) != 1){
		return E_NOINPUT;
	}
	unsigned long descr = strtoul(argv[1], NULL, 10);
	char m = argv[2][0];
	return myfputc(descr, m);
}

/*shell interface for fputs*/
int cmd_fputs(int argc, char *argv[]){
	;
}

/*shell interface for seek (file_descriptor, position)*/
int cmd_seek(int argc, char *argv[]){
	if (argc != 3){
		return E_NUMARGS;
	}
	if (check_digit_all(argv[1]) != TRUE){
		return E_NOINPUT;
	}
	if (check_digit_all(argv[2]) != TRUE){
		return E_NOINPUT;
	}
	unsigned long descr = strtoul(argv[1], NULL, 10);
	unsigned long pos = strtoul(argv[2], NULL, 10);
	return myseek(descr, pos);
}

//command line shell accepts user input and executes basic commands
int shell(void){
	const unsigned long int delayCount = 0x7ffff;
	uart_init(115200);
    //setvbuf(stdin, NULL, _IONBF, 0); //fix for consoleIO stdin and stdout
    //setvbuf(stdout, NULL, _IONBF, 0);
    while(TRUE){
    	uartPutsNL(UART2_BASE_PTR, "$ ");
        int arg_len[MAXARGS+2] = {0};
        char user_cmd[MAXLEN] = {'\0'};       //get argc, create string itself
    	while(!uartGetcharPresent(UART2_BASE_PTR)) {
    		delay(delayCount);
    	}
        if ((error_checker(get_string(&user_cmd, arg_len))) != 0){
          return -99;
        }
        uartPutsNL(UART2_BASE_PTR, "\n");
        int argc = arg_len[MAXARGS+1];
        char** argv = (char **)myMalloc((argc + 1) * sizeof(char *));
        if (argv == NULL) {
          error_checker(E_MALLOC);
          return E_MALLOC;
        }
        argv[argc] = NULL;
        int user_cmd_offset = 0;
        //parse string into argv
        for (int i = 0; i < argc; i++){
          argv[i]=(char*)myMalloc(sizeof(char)*(arg_len[i]+1));
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
            myFree(argv[i]);
          }
        }
        myFree(argv);
    }
    error_t err_code = E_INF;
    error_checker(err_code);
    return err_code;
}

//HELPERS

//basic implementation of isdigit
int check_digit (char c) {
    if ((c >= '0') && (c <= '9')) return 1;
    return 0;
}

//basic implementation of ishex
int check_hex (char c) {
    if ((c >= '0') && (c <= '9')) return 1;
    if ((c >= 'a') && (c <= 'f')) return 1;
    if ((c >= 'A') && (c <= 'F')) return 1;
    return 0;
}

//checks if a given integer, assumed to be a year, is a leap year
int isleapyear(int inyear){
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

/*Helper: checks strtoul output for integer overflow error and prints error if one is encountered.*/
void check_overflow(unsigned long my_num){
  if (my_num == 0){
    error_checker(E_NOINPUT);
  }
  return;
}

/*Helper function accepts a string and returns true if every character in the string is a digit.*/
int check_digit_all(char* str){
  for (int i = 0; i < strlen(str); i++){
    if (!check_digit(str[i])){
      return FALSE;
    }
  }
  return TRUE;
}

/*Helper function accepts a string and returns true if every character in the string is a hex digit.*/
int check_hex_all(char* str){
  for (int i = 0; i < strlen(str); i++){
    if (!check_hex(str[i])){
      return FALSE;
    }
  }
  return TRUE;
}

/*Helper function parses a user string str and returns it in hex, octal or decimal form, if 
it is an integer. If it is not an integer or some other error has occurred, returns 0.*/
size_t hex_dec_oct(char* str){
  char* p_str = str + 2;
  int check_str;
  check_str = check_digit_all(str);
  if (!check_digit(str[0])){
    return 0;
  }
  if (str[0] == '0'){
    if (str[1] == 'x' || str[1] == 'X'){
      if (check_hex_all(p_str) == FALSE){
        return 0;
      }
      return strtoul(str, NULL, 16); //return hex
    }
    if (check_str == FALSE){
      return 0;
    }
    return strtoul(str, NULL, 8); //return octal
}
  if (check_str == FALSE){
    return 0;
  }
  return strtoul(str, NULL, 10); //return decimal
}
