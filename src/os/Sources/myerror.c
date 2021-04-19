#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mymalloc.h"
#include "myerror.h"
#include "devices.h"
#include "uart.h"
#include "uartNL.h"
#include "simpleshell.h"
#include "svc.h"
#include "SDHC_FAT32_Files.h"
#include "intSerialIO.h"

struct _errordesc errordesc[] = {
    { E_SUCCESS, "No error \n" },
    { E_CONSTRUCTION, "Under construction \n" },
    { E_CMD_NOT_FOUND, "Invalid command -- type help for a list \n" },
    { E_SYSCALL, "System call failed \n" },
    { E_NUMARGS, "Wrong number of arguments \n" },
    { E_NOINPUT, "No input or invalid input -- type help for more \n"},
    { E_TOO_LONG, "Input longer than the maximum allowable characters (256) \n" },
    { E_MALLOC, "Failed to allocate memory \n" },
    { E_INF, "End of infinite loop reached \n" },
    { E_FREE, "Free: Invalid pointer \n" },
    { E_EMPTYMEM, "There is no memory allocated. Freeing block. \n"},
    { E_FREE_PERM, "Your PID does not have permission \n"},
    { E_MEMCHK, "memchk failed \n"},
    { E_UNFREE, "There is no free slot for this request \n"},
    { E_EOF, "End of file reached \n"},
    { E_DEV, "Device handling error \n"},
    { E_NOFS, "No file system found \n"},
    { E_IO, "I/O error \n"},
    { E_SEARCH, "Search was unsuccessful \n"},
    { E_DIRENTRY, "This is a directory entry \n"}
};

#ifndef BUFFER_SIZE
#define BUFFER_SIZE MAXLEN
#endif

int error_checker(int return_value){
  //Checks inputted error values against a struct and prints the appropriate error string, when possible
  if (return_value < 0 && return_value >= (-1*NUMCODES)){
    char buffer[BUFFER_SIZE];
    int length = snprintf(buffer, BUFFER_SIZE, errordesc[-1*return_value].message);
    if (length >= BUFFER_SIZE) {
      return E_TOO_LONG;
    }
    char* output = myMalloc(64);
    sprintf(output, "error %d: %s \r\n", return_value, buffer);
	SVC_fputs(io_dev, output, strlen(output));
    if (MYFAT_DEBUG){
    	printf(output);
    }
	myFree(output);
    if (return_value == E_NUMARGS){
      return E_NUMARGS;
    }
  }
  else if (return_value == 0){
    return 0;
  }
  else{
	char* output = SVC_malloc(64);
	sprintf(output, "Unknown error %d: \r\n", return_value);
	putsNLIntoBuffer(output);
	myFree(output);
  }
  return 0;
}
