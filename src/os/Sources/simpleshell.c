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
#include "pdb.h"
#include "procs.h"

/*Globals*/
int g_noFS = TRUE;
uint8_t g_timerExpired = TRUE;
uint8_t g_randVal;
file_descriptor io_dev; //stdin, stdout. Defaults to UART_2

/*Structs*/
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

struct commandEntry commands[] = {{"date", cmd_date}, //0
                {"echo", cmd_echo},
                {"exit", cmd_exit},
                {"help", cmd_help},
                {"malloc", cmd_malloc},
                {"free", cmd_free}, //5
                {"memorymap", cmd_memorymap},
                {"memset", cmd_memset},
                {"memchk", cmd_memchk},
                {"malfree", cmd_malfree},
                {"fopen", cmd_fopen}, //10
                {"fclose", cmd_fclose},
                {"fgetc", cmd_fgetc},
                {"fgets", cmd_fgets},
                {"fputc", cmd_fputc},
                {"fputs", cmd_fputs}, //15
                {"create", cmd_create},
                {"delete", cmd_delete},
                {"seek", cmd_seek},
                {"ls", cmd_ls},
                {"touch2led", cmd_touch2led}, //20
                {"pot2ser", cmd_pot2ser},
                {"therm2ser", cmd_therm2ser},
                {"pb2led", cmd_pb2led},
                {"catfile", cmd_catfile},
                {"cat2file", cmd_cat2file}, //25
                {"flashled", cmd_flashled},
                {"shell", cmd_shell},
                {"spawn", cmd_spawn},
                {"kill", cmd_kill}
};

/*User Commands*/

/*echo prints to the terminal whatever was typed as an argument by the user following the call to echo itself.*/
int cmd_echo(int argc, char *argv[]){
  if (argc == 1){
    return 0;
  }
  char* output[MAXLEN] = {NULLCHAR};
  for (int i = 1; i < argc - 1; i++){
	sprintf(output, "%s \n", argv[i]);
	SVC_fputs(io_dev, output, strlen(argv[i]));
  }
  sprintf(output, "%s \n", argv[argc - 1]);
  SVC_fputs(io_dev, output, strlen(argv[argc - 1]));
  return 0;
}

/*exit shuts down all processes and devices, frees memory and exits the shell.*/
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
  close_all_devices();
  if (!g_noFS){
    file_structure_umount();
  }
  exit(0);
}

/*help displays a list of commands, along with their behavior in the shell.*/
int cmd_help(int argc, char *argv[]){
  if (argc > 1){
    return E_NUMARGS;
  }
  SVC_fputs(io_dev, HELPBLOCK, strlen(HELPBLOCK));
  return 0;
}

/*date formats and prints a date and time from get_time -- "date" will output to stdout 
the current date and time in the format "January 23, 2014 15:57:07.123456". 
If called with one argument, this command sets the system time 
 * to an integer (assumed in ms since the MS-DOS Epoch, 00:00 Jan 1, 1980) provided at the command line.
 * Note: system time WILL NOT INCREMENT unless time is set using this command.
 * This command will display the current date and time when invoked without any arguments.*/ 
int cmd_date(int argc, char *argv[]){
  if (argc == 1){
    if (curTime == 0){
      return E_NOINPUT;
    }
    struct date_time curr_date = get_time();
    print_time(curr_date);
    return 0;
  }
  if (argc != 2){
  	return E_NUMARGS;
  }
  if (argv[1][4] == '-' && argv[1][7] == argv[1][4] && argv[1][10] == 'T'){ //ISO 8601 handling
    int year, month, day, hour, minute;
    char c;
    int fieldsRead = sscanf(argv[1], "%d-%d-%d%c%d%c%d", &year, &month, &day, &c, &hour, &c, &minute);
    if (fieldsRead != 7){
        return E_NOINPUT;
    }
    else{
      unsigned long long resultVal = ymdhm_to_ms(year, month, day, hour, minute);
      return SVC_settime(&resultVal);
    }
    if (MYFAT_DEBUG){
      printf("fieldsRead is %d, year, month, day end are %d, %d, %d, %d, %d. \n", fieldsRead, year, month, day, hour, minute);
    }
  }
  long long setTime;
  if ((setTime = hex_dec_oct_ll(argv[1])) < 1){
  	return E_NOINPUT;
  }
  return SVC_settime(&setTime);
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
	char* msg = "memchk success \n";
	SVC_fputs(io_dev, msg, strlen(msg));
	return 0;
}

/*stress-testing function for malloc and free to uncover errors.*/
int cmd_malfree(int argc, char *argv[]){
	int err;
	uint8_t randVal = g_randVal;
	char** malVal[100];
	for (int i = 0; i < 100; i++){
		if (randVal > 100){
			randVal = 1;
		}
		if ((malVal[i] = SVC_malloc(randVal)) == NULL){
		  return E_MALLOC;
		}
		randVal ++;
	}
	memoryMap();
	for (int i = 0; i < 100; i++){
		if ((err = SVC_free((void *)malVal[i])) != 0){
			return err;
		}
	}
	char* msg = "malfree success \n";
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
	err = SVC_fopen(&myfile, filename, m);
	if (err != 0){
		return err;
	}
	char* output = SVC_malloc(64);
  memset(output, '\0', 64);
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
	return SVC_create(argv[1]);
}

/*shell interface for univio delete (char* filename)*/
int cmd_delete(int argc, char *argv[]){
	if (argc != 2){
		return E_NUMARGS;
	}
	return SVC_delete(argv[1]);
}

/*shell interface for fgetc (file_descriptor descr, char bufp).
 * Prints output directly to terminal. */
int cmd_fgetc(int argc, char *argv[]){
	if (argc != 2){
		return E_NUMARGS;
	}
	int err;
	char bufp = NULLCHAR;
	file_descriptor descr;
	if ((descr = (file_descriptor)hex_dec_oct(argv[1])) == 0){
		return E_NOINPUT;
	}
	err = SVC_fgetc(descr, &bufp);
	if (err < 0){
		return err;
	}
	else{
		char output[3];
		output[0] = bufp;
		output[1] = '\r';
		output[2] = '\n';
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
	if (argc > 2){
		return E_NUMARGS;
	}
	if (argc == 1){
		return SVC_dir_ls(0);
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
    if (strncmp(argv[0], "spawn", 5) == 0){
      ;
    }
    else{
		  return E_NUMARGS;
    }
	}
	file_descriptor myTS1 = 0;
	err = SVC_fopen(&myTS1, "dev_TSI1", 'r');
	if (err != 0){
		return err;
	}
	  file_descriptor myTS2 = 0;
		err = SVC_fopen(&myTS2, "dev_TSI2", 'r');
		if (err != 0){
			return err;
		}
	  file_descriptor myTS3 = 0;
		err = SVC_fopen(&myTS3, "dev_TSI3", 'r');
		if (err != 0){
			return err;
		}
	  file_descriptor myTS4 = 0;
		err = SVC_fopen(&myTS4, "dev_TSI4", 'r');
		if (err != 0){
			return err;
		}
	file_descriptor E1 = 0;
	err = SVC_fopen(&E1, "dev_E1", 'r');
	if (err != 0){
		return err;
	}
  file_descriptor E2 = 0;
	err = SVC_fopen(&E2, "dev_E2", 'r');
	if (err != 0){
		return err;
	}
  file_descriptor E3 = 0;
	err = SVC_fopen(&E3, "dev_E3", 'r');
	if (err != 0){
		return err;
	}
  file_descriptor E4 = 0;
	err = SVC_fopen(&E4, "dev_E4", 'r');
	if (err != 0){
		return err;
	}
	uint32_t ts1Val = 0;
	uint32_t ts2Val = 0;
	uint32_t ts3Val = 0;
	uint32_t ts4Val = 0;
	char* bufp = "a";
	while(TRUE) {
		SVC_fgetc(myTS1, (char*)&ts1Val);
		SVC_fgetc(myTS2, (char*)&ts2Val);
		SVC_fgetc(myTS3, (char*)&ts3Val);
		SVC_fgetc(myTS4, (char*)&ts4Val);
		uint8_t sumVal = ts1Val && ts2Val && ts3Val && ts4Val;
		if(sumVal){
			break;
		}
		if(ts1Val){
		  SVC_fgetc(E1, bufp); //fgetc turns LED on
		} 
		else {
		  SVC_fputc(E1, 'a'); //fputc turns LED off
		}
		if(ts2Val){
		  SVC_fgetc(E4, bufp); //fgetc turns YELLOW LED on
		} 
		else {
		  SVC_fputc(E4, 'a'); //fputc turns YELLOW LED off
		}
		if(ts3Val) {
		  SVC_fgetc(E3, bufp); //fgetc turns LED on
		} 
		else {
		  SVC_fputc(E3, 'a'); //fputc turns LED off
		}
		if(ts4Val) {
		  SVC_fgetc(E2, bufp); //fgetc turns BLUE LED on
		} 
		else {
		  SVC_fputc(E2, 'a'); //fputc turns BLUE LED off
		}
	}
	  SVC_fputc(E1, 'a'); //fputc turns LED off
	  SVC_fputc(E2, 'a'); //fputc turns LED off
	  SVC_fputc(E3, 'a'); //fputc turns LED off
	  SVC_fputc(E4, 'a'); //fputc turns LED off
	err = SVC_fclose(myTS1);
	if (err != 0){
		return err;
	}
  err = SVC_fclose(myTS2);
	if (err != 0){
		return err;
	}
  err = SVC_fclose(myTS3);
	if (err != 0){
		return err;
	}
  err = SVC_fclose(myTS4);
	if (err != 0){
		return err;
	}
	err = SVC_fclose(E1);
	if (err != 0){
		return err;
	}
  err = SVC_fclose(E2);
	if (err != 0){
		return err;
	}
  err = SVC_fclose(E3);
	if (err != 0){
		return err;
	}
  err = SVC_fclose(E4);
	if (err != 0){
		return err;
	}
	return 0;
}

/*pot2ser: Continuously output the value of the analog
   potentiomemter to the serial device as a decimal or
   hexadecimal number followed by a newline.  End when SW1 is
   depressed.*/
int cmd_pot2ser(int argc, char* argv[]){
	if (argc != 1){
		return E_NUMARGS;
	}
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
	uint32_t* potVal = SVC_malloc(sizeof(uint32_t)); //range of potentiometer is uint32_t
	*potVal = 128;
	char* myOutput = SVC_malloc(16); //string output
  memset(myOutput, '\0', 16);
	const unsigned long int delayCount = 0x7ffff;
	while (SVC_fgetc(sw1, "a") != 1){
		delay(delayCount);
		err = SVC_fgetc(pot, (char *)potVal);
		if (err != 0){
			return err;
		}
		longInt2hex(*potVal, myOutput);
		SVC_fputs(io_dev, myOutput, strlen(myOutput));
		SVC_fputc(io_dev, '\r');
		SVC_fputc(io_dev, '\n');
	}
	SVC_free(potVal);
	SVC_free(myOutput);
	SVC_fclose(pot);
	return SVC_fclose(sw1);
}

/* therm2ser: Continuously output the value of the thermistor to
   the serial device as a decimal or hexadecimal number followed
   by a newline.  End when SW1 is depressed.*/
int cmd_therm2ser(int argc, char* argv[]){
	if (argc != 1){
		return E_NUMARGS;
	}
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
	char c = 'a';
	while (SVC_fgetc(sw1, &c) != 1){
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
    if (strncmp(argv[0], "spawn", 5) == 0){
      ;
    }
    else{
		  return E_NUMARGS;
    }
	}
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
	err = SVC_fopen(&E1, "dev_E1", 'r'); //orange
	if (err != 0){
		return err;
	}
	file_descriptor E4;
	err = SVC_fopen(&E4, "dev_E4", 'r'); //yellow
	if (err != 0){
		return err;
	}
	const unsigned long int delayCount = 0x7ffff;
	char* bufp = " ";
	while (SVC_fgetc(sw2, 'a') != 3){
		delay(delayCount);
		int switchState = switchScan();
		if (switchState == noChange){
			continue;
		}
		else if (switchState == switch1Down){
      SVC_fgetc(E1, bufp); //fgetc turns LED on
		}
		else if (switchState == switch2Down){
      SVC_fgetc(E4, bufp); //fgetc turns LED on
		}
		else if (switchState == switch1Up){
      SVC_fputc(E1, 'a'); //fputc turns LED off
		}
		else if (switchState == switch2Up){
      SVC_fputc(E4, 'a'); //fputc turns LED off
		}
	}
  SVC_fputc(E1, 'a'); //fputc turns LED off
  SVC_fputc(E4, 'a'); //fputc turns LED off
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
	char contents[4]; //TODO: doesn't need to be this size
	for (int i = 0; i < userptr->fileSize; ++i){
	    err = SVC_fgetc(descr, &contents[0]);
		if (err != 0){
			SVC_fclose(descr);
			return err;
		}
		err = SVC_fputc(io_dev, contents[0]);
		if (err != 0){
			SVC_fclose(descr);
			return err;
		}
	}
	SVC_fputc(io_dev, '\n');
	return SVC_fclose(descr);
}

/*Continuously copy characters from serial
  input to the specified <file> in the root directory.  End on a
  ^D (control-D) input character.
 * */
int cmd_cat2file(int argc, char* argv[]){
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
	    SVC_fgetc(io_dev, &c);
	    if (c == EOT){
	    	break;
	    }
		SVC_fputc(descr, c);
	}
	return SVC_fclose(descr);
}

/*flashled takes an argument, argv[1], between 0 and 127 (0 is ~7.8ms, 127 is 1s). 
It toggles the orange LED on and off every argv[1] milliseconds 
until sw1 is pushed.*/
int cmd_flashled(int argc, char* argv[]){
  int err = 0;
  uint16_t delayCount;
	if (argc != 2){
    if (strncmp(argv[0], "spawn", 5) == 0){
      delayCount = hex_dec_oct(argv[2]);
    }
    else{
		  return E_NUMARGS;
    }
	}
  if (argc == 2){
    delayCount = hex_dec_oct(argv[1]); 
  }
  if (delayCount < 0 || delayCount > 127){
    return E_NOINPUT;
  }
  uint8_t toggle = TRUE;
  file_descriptor sw1;
	err = SVC_fopen(&sw1, "dev_sw1", 'r');
	if (err != 0){
		return err;
	}
  file_descriptor myE1 = 0;
	err = SVC_fopen(&myE1, "dev_E1", 'r');
	if (err != 0){
		return err;
	}
  while(!sw1In()){
    if (g_timerExpired) {
      if (toggle){
    	char* bufp = " ";
        SVC_fgetc(myE1, bufp); //fgetc turns LED on
      }
      else{
        SVC_fputc(myE1, 'a'); //fputc turns LED off
      }
      toggle = !toggle; //toggle reverses
      SVC_pdb0oneshottimer(&delayCount); //timer reset
    }
  }
  return 0;
}
/*   The spawn command will take a single argument which is one of
   touch2led, pb2led, or flashled and will spawn off a process to run
   that code.*/
int cmd_spawn(int argc, char* argv[]){
  int err = 0;
  if (argc != 2){
    return E_NUMARGS;
  }
  /*get pid for process to be spawned*/
  pid_t shellPid;
  shellPid = get_next_free_pid();
  /*CASE: touch2led*/
  if (strncmp(argv[1], commands[20].name, strlen(commands[20].name)) != 0){
    ;
  }
  else{
    struct spawnData thisSpawnData = {commands[20].name, NEWPROC_DEF, &shellPid};
    err = SVC_spawn(commands[20].functionp, argc, argv, &thisSpawnData);
    SVC_wait(shellPid);
    return err;
  }
  /*CASE: pb2led*/
  if (strncmp(argv[1], commands[23].name, strlen(commands[23].name)) != 0){
    ;
  }
  else{
    struct spawnData thisSpawnData = {commands[23].name, NEWPROC_DEF, &shellPid};
    err = SVC_spawn(commands[23].functionp, argc, argv, &thisSpawnData);
    SVC_wait(shellPid);
    //wait
    return err;
  }
  /*CASE: flashled*/
  if (strncmp(argv[1], commands[26].name, strlen(commands[26].name)) != 0){
    ;
  }
  else{
    struct spawnData thisSpawnData = {commands[26].name, NEWPROC_DEF, &shellPid};
    err = SVC_spawn(commands[26].functionp, argc, argv, &thisSpawnData);
    SVC_wait(shellPid);
    return err;
  }
  return E_NOINPUT;
}

/*cmd_kill shell command takes a single
 argument which is the PID of the process to be killed.
 and kills that process.*/
int cmd_kill(int argc, char* argv[]){
  if (argc != 2){
    return E_NUMARGS;
  }
  pid_t killPid = (pid_t)hex_dec_oct(argv[1]);
  check_overflow(killPid);
  return SVC_kill(killPid);
}

/*String processing functions*/

/*escape_char takes a user command string containing a backslash. 
It then compares the next character to a list of escape characters.
If a match is found, it writes ' ' to the position of the char following
the backslash, and the correct ASCII escaped character to the position of backslash.*/
void escape_char(char* user_cmd, char* user_cmd_clean, int* cleanLen){
  user_cmd ++;
  for (int i = 0; i < NUMESCAPES; i++){
    if (*user_cmd == escapechars[i].c){
      *user_cmd_clean = escapechars[i].ascii;
      *cleanLen ++;
      return;
    }
  }
  //if no match found, simply copy the characters verbatim into the stream
  user_cmd --;
  *user_cmd_clean = *user_cmd;
  user_cmd_clean ++;
  user_cmd ++;
  *user_cmd_clean = *user_cmd;
  *cleanLen += 2;
  return;
}

/*quote_check checks a user command to ensure every open quote has a matching end quote.
returns 0 on success, error on failure.*/
int quote_check(char* user_cmd, uint16_t cmdLen){
  int quote_count = 0;
  for (int i = 0; i < cmdLen; i++){
    if (user_cmd[i] == '"'){
      quote_count ++;
    }
  }
  if (quote_count % 2 != 0){ //discard strings with an invalid number of quotation marks
    return E_NOINPUT;
  }
  return 0;
}

/*quote_char adds all characters between quotation marks in user_cmd to a single argument in user_cmd_clean,
omitting the quotation marks themselves. It also calculates the length of the string between quotes
and updates quote_len with that value.*/
void quote_char(char* user_cmd, char* user_cmd_clean, int* quote_len){
  while (*user_cmd != 34){
	*user_cmd_clean = *user_cmd;
    user_cmd ++;
    user_cmd_clean ++;
    *quote_len = *quote_len + 1;
  }
  return;
}

/*parse_string parses a user-provided string, determines how many
* arguments are in the string as well as the position of each argument
* in the string. It then passes that information on in order to 
* create an argv array.*/
int parse_string(char* user_cmd, char* user_cmd_clean, int arg_len[], uint16_t cmdLen){
  uint16_t left_pos = 0;
  uint32_t argc = 0; //use 1-indexing on argc
  int cleanLen = 0;
  int right_pos = 0;
  for (; right_pos < cmdLen; right_pos++){
    if (user_cmd[right_pos] == BACKSLASH){ //escape character handling
      escape_char(&user_cmd[right_pos], &user_cmd_clean[cleanLen], &cleanLen);
      continue;
    }
    if (user_cmd[right_pos] == 34 || user_cmd[right_pos] == ' ' || user_cmd[right_pos] == '\t' || user_cmd[right_pos] == '\r' || user_cmd[right_pos] == '\n'){ //special char handling
      if (user_cmd[right_pos] == 34){ //quotes mean a new argument, plus a special handler
    	int temp = 0;
        quote_char(&user_cmd[right_pos+1], &user_cmd_clean[cleanLen], &temp);
        arg_len[argc] = temp;
        cleanLen += arg_len[argc];
        right_pos += arg_len[argc] + 1;
        if (++argc == MAXARGS){
          return E_NUMARGS;
        }
      }
      else if (user_cmd[right_pos] == ' ' || user_cmd[right_pos] == '\t'){ //whitespace means a new argument
        arg_len[argc] = right_pos - left_pos;
        left_pos = right_pos + 1;
        if (++argc == MAXARGS){
          return E_NUMARGS;
        }
      }
      else{ //discard newlines and carriage returns
          break;
      }
      while (user_cmd[right_pos+1] == ' ' || user_cmd[right_pos+1] == '\t'){ //surplus characters consumed
      	right_pos ++;
    	left_pos = right_pos;
      }
      continue;
    }
    //fall-through copies character, increments cleaned string length and continues loop
    user_cmd_clean[cleanLen] = user_cmd[right_pos];
    cleanLen ++;
  }
  if (MYFAT_DEBUG){
	  printf("parsestring: ");
	  for (int i = 0; i < cleanLen; i++){
		  printf("%c", user_cmd_clean[i]);
	  }
	  printf("\n");
  }
  arg_len[argc] = right_pos - left_pos;
  user_cmd_clean[cleanLen] = NULLCHAR;
  arg_len[MAXARGS] = cleanLen; //store length of string
  arg_len[MAXARGS+1] = argc + 1; //store argc with 1-indexing
  return 0;
}

/*This is a dummy shell function to pass into spawn.*/
int cmd_shell(int argc, char* argv[]){
    int error;
    error = shell();
    return 0;
}

/*main shell function*/
int shell(void){
  unsigned long long gmtTime = timestamp_to_ms();
  g_randVal = ((gmtTime / 1000)% 100) + 1; //establishes semi-random value between 1 and 100 for future reference by other functions
  SVC_settime(&gmtTime); //set default time to GMT
  char output[64] = {NULLCHAR};
  sprintf(output, "Your STDIN/STDOUT file is %x \n", (unsigned int)io_dev);
  SVC_fputs(io_dev, output, strlen(output));
  while(TRUE){
    int err;
    char dollar[4] = {NULLCHAR};
    sprintf(dollar, "$ ");
    SVC_fputs(io_dev, dollar, strlen(dollar));
    int arg_len[MAXARGS+2] = {0}; //arglen_maxargs is string length, arglen_maxargs+1 is argc
    char user_cmd[MAXLEN] = {NULLCHAR};       //get argc, create string itself
    if ((err = SVC_fgets(io_dev, user_cmd, MAXLEN)) != 0){ //gets user input up to \n
      error_checker(err); //print error message
      continue; //discard the string and try again
    }
    if (user_cmd == NULL){ //user command was empty
      error_checker(E_NOINPUT);
      continue;
    }
    uint16_t cmdLen = strlen(user_cmd);
    if ((err = quote_check(user_cmd, cmdLen)) != 0){ //checks for valid number of quotation marks
      error_checker(err); //print error message
      continue; //discard the string and try again
    }
    char user_cmd_clean[MAXLEN] = {NULLCHAR};
    if ((err = parse_string(user_cmd, user_cmd_clean, arg_len, cmdLen)) != 0){ //parses string for argc
      error_checker(err); //print error message
      continue; //discard the string and try again
    }
    //SVC_fputs(io_dev, "\n", 1);
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
      size_t arrSize = sizeof(char)*(arg_len[i]+1);
      argv[i]=(char*)SVC_malloc(arrSize);
      if (argv[i] == NULL) {
        error_checker(E_MALLOC);
        continue;
      }
      memset(argv[i], '\0', arrSize);
      if (MYFAT_DEBUG){
          printf("argument %d: ", i);
      }
      int j = 0;
      for (; j < arg_len[i]; j++){
        argv[i][j] = user_cmd_clean[user_cmd_offset + j];
        if (MYFAT_DEBUG){
      	  printf("%c", argv[i][j]);
        }
      }
      if (MYFAT_DEBUG){
          printf("\n");
      }
      argv[i][j] = NULLCHAR;
      user_cmd_offset += j;
    }
    //check if command exists in struct
    for (int i = 0; i < NUMCOMMANDS; i++){
      if (MYFAT_DEBUG){
      	printf("%s compared to %s \n", argv[0], commands[i].name);
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
  //Error; infinite loop was escaped
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
it is an unsigned int or long. If it is not an integer or some other error has occurred, returns 0.*/
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

/*Helper function parses a user string str and returns it in hex, octal or decimal form, if 
it is an long long integer. If it is not an integer or some other error has occurred, returns 0.*/
long long hex_dec_oct_ll(char* str){
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
      return strtoll(str, NULL, 16); //return hex
    }
    if (check_str == FALSE){
      return 0;
    }
    return strtoll(str, NULL, 8); //return octal
}
  if (check_str == FALSE){
    return 0;
  }
  return strtoll(str, NULL, 10); //return decimal
}
