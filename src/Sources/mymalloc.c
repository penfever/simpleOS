#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "breakpoint.h"
#include "mymalloc.h"
#include "myerror.h"
#include "devices.h"
#include "uart.h"
#include "uartNL.h"
#include "intSerialIO.h"
#include "sdram.h"
#include "simpleshell.h"
#include "procs.h"
#include "svc.h"
#include "lcdc.h"

static int node_count = 0;

struct mem_region* first = NULL;

char* g_lower_bound = NULL;

char* g_upper_bound = NULL;

/* round_size accepts as a parameter a size, in bytes, and returns a size, in bytes,
rounded up to the nearest double word boundary.  */
int round_size(int size) {
    if (size % DOUBLE_WORD != 0) {
        size = (size - (size % DOUBLE_WORD))+DOUBLE_WORD; //round size up to double word boundary
    }
    return size;
}

/* init_struct accepts as a parameter a struct representing the first region of memory
allocated to myMalloc, attempts to call malloc to acquire the necessary memory, and returns an error if that attempt fails.
init_struct then fills in the correct values for 'free', 'size' and 'pid' for the initial struct. Finally, it updates the
global variable for total node count and returns the modified struct. */
struct mem_region* init_struct(struct mem_region* first) {
    uint32_t startRegion = (SDRAM_START + LCDC_FRAME_BUFFER_SIZE);
    first = (struct mem_region*)startRegion;
    g_lower_bound = (char *)startRegion; //pointers to lower and upper memory bounds
    g_upper_bound = (char *)SDRAM_END;
    node_count += 1;
    first->free = TRUE;
    first->size = MAX - MEMSTRUCT;
    first->pid = getCurrentPid();
    return first;
}

/* subdivide accepts as a parameter a struct representing a region of memory to be subdivided and a size in bytes to be
broken off from the larger region of memory. After subdividing, it fills in the correct values for 'free', 'size' and
'pid' for the initial struct. Finally, it updates the global variable for total node count and returns the modified struct. */
struct mem_region* subdivide(struct mem_region* mem, int size) {
    char* next_c = (char *)mem;
    next_c += (mem->size - size);
    struct mem_region* next = (struct mem_region*)next_c;// pointer backs up so it now points to the correct spot in memory
    next->free = FALSE; //marks the new region as allocated (since a new region will always be requested)
    next->size = size;
    next->pid = getCurrentPid();
    node_count += 1;
    mem->size -= (size + MEMSTRUCT); //shrink the size of the first block
    return (void*)next->data;
}

/*first_fit accepts as a parameter a struct representing a region of memory and a size in bytes to be
allocated. It then walks through each address in memory until it finds a free region greater than or equal to
the required size, in bytes. Finally, it returns a pointer to the region it has found, or, if it has not found such a
region, it returns NULL. */
struct mem_region* first_fit(struct mem_region* temp, int size) {
    for (int i = 0; i < node_count; i++) {
        if ((temp->size >= size + MEMSTRUCT) && (temp->free != FALSE)) { // if it's a fit and is free, return it
            return temp;
        }
        else { //walk the list
            temp = walk_struct(temp);
            if (temp <= 0) {
                walk_struct_err();
            }
        }
    }
    return NULL; //eventually, return NULL
}

/*compact_prev accepts as a parameter a struct representing the previous region of memory and a size in bytes representing the
size of the region that was just freed. If it finds the previous region is also free, it merges the two free regions together and
updates the global node_count variable. */
void compact_prev(struct mem_region* prev, int size) { //PREV on REGION boundary
    if (prev->free != FALSE) {
        prev->size += MEMSTRUCT; //expand previous free block to include newly freed one
        prev->size += size;
        node_count -= 1;
    }
}

/*compact_next accepts as a parameter a struct representing the freed region of memory.
If it finds the next region is also free, it merges the two free regions together and
updates the global node_count variable. */

void compact_next(struct mem_region* curr) { //CURR on REGION boundary
    struct mem_region* next = (struct mem_region *)(curr->data + curr->size);;
    if (next->free != FALSE) {
        curr->size += MEMSTRUCT; //expand previous free block to include newly freed one
        curr->size += next->size;
        node_count -= 1;
    }
}

/*walk_struct accepts as a parameter a struct representing a region of memory and returns the
address of the next struct in memory. If it walks past the end of the struct, it returns NULL
and an error.*/
struct mem_region* walk_struct(struct mem_region* this_region) { //REGION boundary
    if ((char*)this_region > g_upper_bound || (char*)this_region < g_lower_bound) {
        return NULL;
    }
    char* char_region = (char *)(this_region->data + this_region->size);
    return (struct mem_region*)char_region;
}

/*memoryMap prints the contents of memory in their entirety. If memory is empty, it prints NULL.*/
void memoryMap(void) {
    char* output = 						"-------------------MEMORYMAP--------------- \n\n";
    SVC_fputs(io_dev, output, strlen(output));
    output = 							"|    ENTRY ADDR    | FREE |   SIZE   | PID | \n";
    SVC_fputs(io_dev, output, strlen(output));
    output = 							"------------------------------------------- \n";
    SVC_fputs(io_dev, output, strlen(output));
    struct mem_region* temp = first;
    int total_size = 0;
    for (int i = 0; i < node_count; i++) {
        char* bool_str = NULL;
        total_size += temp->size;
        total_size += MEMSTRUCT;
        if (temp->free == FALSE) {
            bool_str = "False";
        }
        else {
            bool_str = "True ";
        }
        char* memDisplay = SVC_malloc(256); //uart gets a map of all used and free regions in the 128M byte region of memory.
        sprintf(memDisplay, 			"|   %p   |  %s  |  %05d  | %d |\n", temp, bool_str, temp->size, temp->pid);
        SVC_fputs(io_dev, memDisplay, strlen(memDisplay));
        char* output2 = 				"-------------------------------------------- \n";
        SVC_fputs(io_dev, output2, strlen(output2));
        SVC_free(memDisplay);
        temp = walk_struct(temp);
        if (temp <= 0) {
            walk_struct_err();
        }
    }
    char* memDisplay2 = SVC_malloc(256);
    sprintf(memDisplay2, "TOTAL MEMORY SIZE = %d \n", total_size);
    SVC_fputs(io_dev, memDisplay2, strlen(memDisplay2));
    SVC_free(memDisplay2);
}

void walk_struct_err() {
    if (MYFAT_DEBUG) {
        printf("walk_struct error (check memory bounds)\n");
    }
    cmd_exit(1, NULL);
}

/*bounds accepts as a parameter a void* ptr. It then finds the appropriate
position in memory (if such exists) and returns the maximum amount memcheck may fill from its given
position, as well as modifying a pointer to an offset value indicating how far memcheck's pointer
was from the start of the region.*/
uint32_t bounds(void* ptr) {
    struct mem_region* mem_ptr = (struct mem_region*)ptr;
    struct mem_region* temp = first;
    struct mem_region* prev;
    if (mem_ptr < first) {
        return 0;
    }
    for (int i = 0; i < node_count; i++) {
        if ((mem_ptr - MEMSTRUCT/sizeof(struct mem_region)) > temp) {
            prev = temp;
            if (i < node_count - 1) {
                temp = walk_struct(temp);
                if (temp <= 0) {
                    walk_struct_err();
                }
            }
            else if (i == node_count - 1) { // mem_ptr points either into or past the last region
                long bounds = (unsigned char*)mem_ptr - &temp->data[0];
                if (bounds > temp->size) { // mem-ptr points past the last region
                    return 0;
                }
                // *offset = bounds;
                return temp->size - bounds; //return the difference
            }
        }
        else {
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

int free_match(struct mem_region* temp, void* ptr) { //PTR is on data boundary. TEMP is on region boundary.
    struct mem_region* prev = NULL;
    for (int i = 0; i < node_count; i++) {
        if (ptr == (void*)temp->data) { // is ptr identical to this address? DATA to DATA comparison
            if (temp->free == FALSE) { // throws error on attempt to free an already freed region
                temp->free = TRUE;
                temp->pid = SHELLPID; //all free memory should be owned by shell
                if (i == 0) { //if at first block, check ahead only
                    compact_next(temp);
                }
                else if (i == node_count - 1) { // if at last block, check behind only
                    compact_prev(prev, temp->size);
                }
                else {
                    compact_next(temp);
                    compact_prev(prev, temp->size); // compacts next and prior regions of memory
                }
                return 0;
            }
            else {
                return E_FREE;
            }
        }
        else {
            prev = temp; //previous gets current
            temp = walk_struct(temp);  //current gets next.
            if (temp <= 0) {
                walk_struct_err();
            }
        }
    }
    return E_FREE;
}

/*The myMalloc function is declared as taking an unsigned int as its
only parameter and returning a pointer to void. The "size" parameter
is the size in bytes of the storage needed by the caller.  The myMalloc
function should allocate an appropriately sized region of memory and it
should return a pointer to (i.e., the address of) the first byte of that region. */
void* myMalloc(unsigned int size) {
    disable_interrupts();
    void* return_ptr = NULL;
    if (size >= MAX - MEMSTRUCT || size <= 0) { //returns error if size is invalid
        enable_interrupts();
        return NULL;
    }
    size = round_size(size);
    if (first == NULL) {
        if ((first = init_struct(first)) == NULL) { //build initial struct
            enable_interrupts();
            return NULL;
        }
    }
    return_ptr = first_fit(first, size);
    if (return_ptr == NULL) {
        enable_interrupts();
        return NULL;
    }
    else {
        return_ptr = subdivide((struct mem_region*)return_ptr, size);
    }
    enable_interrupts();
    return return_ptr;
}

/*The behavior of myFreeErrorCode is the same as for myFree except that
myFreeErrorCode returns an int to indicate success or failure of the
storage deallocation request. */
int myFreeErrorCode(void *ptr) {
    disable_interrupts();
    struct mem_region* memPtr = (struct mem_region*)ptr - 1; //steps back from data region to mem-region
    int match_val = free_match(first, ptr);
    if (match_val == E_FREE) { //case: pointer not found in memory
        enable_interrupts();
        return E_FREE;
    }
    if (memPtr->pid != getCurrentPid()) { //case: PIDs do not match
        enable_interrupts();
        return E_FREE_PERM;
    }
    else {
        enable_interrupts();
        return 0; //success
    }
    enable_interrupts();
    return E_FREE; //should never reach this point
}

/*The myFree function is declared as taking a pointer to void as its
only parameter and not returning anything. The "ptr" parameter is a pointer
to a region of memory previously allocated by the myMalloc function.
The myFree function deallocates the entire region of memory pointed to by the parameter. */
void myFree(void *ptr) {
    int err = myFreeErrorCode(ptr); //calls myFreeErrorCode and ignores return value
    if (MYFAT_DEBUG) {
        printf("%d \n", err);
    }
    return;
}
