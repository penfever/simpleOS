#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "mymalloc.h"
#include "..\helpers\myerror.h"

static int node_count = 0;

struct pcb op_sys = {"OS", 0};

struct pcb* currentPCB = &op_sys;

struct mem_region* first = NULL;

int round_size(int size){
    /* round_size accepts as a parameter a size, in bytes, 
    and returns a size, in bytes, rounded up to the nearest double word boundary.  */
    if (size % DOUBLE_WORD != 0){
        size = (size - (size % DOUBLE_WORD))+DOUBLE_WORD; //round size up to double word boundary
    }
    return size;
}

struct mem_region* init_struct(struct mem_region* first){
    /* init_struct accepts as a parameter a struct representing the first region of memory
    allocated to myMalloc, attempts to call malloc to acquire the necessary memory, and returns an error if that attempt fails. 
    init_struct then fills in the correct values for 'free', 'size' and 'pid' for the initial struct. Finally, it updates the
    global variable for total node count and returns the modified struct. */
    if ((first = (struct mem_region*)malloc(MAX)) == NULL){
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
    /* subdivide accepts as a parameter a struct representing a region of memory to be subdivided and a size in bytes to be 
    broken off from the larger region of memory. After subdividing, it fills in the correct values for 'free', 'size' and 
    'pid' for the initial struct. Finally, it updates the global variable for total node count and returns the modified struct. */

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
    /*first_fit accepts as a parameter a struct representing a region of memory and a size in bytes to be 
    allocated. It then walks through each address in memory until it finds a free region greater than or equal to
    the required size, in bytes. Finally, it returns a pointer to the region it has found, or, if it has not found such a 
    region, it returns NULL. */
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
    /*compact_prev accepts as a parameter a struct representing the previous region of memory and a size in bytes representing the 
    size of the region that was just freed. If it finds the previous region is also free, it merges the two free regions together and
    updates the global node_count variable. */
    if (prev->free != FALSE){
        prev->size += MEMSTRUCT; //expand previous free block to include newly freed one
        prev->size += size;
        node_count -= 1;
    }
}

void compact_next(struct mem_region* curr){
    /*compact_next accepts as a parameter a struct representing the freed region of memory. 
    If it finds the next region is also free, it merges the two free regions together and
    updates the global node_count variable. */
    struct mem_region* next = &curr->data[0] + curr->size;
    if (next->free != FALSE){
        curr->size += MEMSTRUCT; //expand previous free block to include newly freed one
        curr->size += next->size;
        node_count -= 1; 
    }
}

int empty_mem_check(struct mem_region* temp){
    /*empty_mem_check accepts as a parameter a struct representing a region of memory. 
    It then checks each region of memory to determine whether that region is allocated.
    If no region of memory is allocated, empty_mem_check returns an error code.*/
    int all_free = TRUE;
    for (int i = 0; i < node_count; i++){
        if (temp->free == FALSE){
            all_free = FALSE;
        }
        temp = walk_struct(temp);
    }
    if (all_free == TRUE){
        return E_EMPTYMEM;
    }
    return 0;
}

struct mem_region* walk_struct(struct mem_region* this_region){
    /*walk_struct accepts as a parameter a struct representing a region of memory and returns the 
    address of the next struct in memory. */
    int incr_size = this_region->size;
    this_region = &this_region->data[0];
    this_region += incr_size/sizeof(this_region);
    return this_region;
}

void memoryMap(struct mem_region* first){
    /*memoryMap accepts as a parameter a struct representing the first region of memory and prints
    the contents of memory in their entirety.*/
    fprintf(stdout,     "-------------MEMORYMAP--------- \n\n");
    fprintf(stdout,     "| ENTRY # | FREE | SIZE | PID | \n");
    fprintf(stdout,     "------------------------------- \n");
    struct mem_region* temp = first;
    if (temp == NULL){
        fprintf (stdout, "NULL \n");
        return;
    }
    int total_size = 0;
    for (int i = 0; i < node_count; i++){
        char* bool_str = NULL;
        total_size += temp->size;
        total_size += MEMSTRUCT;
        if (temp->free == FALSE){
            bool_str = "False";
        }
        else{
            bool_str = "True ";
        }
        fprintf(stdout, "|   %d   |  %s  |  %d  | %d |\n", i+1, bool_str, temp->size, temp->pid);
        fprintf(stdout, "------------------------------- \n");
        temp = walk_struct(temp);
    }
    fprintf(stdout,     "TOTAL MEMORY SIZE = %d \n", total_size);
    //outputs to stdout a map of all used and free regions in the 128M byte region of memory.
}

unsigned int bounds(void* ptr){
    /*bounds accepts as a parameter a void* ptr. It then finds the appropriate
    position in memory (if such exists) and returns the size of the referenced memory block.*/
    
    struct mem_region* mem_ptr = (struct mem_region*)ptr;
    struct mem_region* temp = first;
    struct mem_region* prev;
    if (mem_ptr < first){
        return 0;
    }
    for (int i = 0; i < node_count; i++){
        if (mem_ptr > temp){ 
            prev = temp;
            walk_struct(temp);
        }
        else{
            ptrdiff_t diff = mem_ptr - prev;
            return prev->size - diff;
        }
    }
    return 0;
}

int free_match(struct mem_region* temp, void* ptr){
    /*free_match accepts as a parameter a struct representing a region of memory and a void pointer representing the
    value myFreeErrorCode will ultimately return. For each region in memory, free_match executes a series of error
    checks, returning error values where appropriate. If it finds a match in memory, it frees and compacts that
    region of memory, as appropriate. Finally, if successful, it returns 0.*/
    struct mem_region* prev;
    for (int i = 0; i < node_count; i++){
        if (ptr == &(temp->data[0])){ // is ptr identical to this address?
            if (temp->free == FALSE){ // throws error on attempt to free an already freed region
                temp->free |= TRUE;
                if (i == 0){ //if at first block, check ahead only
                    compact_next(temp);
                }
                else if (i == node_count - 1){ // if at last block, check behind only
                    compact_prev(prev, temp->size);
                }
                else {
                    compact_prev(prev, temp->size); // compacts next and prior regions of memory
                    compact_next(temp);
                }
                return 0;
            }
        }
        else {
        prev = temp; //previous gets current
        temp = walk_struct(temp);  //current gets next.
        }
    }
    return E_FREE;
}

void* insert_at_tail(struct mem_region* first, int size){
    /*insert_at_tail accepts as a parameter a struct representing a region of memory and a size in bytes. 
    insert_at_tail first checks first_fit to find an appropriate place in memory. If it finds one, it 
    subdivides memory and returns the pointer to the newly subdivided region. If it cannot find a match,
    it returns NULL.*/
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
    /*The myMalloc function is declared as taking an unsigned int as its
only parameter and returning a pointer to void, as follows:

	void *myMalloc(unsigned int size);

The "size" parameter is the size in bytes of the storage needed by the
caller.  The myMalloc function should allocate an appropriately sized
region of memory and it should return a pointer to (i.e., the address
of) the first byte of that region. */
    void* return_ptr = NULL;
    if (size >= MAX - MEMSTRUCT || size <= 0){ //returns error if size is invalid
        return NULL;
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
    /*The behavior of myFreeErrorCode is the same as for myFree except that
myFreeErrorCode returns an int to indicate success or failure of the
storage deallocation request. */
    struct mem_region* temp = first;
    int match_val = free_match(temp, ptr);
    if (match_val == E_FREE){ //case: pointer not found in memory 
        return E_FREE;
    }
    if (temp->pid != currentPCB->pid){ //case: PIDs do not match
        return E_FREE_PERM;
    }
    else if (empty_mem_check(first) == E_EMPTYMEM){
        free(first);
        perror("free");
        return 0;
    }
    else{
        return 0; //success
    }
    return E_FREE; //should never reach this point
}

void myFree(void *ptr){
    /*The myFree function is declared as taking a pointer to void as its
only parameter and not returning anything, as follows:

	void myFree(void *ptr);

The "ptr" parameter is a pointer to a region of memory previously
allocated by the myMalloc function.  The myFree function deallocates
the entire region of memory pointed to by the parameter. */
    myFreeErrorCode(ptr); //calls myFreeErrorCode and ignores return value
    return;
}