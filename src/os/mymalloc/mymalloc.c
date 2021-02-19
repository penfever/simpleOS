#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "mymalloc.h"
#include "..\helpers\myerror.h"

/*    Your code may use only the following systems calls or library
   functions:
     malloc free    exit    fgetc   feof
     ferror fputc   fputs   fprintf sprintf
     snprintf   vsprintf    vsnprintf   gettimeofday
     memcpy memmove strcpy  strncpy strcat
     strncat    memcmp  strcmp  strncmp memchr
     strchr strcspn strpbrk strrchr strspn
     strstr strtok  memset  strerror    strlen */

static int mem_used = 0;

struct mem_region* first = NULL;

int round_size(int size){
    if (size % 8 != 0){
        size = size + size % 8; //round size up to double word boundary
    }
    return size;
}

void subdivide(struct mem_region* mem){
    struct mem_region* next = mem->data; // SUBDIVISION: this covers the rest of memory. Anytime I am looking at a data address, that's the start of a struct
    next->free = TRUE;
    next->size = 0;
    next->pid = 1;
    next->data = NULL;
}

struct mem_region* init_struct(struct mem_region* first, int size){
    if ((first = (struct mem_region*)malloc(128 * MEGA_BYTE)) == NULL){
        perror("malloc"); return NULL;
    }
    else{
        first->free = TRUE;
        mem_used += size;
        first->size = size;
        first->pid = 1;
        first->data = &first + sizeof(struct mem_region) + size; //TODO: figure out how to convert ptr to and from uint8_t and make sure the ptr arith is working
        subdivide(first);
    }
    return first;
    //subdivide memory
}

void* insert_at_tail(struct mem_region* first, int size){
    //TODO -- protect against having reached end of region
    struct mem_region* temp = first;
    while (temp->data != NULL){
        temp = temp->data; //traversal, assumes pointer at data
    }
    mem_used += size;
    temp->size = size;
    temp->pid = 1; //make this a real PID later
    temp->data = &temp + sizeof(struct mem_region) + size;
    subdivide(temp);
}

void *myMalloc(unsigned int size){
    void* return_ptr = NULL;
    if (size > MAX || size <= 0){
        fprintf(stdout, "Error: not enough RAM available or no RAM requested \n");
        return return_ptr;
    }
    size = round_size(size);
    if (first == NULL){
        if (first = init_struct(first, size) == NULL){ //build initial struct
            return NULL;
        }
        return_ptr = first;
    }
    else {
        return_ptr = insert_at_tail(first, size);
    }
    //todo: modularize function once working
    return return_ptr;
}

int myFreeErrorCode(void *ptr){
    int arr_empty = FALSE;
    struct mem_region* temp = first;
    while (temp->data != ptr && temp->data != NULL){
        temp = temp->data; //traversal, assumes pointer at data
    }
    //walk through the entire linked list and verify that this was a previously allocated region of memory.
    //when you reach the correct pointer ...
    //merge it with the one after it and before it, if necessary.
    //if you reach the end of the walk, return null
    //finally ...
    if (arr_empty == TRUE){
        free(first);
        if (first != NULL){
           perror("free"); exit(2);
        }
    }
    return E_FREE;
}

void myFree(void *ptr){
    //TODO
    return;
}