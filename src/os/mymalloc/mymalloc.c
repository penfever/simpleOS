#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "mymalloc.h"
#include "../helpers/linked_list.c"
#include "../helpers/linked_list.h"

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

int global_counter = 0;

void *myMalloc_allocate(char *mem_array){
    void* my_ptr = (&mem_array)[global_counter];
    return my_ptr;
}

void *myMalloc(unsigned int size){
    void* return_ptr = NULL;
    if (size > MAX || size <= 0){
        fprintf(stdout, "Error: not enough RAM available or no RAM requested \n");
        return return_ptr;
    }
    if (mem_array == NULL){
        if ((mem_array = (char *)malloc(128 * MEGA_BYTE)) == NULL){
           perror("malloc"); return return_ptr;
        }
        // for(int i=0;i<size;i++){
        //     mem_array[i] = i;
        //     printf("%c",mem_array[i]);
        // }
        // mem_array[MAX] = 'N';
        // printf("%c \n",mem_array[MAX]);
    }
    if (size % 8 != 0){
        size = size + size % 8; //round up to double word boundary
    }
    return_ptr = myMalloc_allocate(mem_array);
    global_counter += size;
    return return_ptr;
}

int myFreeErrorCode(void *ptr){

    //walk through the entire linked list and verify that this was a previously allocated region of memory.
    //when you reach the correct pointer ...
    //merge it with the one after it and before it, if necessary.
    //if you reach the end of the walk, return null
    //finally ...
    if (arr_empty == TRUE){
        free(mem_array);
        if (mem_array != NULL){
           perror("free"); exit(2);
        }
    }
    //TODO: Add error code returns to error struct, see PSET 2
    return 0;
}

void myFree(void *ptr){
    //TODO
    return;
}