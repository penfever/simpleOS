#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mymalloc.h"
#include "mymalloc.c"
#include "../helpers/myerror.h"
#include "../helpers/myerror.c"

int main(void){
    int* err;
    err = myMalloc(1024);
    // if (err == NULL){
    //     error_checker(E_MALLOC);
    //     exit(E_MALLOC);
    // }
    myFreeErrorCode(sizeof(int));
    exit(0);
}