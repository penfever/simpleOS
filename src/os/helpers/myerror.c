#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myerror.h"

int error_checker(int return_value){
  //Checks inputted error values against a struct and prints the appropriate error string, when possible
  if (return_value < 0 && return_value >= (-1*NUMCODES)){
    char* error_string = errordesc[-1*return_value].message;
    fprintf(stderr, "error %d: %s", return_value, error_string);
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
    fprintf(stderr, "Unknown error %d: \n", return_value);
  }
  return 0;
}