#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myerror.h"

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
    { E_FREE_PERM, "Your PID does not have permission to free this block \n"},
    { E_MEMCHK, "memchk failed \n"}
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
    printf("error %d: %s", return_value, buffer);
    if (return_value == E_NUMARGS){
      return E_NUMARGS;
    }
    if (return_value == E_TOO_LONG){
      int c;
      while ((c = fgetc(stdin) != '\n') && (c != EOF));
    }
  }
  else if (return_value == 0){
    return 0;
  }
  else{
    printf("Unknown error %d: \n", return_value);
  }
  return 0;
}
