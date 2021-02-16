#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "mymalloc.h"

/*    Your code may use only the following systems calls or library
   functions:
     malloc free    exit    fgetc   feof
     ferror fputc   fputs   fprintf sprintf
     snprintf   vsprintf    vsnprintf   gettimeofday
     memcpy memmove strcpy  strncpy strcat
     strncat    memcmp  strcmp  strncmp memchr
     strchr strcspn strpbrk strrchr strspn
     strstr strtok  memset  strerror    strlen */

char* mem_array = NULL;

int arr_empty = TRUE;

void *myMalloc(unsigned int size){
    if (size > MAX){
        fprintf(stdout, "Error: not enough RAM available \n");
        return NULL;
    }
    if (mem_array == NULL){
        if ((mem_array = (char *)malloc(128 * MEGA_BYTE)) == NULL){
           perror("malloc"); return NULL;
        }
        for(int i=0;i<size;i++){
            mem_array[i] = i;
            printf("%c",mem_array[i]);
        }
        mem_array[MAX] = 'N';
        printf("%c \n",mem_array[MAX]);
    }
    return 0;
}

int myFreeErrorCode(void *ptr){


    //finally ...
    if (arr_empty == TRUE){
        free(mem_array);
        if (mem_array != NULL){
           perror("free"); exit(2);
        }
    }

    return 0;
}

void myFree(void *ptr){
    //TODO
    return;
}