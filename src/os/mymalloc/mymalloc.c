#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "mymalloc.h"
#include "helpers/myerror.h"

static int node_count = 0;

struct pcb op_sys = {"OS", 0};

struct pcb* currentPCB = &op_sys;

struct mem_region* first = NULL;

/* round_size accepts as a parameter a size, in bytes, and returns a size, in bytes,
rounded up to the nearest double word boundary.  */
int round_size(int size){
    if (size % DOUBLE_WORD != 0){
        size = (size - (size % DOUBLE_WORD))+DOUBLE_WORD; //round size up to double word boundary
    }
    return size;
}

/* init_struct accepts as a parameter a struct representing the first region of memory
allocated to myMalloc, attempts to call malloc to acquire the necessary memory, and returns an error if that attempt fails. 
init_struct then fills in the correct values for 'free', 'size' and 'pid' for the initial struct. Finally, it updates the
global variable for total node count and returns the modified struct. */
struct mem_region* init_struct(struct mem_region* first){
    if ((first = (struct mem_region*)malloc(MAX)) == NULL){
        perror("malloc"); return NULL;
    }
    else{
        node_count += 1;
        first->free = TRUE;
        first->size = MAX - MEMSTRUCT;
        first->pid = getCurrentPid();
    }
    return first;
    //subdivide memory
}

/* subdivide accepts as a parameter a struct representing a region of memory to be subdivided and a size in bytes to be 
broken off from the larger region of memory. After subdividing, it fills in the correct values for 'free', 'size' and 
'pid' for the initial struct. Finally, it updates the global variable for total node count and returns the modified struct. */
struct mem_region* subdivide(struct mem_region* mem, int size){
    struct mem_region* next = &mem->data[0] + mem->size; // takes you all the way to the end of the region 
    next -= MEMSTRUCT/sizeof(next); // pointer backs up so it now points to the correct spot in memory 
    next -= size/sizeof(next);
    next->free = FALSE; //marks the new region as allocated (since a new region will always be requested)
    next->size = size;
    next->pid = getCurrentPid();
    node_count += 1;
    first->size -= size + MEMSTRUCT; //shrink the size of the first block
    return &(next->data[0]);
}

/*first_fit accepts as a parameter a struct representing a region of memory and a size in bytes to be 
allocated. It then walks through each address in memory until it finds a free region greater than or equal to
the required size, in bytes. Finally, it returns a pointer to the region it has found, or, if it has not found such a 
region, it returns NULL. */
struct mem_region* first_fit(struct mem_region* temp, int size){
    for (int i = 0; i < node_count; i++){
        if ((temp->size >= size + MEMSTRUCT) && (temp->free != FALSE)){ // if it's a fit, return it
            return temp;
        }
        else{ //walk the list
            temp = walk_struct(temp);
        }
    }
    return NULL; //eventually, return NULL
}

/*compact_prev accepts as a parameter a struct representing the previous region of memory and a size in bytes representing the 
size of the region that was just freed. If it finds the previous region is also free, it merges the two free regions together and
updates the global node_count variable. */
void compact_prev(struct mem_region* prev, int size){
    if (prev->free != FALSE){
        prev->size += MEMSTRUCT; //expand previous free block to include newly freed one
        prev->size += size;
        node_count -= 1;
    }
}

/*compact_next accepts as a parameter a struct representing the freed region of memory. 
If it finds the next region is also free, it merges the two free regions together and
updates the global node_count variable. */
void compact_next(struct mem_region* curr){
    struct mem_region* next = &curr->data[0] + curr->size;
    if (next->free != FALSE){
        curr->size += MEMSTRUCT; //expand previous free block to include newly freed one
        curr->size += next->size;
        node_count -= 1; 
    }
}

/* You should implement an accessor function named getCurrentPID that returns the PID contained
in the PCB of the current process (i.e., in the PCB pointed to by currentPCB). */
uint8_t getCurrentPid(){
    return currentPCB->pid;
}

/*walk_struct accepts as a parameter a struct representing a region of memory and returns the 
address of the next struct in memory. */
struct mem_region* walk_struct(struct mem_region* this_region){
    int incr_size = this_region->size;
    this_region = &this_region->data[0];
    this_region += incr_size/sizeof(this_region);
    return this_region;
}

/*memoryMap prints the contents of memory in their entirety. If memory is empty, it prints NULL.*/
void memoryMap(void){
    fprintf(stdout,     "-------------MEMORYMAP--------- \n\n");
    fprintf(stdout,     "| ENTRY # | FREE | SIZE | PID | \n");
    fprintf(stdout,     "------------------------------- \n");
    struct mem_region* temp = first;
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

/*bounds accepts as a parameter a void* ptr. It then finds the appropriate
position in memory (if such exists) and returns the maximum amount memcheck may fill from its given
position, as well as modifying a pointer to an offset value indicating how far memcheck's pointer 
was from the start of the region.*/
unsigned int bounds(void* ptr){
    struct mem_region* mem_ptr = (struct mem_region*)ptr;
    struct mem_region* temp = first;
    struct mem_region* prev;
    if (mem_ptr < first){
        return 0;
    }
    for (int i = 0; i < node_count; i++){
        if ((mem_ptr - MEMSTRUCT/sizeof(struct mem_region)) > temp){ 
            prev = temp;
            if (i < node_count - 1){
                temp = walk_struct(temp);
            }
            else if (i == node_count - 1){ // mem_ptr points either into or past the last region
                long bounds = (unsigned char*)mem_ptr - &temp->data[0];
                if (bounds > temp->size){ // mem-ptr points past the last region
                    return 0;
                }
                // *offset = bounds;
                return temp->size - bounds; //return the difference
            }
        }
        else{
            long bounds = (unsigned char*)mem_ptr - &prev->data[0]; //overcorrect (jump back too far)
            bounds -= prev->size + sizeof(struct mem_region) - temp->size;
            return bounds;
        }
    }
    return 0;
}

/*free_match accepts as a parameter a struct representing a region of memory and a void pointer representing the
value myFreeErrorCode will ultimately return. For each region in memory, free_match executes a series of error
checks, returning error values where appropriate. If it finds a match in memory, it frees and compacts that
region of memory, as appropriate. Finally, if successful, it returns 0.*/
int free_match(struct mem_region* temp, void* ptr){
    struct mem_region* prev;
    for (int i = 0; i < node_count; i++){
        if (ptr == &(temp->data[0])){ // is ptr identical to this address?
            if (temp->free == FALSE){ // throws error on attempt to free an already freed region
                temp->free = TRUE;
                if (i == 0){ //if at first block, check ahead only
                    compact_next(temp);
                }
                else if (i == node_count - 1){ // if at last block, check behind only
                    compact_prev(prev, temp->size);
                }
                else {
                    compact_next(temp);
                    compact_prev(prev, temp->size); // compacts next and prior regions of memory
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

/*insert_at_tail accepts as a parameter a struct representing a region of memory and a size in bytes. 
insert_at_tail first checks first_fit to find an appropriate place in memory. If it finds one, it 
subdivides memory and returns the pointer to the newly subdivided region. If it cannot find a match,
it returns NULL.*/
void* insert_at_tail(struct mem_region* first, int size){
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

/*The myMalloc function is declared as taking an unsigned int as its
only parameter and returning a pointer to void. The "size" parameter 
is the size in bytes of the storage needed by the caller.  The myMalloc 
function should allocate an appropriately sized region of memory and it 
should return a pointer to (i.e., the address of) the first byte of that region. */
void *myMalloc(unsigned int size){
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

/*The behavior of myFreeErrorCode is the same as for myFree except that
myFreeErrorCode returns an int to indicate success or failure of the
storage deallocation request. */
int myFreeErrorCode(void *ptr){
    struct mem_region* temp = first;
    int match_val = free_match(temp, ptr);
    if (match_val == E_FREE){ //case: pointer not found in memory 
        return E_FREE;
    }
    if (temp->pid != getCurrentPid()){ //case: PIDs do not match
        return E_FREE_PERM;
    }
    else{
        return 0; //success
    }
    return E_FREE; //should never reach this point
}

/*The myFree function is declared as taking a pointer to void as its
only parameter and not returning anything. The "ptr" parameter is a pointer 
to a region of memory previously allocated by the myMalloc function.  
The myFree function deallocates the entire region of memory pointed to by the parameter. */
void myFree(void *ptr){
    myFreeErrorCode(ptr); //calls myFreeErrorCode and ignores return value
    return;
}