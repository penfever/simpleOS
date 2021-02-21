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
        size = (size - (size % 8))+8; //round size up to double word boundary
    }
    return size;
}

struct mem_region* init_struct(struct mem_region* first, int size){
    if ((first = (struct mem_region*)malloc(128 * MEGA_BYTE)) == NULL){
        perror("malloc"); return NULL;
    }
    else{
        node_count += 1;
        first->free |= TRUE;
        first->size = MAX - MEMSTRUCT;
        first->pid = 1;
        fprintf(stdout, "Free's address is %p \n", &first);
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
    struct mem_region* next = &mem->data[0] + mem->size; // takes you all the way to the end of the large malloc region 
    next -= MEMSTRUCT/sizeof(next); // pointer backs up so it now points to the correct spot in memory 
    next -= size/sizeof(next);
    next->free |= FALSE; //marks the new region as allocated (since a new region will always be requested)
    next->size = size;
    next->pid = 1;
    node_count += 1;
    first->size -= size + MEMSTRUCT; //shrink the size of the first block
    return &(next->data[0]);
}

void* perfect_fit(struct mem_region* temp, int size){
    for (int i = 0; i < node_count; i++){
        if (temp->size == size && temp->free != FALSE){ // if it's a perfect fit, return it
            temp->free |= FALSE;
            return &temp->data[0];
        }
        else{ //walk the list
            temp = &temp->data[0] + temp->size;
        }
    }
    return NULL; //eventually, return NULL
}

void compact_prev(struct mem_region* prev, int size){
    if (prev->free != FALSE){
        prev->size += MEMSTRUCT; //expand previous free block to include newly freed one
        prev->size += size;
        node_count -= 1;
    }
}

void compact_next(struct mem_region* prev, int size){
    struct mem_region* next = &prev->data[0] + prev->size;
    if (next->free != FALSE){
        prev->size += MEMSTRUCT; //expand previous free block to include newly freed one
        prev->size += size;
        node_count -= 1; 
    }
    }
}

int free_match(struct mem_region* temp, void* ptr){
    int all_free = TRUE;
    int found = FALSE;
    struct mem_region* prev;
    for (int i = 0; i < node_count; i++){
        if (ptr == &(temp->data[0])){ // is ptr identical to this address?
            if (temp->free == FALSE){ // throws error on attempt to free an already freed region
                temp->free |= TRUE;
                found = TRUE;
                if (i == 0){ //if at first block, check ahead only
                    compact_next(temp, temp->size);
                }
                else if (i == node_count - 1){ // if at last block, check behind only
                    compact_prev(prev, temp->size);
                }
                else {
                    compact_prev(prev, temp->size); // compacts next and prior regions of memory
                    compact_next(temp, temp->size);
                }
            }
        }
        if (temp->free == FALSE){ //if any item is not free
            all_free = FALSE;
        }
        prev = temp; //previous gets current
        int incr_size = temp->size;
        temp = &temp->data[0];
        temp += incr_size/sizeof(temp); //current gets next. TODO: not finding the next block
    }
    if (all_free == TRUE){
        return E_EMPTYMEM;
    }
    else if (found == TRUE){
        return 0;
    }
    else{
        return E_FREE;
    }
}

void* insert_at_tail(struct mem_region* first, int size){
    //TODO -- protect against having reached end of region. Do I still need this?
    struct mem_region* my_ptr = NULL;
    struct mem_region* temp = first;
    if ((my_ptr = perfect_fit(temp, size)) == NULL){
        my_ptr = subdivide(first, size);
    }
    return (void*)my_ptr;
}

void *myMalloc(unsigned int size){
    void* return_ptr = NULL;
    if (size >= MAX - MEMSTRUCT || size <= 0){
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
    //TODO: what if someone passes in a very large or very small garbage number?
    struct mem_region* temp = first;
    int match_val = free_match(temp, ptr);
    if (match_val == E_FREE){ //case: match_val error
        return E_FREE;
    }
    else if (match_val == E_EMPTYMEM){ //case: all of memory is empty
        free(first);
        if (first != NULL){
           perror("free"); exit(2);
        }
        return 0;
    }
    else{
        return 0; //success
    }
    return E_FREE; //should never reach this point
}

void myFree(void *ptr){
    //TODO
    return;
}