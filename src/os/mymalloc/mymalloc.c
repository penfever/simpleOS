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

static int node_count = 0;

struct mem_region* first = NULL;

int round_size(int size){
    if (size % 8 != 0){
        size = size + size % 8; //round size up to double word boundary
    }
    return size;
}

struct mem_region* init_struct(struct mem_region* first, int size){
    if ((first = (struct mem_region*)malloc(128 * MEGA_BYTE)) == NULL){
        perror("malloc"); return NULL;
    }
    else{
        node_count += 1;
        first->free = TRUE;
        first->size = MAX - MEMSTRUCT;
        first->pid = 1;
    }
    return first;
    //subdivide memory
}

void* subdivide(struct mem_region* mem, int size){
    //1. Always breaks up the first region, which will always be the largest region
    //2. Does this by allocating from the end of the region.
    if (size > mem->size){
        return NULL; //TODO: possibly send a clearer error message to stdout?
    }
    struct mem_region* next = &mem->data[0] + mem->size; // takes you all the way to the end of the large malloc region VERIFIED
    next -= MEMSTRUCT + size; // backs up so it now points to the correct spot in memory VERIFIED
    next->free = FALSE; //marks the new region as allocated (since a new region will always be requested)
    next->size = size;
    next->pid = 1;
    node_count += 1;
    return &(next->data[0]);
}

void* perfect_fit(struct mem_region* temp, int size){
    for (int i = 0; i < node_count; i++){
        if (temp->size == size && temp->free == TRUE){ // if it's a perfect fit, return it
            temp->free = FALSE;
            return &(temp->data[0]);
        }
        else{ //walk the list
            temp = &temp->data[0] + temp->size;
        }
    }
    return NULL; //eventually, return NULL
}

void* insert_at_tail(struct mem_region* first, int size){
    //TODO -- protect against having reached end of region
    struct mem_region* my_ptr = NULL;
    struct mem_region* temp = first;
    if ((my_ptr = perfect_fit(temp, size)) == NULL){
        my_ptr = subdivide(first, size);
    }
    return (void*)my_ptr;
}

void *myMalloc(unsigned int size){
    void* return_ptr = NULL;
    if (size > MAX || size <= 0){
        fprintf(stdout, "Error: not enough RAM available or no RAM requested \n");
        return return_ptr;
    }
    size = round_size(size);
    if (first == NULL){
        if ((first = init_struct(first, size)) == NULL){ //build initial struct
            return NULL;
        }
    }
    return_ptr = insert_at_tail(first, size);
    return return_ptr;
}

int myFreeErrorCode(void *ptr){
    int arr_empty = FALSE;
    struct mem_region* temp = first;
    while (temp->data != ptr && temp->data != NULL){
        temp = temp->data; //TODO: Fix traversal, assumes pointer at data
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