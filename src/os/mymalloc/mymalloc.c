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
     strstr strtok  memset  strerror    strlen 
     
     TODO: In preparation for later problem sets, in this problem set you should
allocate a single PCB (Process Control Block) struct that contains a
place-holder PID (Process ID) number.  The PID in the PCB should be
set to zero for now.  The PCB should be pointed to by a global
file-scope variable named, say, currentPCB.  You should implement an
accessor function named getCurrentPID that returns the PID contained
in the PCB of the current process (i.e., in the PCB pointed to by
currentPCB).  For now, because our PID is always zero, it will always
return zero.  The value returned by this function will be used to tag
each region of storage when that region is allocated via myMalloc.  In
later problem sets, you will be allocating multiple PCBs.
     
     */

static int node_count = 0;

struct pcb op_sys = {"OS", 0};

struct pcb* currentPCB = &op_sys;

struct mem_region* first = NULL;

int round_size(int size){
    if (size % 8 != 0){
        size = (size - (size % 8))+8; //round size up to double word boundary
    }
    return size;
}

struct mem_region* init_struct(struct mem_region* first){
    if ((first = (struct mem_region*)malloc(128 * MEGA_BYTE)) == NULL){
        perror("malloc"); return NULL;
    }
    else{
        node_count += 1;
        first->free |= TRUE;
        first->size = MAX - MEMSTRUCT;
        first->pid = currentPCB->pid;
        fprintf(stdout, "Free's address is %p \n", &first);
    }
    return first;
    //subdivide memory
}

struct mem_region* subdivide(struct mem_region* mem, int size){
    //1. Breaks up the passed-in region
    //2. Does this by allocating from the end of the region.
    struct mem_region* next = &mem->data[0] + mem->size; // takes you all the way to the end of the region 
    next -= MEMSTRUCT/sizeof(next); // pointer backs up so it now points to the correct spot in memory 
    next -= size/sizeof(next);
    next->free |= FALSE; //marks the new region as allocated (since a new region will always be requested)
    next->size = size;
    next->pid = currentPCB->pid;
    node_count += 1;
    first->size -= size + MEMSTRUCT; //shrink the size of the first block
    return &(next->data[0]);
}

struct mem_region* first_fit(struct mem_region* temp, int size){
    /*Because the operating system I am writing is likely to be used mostly for small programs and will not be running
    any massively parallel or RAM-intensive applications for now, and because the NXP/Freescale Hardware is quite
    limited in its capabilities, I decided the speed of first-fit, combined with merging adjacent regions when free is called,
    was the optimal choice in thhis situation. Although first-fit can lead to memory fragmentation over time, my operating
    system is not capable of running enough concurrent programs for this to be as much of an issue as speed at the moment.*/
    for (int i = 0; i < node_count; i++){
        if ((temp->size >= size) && (temp->free != FALSE)){ // if it's a perfect fit, return it
            return temp;
        }
        else{ //walk the list
            temp = walk_struct(temp);
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

int free_match(struct mem_region* temp, void* ptr){
    struct mem_region* prev;
    for (int i = 0; i < node_count; i++){
        if (ptr == &(temp->data[0])){ // is ptr identical to this address?
            if (temp->free == FALSE){ // throws error on attempt to free an already freed region
                temp->free |= TRUE;
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
                return 0;
            }
        }
        else {
        prev = temp; //previous gets current
        temp = walk_struct(temp);  //current gets next. TODO: not finding the next block
        }
    }
    return E_FREE;
}

int empty_mem_check(struct mem_region* temp){
    int all_free = TRUE;
    for (int i = 0; i < node_count; i++){
        if (temp->free == FALSE){
            all_free = FALSE;
        }
        temp = walk_struct(temp); //current gets next. TODO: not finding the next block
    }
    if (all_free == TRUE){
        return E_EMPTYMEM;
    }
    return 0;
}

struct mem_region* walk_struct(struct mem_region* this_region){
    int incr_size = this_region->size;
    this_region = &this_region->data[0];
    this_region += incr_size/sizeof(this_region);
    return this_region;
}

void memoryMap(struct mem_region* first){
    fprintf(stdout,     "-------------MEMORYMAP--------- \n\n");
    fprintf(stdout,     "| ENTRY # | FREE | SIZE | PID | \n");
    fprintf(stdout,     "------------------------------- \n");
    struct mem_region* temp = first;
    for (int i = 0; i < node_count; i++){
        char* bool_str = NULL;
        if (temp->free == FALSE){
            bool_str = "False";
        }
        else{
            bool_str = "True ";
        }
        fprintf(stdout, "|   %d   |  %s  |  %d  | %d |\n", i+1, bool_str, temp->size, temp->pid);
        fprintf(stdout, "------------------------------- \n");
        temp = walk_struct(temp); //current gets next. TODO: not finding the next block
    }
    //outputs to stdout a map of all used and free regions in the 128M byte region of memory.
}

void* insert_at_tail(struct mem_region* first, int size){
    //TODO -- protect against having reached end of region. Do I still need this?
    struct mem_region* my_ptr = NULL;
    struct mem_region* temp = first;
    my_ptr = first_fit(temp, size);
    if (my_ptr == NULL){
        return NULL;
    }
    else{
        my_ptr = subdivide(my_ptr, size);
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
        if ((first = init_struct(first)) == NULL){ //build initial struct
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
    else if (empty_mem_check(first) == E_EMPTYMEM){
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