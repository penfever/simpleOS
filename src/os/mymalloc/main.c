#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "mymalloc.h"
#include "mymalloc.c"
#include "../helpers/myerror.h"
#include "../helpers/myerror.c"

int main(void){
    char* normal;
    if ((normal = (char *)myMalloc(32)) == NULL){
        error_checker(E_MALLOC);
    }
    int* odd;
    if ((odd = (int *)myMalloc(sizeof(int*)-1)) == NULL){
        error_checker(E_MALLOC);
    }
    char* another;
    if ((another = (char *)myMalloc(512)) == NULL){
        error_checker(E_MALLOC);
    }
    int* too_small;
    if ((too_small = myMalloc(0)) == NULL){
        error_checker(E_MALLOC);
    }
    int* negative;
    if ((negative = myMalloc(-10)) == NULL){
        error_checker(E_MALLOC);
    }
    int* too_big;
    if ((too_big = myMalloc(129*1024*1024)) == NULL){
        error_checker(E_MALLOC);
    }
    memoryMap(first);
    int var = 4;
    for (int i = 0; i < 31; i++){
        normal[i] = 'c';
    }
    *odd = var;
    normal[31] = '\0';
    for (int i = 0; i < 511; i++){
        another[i] = 'c';
    }
    another[511] = '\0';
    *odd += 2;
    int errcode;
    errcode = myFreeErrorCode((void*)93234567); //this is an invalid pointer and should be discarded
    fprintf(stdout, "errcode is %d \n", errcode);
    memoryMap(first);
    errcode = myFreeErrorCode(odd);
    odd = myMalloc(sizeof(int*)-1); //this should find the empty slot I just created and fill it
    *odd = var;
    *odd += 4;
    fprintf(stdout, "Odd is now %d \n", *odd);
    errcode = myFreeErrorCode(odd);
    errcode = myFreeErrorCode(normal); //this should cause a merge in the free blocks
    memoryMap(first);
    
    errcode = myFreeErrorCode(normal); // double free should cause error
    fprintf(stdout, "errcode is %d \n", errcode);

    myFreeErrorCode(another); //this should free everything
    exit(0);
}