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
#include "svc.h"
#include "led.h"
#include "pushbutton.h"
#include "util.h"
#include "switchcmd.h"
#include "dateTime.h"

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

int g_noFS = TRUE;
file_descriptor io_dev;

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
                {"seek", cmd_seek},
                {"ls", cmd_ls},
                {"touch2led", cmd_touch2led},
                {"pot2ser", cmd_pot2ser},
                {"therm2ser", cmd_therm2ser},
                {"pb2led", cmd_pb2led},
                {"catfile", cmd_catfile},
                {"cat2file", cmd_cat2file},
                {"settime", cmd_settime},
                {"gettime", cmd_gettime}
};

/*Takes as arguments a user command and the length of that command.
It then compares the next character to a list of escape characters and processes it accordingly.*/
char escape_char(char* user_cmd, int* str_len){
  char c;
  SVC_fgetc(io_dev, &c);
  for (int i = 0; i < NUMESCAPES; i++){
    if (c == escapechars[i].c){
      char* user_cmd_ptr = user_cmd + *str_len;
      char escape = escapechars[i].ascii;
      sprintf(user_cmd_ptr, &escape);
      *str_len = *str_len + 1;
      SVC_fgetc(io_dev, &c);
      return c;
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
  SVC_fgetc(io_dev, &c);
  SVC_fputc(io_dev, c);
  while (c != '"') { //TODO: fix this error catching -- maybe memcpy to new string?
    if (c == '\r'){
      SVC_fgetc(io_dev, &c);
      SVC_fputc(io_dev, c);
      if (c == '\n'){
        return E_NOINPUT;
      }
    }
    user_cmd_ptr = user_cmd + *str_len;
    sprintf(user_cmd_ptr, &c);
    *str_len = *str_len + 1;
    SVC_fgetc(io_dev, &c);
    SVC_fputc(io_dev, c);
  }
  *this_len = right_pos - left_pos;
  left_pos = right_pos;
  *argc = *argc + 1;
  user_cmd_ptr = user_cmd + *str_len;
  sprintf(user_cmd_ptr, &c);
  *str_len = *str_len + 1;
  SVC_fgetc(io_dev, &c);
  return c;
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
      SVC_fgetc(io_dev, &c);
      SVC_fputc(io_dev, c);
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
      SVC_fgetc(io_dev, &c);
      if (c == '\n'){
        break;
      }
      else{
    	continue;
      }
    }
    if (c == '\033'){       //attempt to handle special shell escape char
        SVC_fgetc(io_dev, &c);
    }
    if (c == BACKSLASH){     //if it finds a backslash, assumes the use of escape character
      c = escape_char(user_cmd, &str_len);
    }
    //otherwise, falls through and checks for a quotation mark
    if (c == '"'){
      int* this_len = &arg_len[argc + 1];
      c = quote_string(user_cmd, &str_len, &argc, left_pos, this_len);
      SVC_fputc(io_dev, c);
      if (c == E_NOINPUT){
        return E_NOINPUT;
      }
    }
    if (c == ' ' || c == '\t'){
      int right_pos = str_len;
      buffer[0] = c;
      while( buffer[0] == ' ' || buffer[0] == '\t' ){
        buffer[1] = buffer[0];
        SVC_fgetc(io_dev, &buffer[0]);
        SVC_fputc(io_dev, buffer[0]);
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
      SVC_fgetc(io_dev, &c);
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
	SVC_fputs(io_dev, argv[i], strlen(argv[i]));
	SVC_fputc(io_dev, '\r');
	SVC_fputc(io_dev, '\n');
  }
  SVC_fputs(io_dev, argv[argc - 1], strlen(argv[argc - 1]));
  SVC_fputc(io_dev, '\r');
  SVC_fputc(io_dev, '\n');
  return 0;
}

int cmd_exit(int argc, char *argv[]){
  if (argc != 1){
    return E_NUMARGS;
  }
  for (int i = 0; i <= argc; i ++){
    if (argv[i] != NULL){
      SVC_free(argv[i]);
    }
  }
  SVC_free(argv);
  SVC_free(first);
  first = NULL;
  SVC_fclose(io_dev);
  exit(0);
}

int cmd_help(int argc, char *argv[]){
  if (argc > 1){
    return E_NUMARGS;
  }
  SVC_fputs(io_dev, HELPBLOCK, strlen(HELPBLOCK));
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
  if ((mal_val = SVC_malloc(my_size)) == NULL){
    return E_MALLOC;
  }
  else{
	char output[32];
	sprintf(output, "%p \n", mal_val);
	SVC_fputs(io_dev, output, strlen(output));
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
  err_val = SVC_free((void *)ptr_val);
  if (err_val == 0){
	char* msg = "Free successful \n";
	SVC_fputs(io_dev, msg, strlen(msg));
  }
  return err_val;
}

int cmd_memorymap(int argc, char *argv[]){
  if (argc != 1){
    return E_NUMARGS;
  }
  if (first == NULL){ //Initialize
      void* ptr = SVC_malloc(8);
      SVC_free(ptr);
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
  unsigned int my_bound = bounds((void *)ptr_val); //TODO: fix bounds so it can handle the middle of a region
  if (my_bound == 0 || my_bound < reg_len){
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
	char* msg = "memchk successful \n";
	SVC_fputs(io_dev, msg, strlen(msg));
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
	file_descriptor myfile = 0;
	//svcInit_SetSVCPriority(7);
	err = SVC_fopen(&myfile, filename, m);
	if (err != 0){
		return err;
	}
	char* output = SVC_malloc(64);
	sprintf(output, "fopen success \n FILE* is 0x%x \n", (unsigned int)myfile);
	SVC_fputs(io_dev, output, strlen(output));
	SVC_free(output);
	return 0;
}

/*shell interface for univio fclose (file_descriptor descr)*/
int cmd_fclose(int argc, char *argv[]){
	if (argc != 2){
		return E_NUMARGS;
	}
	file_descriptor descrf;
	if ((descrf = (file_descriptor)hex_dec_oct(argv[1])) == 0){
		return E_NOINPUT;
	}
	//svcInit_SetSVCPriority(7);
	int err = SVC_fclose(descrf);
	if (err == 0){
		char* msg = "File close successful \n";
		SVC_fputs(io_dev, msg, strlen(msg));
	}
	return err;
}

/*shell interface for univio create (char* filename)*/
int cmd_create(int argc, char *argv[]){
	if (argc != 2){
		return E_NUMARGS;
	}
	//svcInit_SetSVCPriority(7);
	return SVC_create(argv[1]);
}

/*shell interface for univio delete (char* filename)*/
int cmd_delete(int argc, char *argv[]){
	if (argc != 2){
		return E_NUMARGS;
	}
	//svcInit_SetSVCPriority(7);
	return SVC_delete(argv[1]);
}

/*shell interface for fgetc (file_descriptor descr, char bufp).
 * Prints output directly to terminal. */
int cmd_fgetc(int argc, char *argv[]){
	if (argc != 2){
		return E_NUMARGS;
	}
	int err;
	char bufp = '\0';
	file_descriptor descr;
	if ((descr = (file_descriptor)hex_dec_oct(argv[1])) == 0){
		return E_NOINPUT;
	}
	//svcInit_SetSVCPriority(7);
	err = SVC_fgetc(descr, &bufp);
	if (err < 0){
		return err;
	}
	else{
		char output[2];
		output[0] = bufp;
		output[1] = '\n';
		SVC_fputs(io_dev, output, strlen(output));
	}
	return err;
}

/*shell interface for fgets*/
int cmd_fgets(int argc, char *argv[]){
	return 0;
}

/*shell interface for fputc (file_descriptor descr, char bufp)*/
int cmd_fputc(int argc, char *argv[]){
	if (argc != 3){
		return E_NUMARGS;
	}
	if (strlen(argv[2]) != 1){
		return E_NOINPUT;
	}
	file_descriptor descr;
	if ((descr = (file_descriptor)hex_dec_oct(argv[1])) == 0){
		return E_NOINPUT;
	}
	char m = argv[2][0];
	//svcInit_SetSVCPriority(7);
	return SVC_fputc(descr, m);
}

/*shell interface for fputs. argv[1] is file_descriptor, argv[2] is the string to be put.*/
int cmd_fputs(int argc, char *argv[]){
	if (argc != 3){
		return E_NUMARGS;
	}
	file_descriptor descr;
	if ((descr = (file_descriptor)hex_dec_oct(argv[1])) == 0){
		return E_NOINPUT;
	}
	if (argv[1][0] != '"' && argv[2][strlen(argv[2])-1] != '"'){ //must use quotation marks
		return E_NOINPUT;
	}
	//svcInit_SetSVCPriority(7);
	return SVC_fputs(descr, argv[2], strlen(argv[2]));
}

/*shell interface for seek (file_descriptor, position)*/
int cmd_seek(int argc, char *argv[]){
	if (argc != 3){
		return E_NUMARGS;
	}
	file_descriptor descr;
	if ((descr = (file_descriptor)hex_dec_oct(argv[1])) == 0){
		return E_NOINPUT;
	}
	unsigned long pos = hex_dec_oct(argv[2]);
	return myseek(descr, pos);
}

int cmd_ls(int argc, char *argv[]){
	if (argc != 2){
		return E_NUMARGS;
	}
	if (argv[1][0] != '0' && argv[1][0] != '1'){
		return E_NOINPUT;
	}
	else if (argv[1][0] == '0'){
		return SVC_dir_ls(0);
	}
	return SVC_dir_ls(1);
}

/*touch2led: Continuously copy from each touch sensor to the
        corresponding LED.  End when all four touch sensors are
        "depressed."*/
int cmd_touch2led(int argc, char* argv[]){
	int err;
	if (argc != 1){
		return E_NUMARGS;
	}
	file_descriptor myTS1 = 0;
	//svcInit_SetSVCPriority(7); //TODO: sufficient to open one of each?
	err = SVC_fopen(&myTS1, "dev_TSI1", 'r');
	if (err != 0){
		return err;
	}
	file_descriptor myE1 = 0;
	err = SVC_fopen(&myE1, "dev_E1", 'r');
	if (err != 0){
		return err;
	}
	const unsigned long int delayCount = 0x7ffff;
	while(!(electrode_in(0) && electrode_in(1) && electrode_in(2) && electrode_in(3))) {
		delay(delayCount);
		if(electrode_in(0)) {
			ledOrangeOn();
		} else {
			ledOrangeOff();
		}
		if(electrode_in(1)) {
			ledYellowOn();
		} else {
			ledYellowOff();
		}
		if(electrode_in(2)) {
			ledGreenOn();
		} else {
			ledGreenOff();
		}
		if(electrode_in(3)) {
			ledBlueOn();
		} else {
			ledBlueOff();
		}
	}
	ledOrangeOff();
	ledBlueOff();
	ledGreenOff();
	ledYellowOff();
	err = SVC_fclose(myTS1);
	if (err != 0){
		return err;
	}
	err = SVC_fclose(myE1);
	if (err != 0){
		return err;
	}
	return 0;
}
/*
pot2ser: Continuously output the value of the analog
   potentiomemter to the serial device as a decimal or
   hexadecimal number followed by a newline.  End when SW1 is
   depressed.*/

int cmd_pot2ser(int argc, char* argv[]){
	if (argc != 1){
		return E_NUMARGS;
	}
	//svcInit_SetSVCPriority(7);
	int err;
	file_descriptor sw1;
	err = SVC_fopen(&sw1, "dev_sw1", 'r');
	if (err != 0){
		return err;
	}
	file_descriptor pot;
	err = SVC_fopen(&pot, "dev_pot", 'r');
	if (err != 0){
		return err;
	}
	uint32_t* i = SVC_malloc(sizeof(uint32_t)); //range of potentiometer is uint32_t
	char* myOutput = SVC_malloc(16); //string output
	const unsigned long int delayCount = 0x7ffff;
	while (!sw1In()){
		delay(delayCount);
		err = SVC_fgetc(pot, (char *)i);
		if (err != 0){
			return err;
		}
		longInt2hex(*i, myOutput);
		SVC_fputs(io_dev, myOutput, strlen(myOutput));
		SVC_fputc(io_dev, '\r');
		SVC_fputc(io_dev, '\n');
	}
	SVC_free(i);
	SVC_free(myOutput);
	SVC_fclose(pot);
	return SVC_fclose(sw1);
}

/*
therm2ser: Continuously output the value of the thermistor to
   the serial device as a decimal or hexadecimal number followed
   by a newline.  End when SW1 is depressed.
*/
int cmd_therm2ser(int argc, char* argv[]){
	if (argc != 1){
		return E_NUMARGS;
	}
	//svcInit_SetSVCPriority(7);
	int err;
	file_descriptor sw1;
	err = SVC_fopen(&sw1, "dev_sw1", 'r');
	if (err != 0){
		return err;
	}
	file_descriptor thm;
	err = SVC_fopen(&thm, "dev_temp", 'r');
	if (err != 0){
		return err;
	}
	uint32_t* i = SVC_malloc(sizeof(uint32_t)); //range of potentiometer is uint32_t
	char* myOutput = SVC_malloc(16); //string output
	const unsigned long int delayCount = 0x7ffff;
	while (!sw1In()){
		delay(delayCount);
		err = SVC_fgetc(thm, (char *)i);
		if (err != 0){
			return err;
		}
		longInt2hex(*i, myOutput);
		SVC_fputs(io_dev, myOutput, strlen(myOutput));
		SVC_fputc(io_dev, '\r');
		SVC_fputc(io_dev, '\n');
	}
	SVC_free(i);
	SVC_free(myOutput);
	SVC_fclose(thm);
	return SVC_fclose(sw1);
}

/* 6. pb2led: Continuously copy from SW1 to orange LED and SW2 to
        yellow LED.  End when both SW1 and SW2 are depressed.*/

int cmd_pb2led(int argc, char* argv[]){
	if (argc != 1){
		return E_NUMARGS;
	}
	//svcInit_SetSVCPriority(7);
	int err;
	file_descriptor sw1;
	err = SVC_fopen(&sw1, "dev_sw1", 'r');
	if (err != 0){
		return err;
	}
	file_descriptor sw2;
	err = SVC_fopen(&sw2, "dev_sw2", 'r');
	if (err != 0){
		return err;
	}
	file_descriptor E1;
	err = SVC_fopen(&E1, "dev_E1", 'r'); //red
	if (err != 0){
		return err;
	}
	file_descriptor E4;
	err = SVC_fopen(&E4, "dev_E4", 'r'); //yellow
	if (err != 0){
		return err;
	}
	const unsigned long int delayCount = 0x7ffff;
	while (!(sw1In() && sw2In())){
		delay(delayCount);
		int switchState = switchScan();
		if (switchState == noChange){
			continue;
		}
		else if (switchState == switch1Down){
			ledOrangeOn();
		}
		else if (switchState == switch2Down){
			ledYellowOn();
		}
		else if (switchState == switch1Up){
			ledOrangeOff();
		}
		else if (switchState == switch2Up){
			ledYellowOff();
		}
	}
	ledYellowOff();
	ledOrangeOff();
	err = SVC_fclose(E1);
	if (err != 0){
		return err;
	}
	err = SVC_fclose(E4);
	if (err != 0){
		return err;
	}
	err = SVC_fclose(sw1);
	if (err != 0){
		return err;
	}
	return SVC_fclose(sw2);
}
/*Display the contents of the specified <file> in
        the root directory by sending to STDOUT.
 * */
int cmd_catfile(int argc, char* argv[]){
	if (argc != 2){
		return E_NUMARGS;
	}
	int err = 0;
	char* filename = argv[1];
	file_descriptor descr;
	err = SVC_fopen(&descr, filename, 'r');
	if (err != 0){
		return err;
	}
	struct stream* userptr = (struct stream *) descr;
	char contents[userptr->fileSize];
	for (int i = 0; i < userptr->fileSize; ++i){
	    err = SVC_fgetc(descr, &contents[i]);
		if (err != 0){
			return err;
		}
		err = SVC_fputc(io_dev, contents[i]);
		if (err != 0){
			return err;
		}
	}
	SVC_fputc(io_dev, '\r');
	SVC_fputc(io_dev, '\n');
	return SVC_fclose(descr);
}

/*Continuously copy characters from serial
  input to the specified <file> in the root directory.  End on a
  ^D (control-D) input character.
 * */
int cmd_cat2file(int argc, char* argv[]){
	const unsigned long int delayCount = 0x7ffff;
	if (argc != 2){
		return E_NUMARGS;
	}
	int err = 0;
	char* filename = argv[1];
	file_descriptor descr;
	err = SVC_fopen(&descr, filename, 'w');
	if (err != 0){
		return err;
	}
	char c;
	while(TRUE){
    	// while(!SVC_ischar(io_dev)) {
    	// 	delay(delayCount);
    	// }
	    SVC_fgetc(io_dev, &c);
	    if (c == EOT){
	    	break;
	    }
		SVC_fputc(descr, c);
	}
	return SVC_fclose(descr);
}

/*This sets the system time to an integer (assumed in ms since the MS-DOS Epoch, 00:00 Jan 1, 1980) provided at the command line.*/
int cmd_settime(int argc, char *argv[]){
     if (argc != 2){
		return E_NUMARGS;
	}
     long long thisTime;
     if ((thisTime = (long long)hex_dec_oct(argv[1])) == 0)
     {
		return E_NOINPUT;
	}
     return SVC_settime(thisTime);
}

/*Prints current system time to STDOUT (in ms since the MS-DOS Epoch, 00:00 Jan 1, 1980*/
int cmd_gettime(int argc, char *argv[]){
     if (argc != 1){
		return E_NUMARGS;
	}
     int error;
     check argc argv
     sprintf("%ll", curTime); //check reqd formatting
     return 0;
}

//command line shell accepts user input and executes basic commands
int shell(void){
	const unsigned long int delayCount = 0x7ffff;
	if (UARTIO){
		SVC_fopen(&io_dev, "dev_UART2", 'w'); //open stdin/stdout device
		SVC_fputs(io_dev, "Test\r\n", 6);
		char output[64] = {'\0'};
		sprintf(output, "Your STDIN/STDOUT file is %x \r\n", io_dev);
		SVC_fputs(io_dev, output, strlen(output));
	}
    while(TRUE){
    	char dollar[4] = {'\0'};
    	sprintf(dollar, "$ ", io_dev);
    	SVC_fputs(io_dev, dollar, strlen(dollar));
        int arg_len[MAXARGS+2] = {0};
        char user_cmd[MAXLEN] = {'\0'};       //get argc, create string itself
    	// while(!SVC_ischar(io_dev)) {
    	// 	delay(delayCount);
    	// }
        if ((error_checker(get_string(&user_cmd, arg_len))) != 0){
          return -99;
        }
    	SVC_fputs(io_dev, "\n", 1);
        int argc = arg_len[MAXARGS+1];
        char** argv = (char **)SVC_malloc((argc + 1) * sizeof(char *));
        if (argv == NULL) {
          error_checker(E_MALLOC);
          return E_MALLOC;
        }
        argv[argc] = NULL;
        int user_cmd_offset = 0;
        //parse string into argv
        for (int i = 0; i < argc; i++){
          argv[i]=(char*)SVC_malloc(sizeof(char)*(arg_len[i]+1));
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
          if (strncmp(argv[0], commands[i].name, strlen(commands[i].name)) != 0){
        	  ;
          }
          else{
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
            SVC_free(argv[i]);
          }
        }
        SVC_free(argv);
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
