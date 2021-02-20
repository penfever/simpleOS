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
    if ((normal = myMalloc(32)) == NULL){
        error_checker(E_MALLOC);
    }
    int* odd;
    if ((odd = myMalloc(sizeof(int*)-1)) == NULL){
        error_checker(E_MALLOC);
    }
    int* another;
    if ((another = myMalloc(512)) == NULL){
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
    for (int i = 0; i < 31; i++){
        normal[i] = 'z';
    }
    normal[31] = '\0';
    int var = 4;
    odd = &var;
    fprintf(stdout, "Odd is %d \n", *odd);
    fprintf(stdout, "String is %s, length is %d \n", normal, strlen(normal));
    myFreeErrorCode(sizeof(int));
    exit(0);
}