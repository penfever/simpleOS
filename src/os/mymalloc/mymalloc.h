#ifndef _MYMALLOC_H
#define _MYMALLOC_H
#define TRUE 1
#define FALSE 0
#define MAX 134217728
#define DOUBLE_WORD 8
#define MEMSTRUCT sizeof(struct mem_region)

struct pcb {
    char* proc_name;
    unsigned int pid;
};

struct mem_region {
    int free : 1;
    unsigned int size : 31;
    unsigned int pid;
    uint8_t data[0];
};

int round_size(int size);

unsigned int bounds(void* ptr);

struct mem_region* init_struct(struct mem_region* first);

struct mem_region* subdivide(struct mem_region* mem, int size);

struct mem_region* first_fit(struct mem_region* temp, int size);

void compact_prev(struct mem_region* prev, int size);

void compact_next(struct mem_region* prev);

int free_match(struct mem_region* temp, void* ptr);

int empty_mem_check(struct mem_region* temp);

void memoryMap(struct mem_region* first);

void* insert_at_tail(struct mem_region* first, int size);

void *myMalloc(unsigned int size);

void myFree(void *ptr);

int myFreeErrorCode(void *ptr);

struct mem_region* walk_struct(struct mem_region* this_region);

//TODO add func declarations

#endif