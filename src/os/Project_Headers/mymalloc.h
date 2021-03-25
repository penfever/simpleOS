#ifndef _MYMALLOC_H
#define _MYMALLOC_H
#define TRUE 1
#define FALSE 0
#define MAX 48000
#define SHELL_RESERVE 512
#define DOUBLE_WORD 8
#define MEMSTRUCT sizeof(struct mem_region)

extern struct mem_region* first;

struct mem_region {
    int free : 1;
    unsigned int size : 31;
    unsigned int pid;
    uint8_t data[0];
};

int round_size(int size);

uint8_t getCurrentPid();

struct mem_region* init_struct(struct mem_region* first);

void memoryMap(void);

uint32_t bounds(void* ptr);

void *myMalloc(unsigned int size);

void myFree(void *ptr);

int myFreeErrorCode(void *ptr);

int free_match(struct mem_region* temp, void* ptr);

struct mem_region* walk_struct(struct mem_region* this_region);

void walk_struct_err();

#endif
