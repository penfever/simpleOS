#ifndef _MYMALLOC_H
#define _MYMALLOC_H
#define TRUE 1
#define FALSE 0
#define MEGA_BYTE 1048576
#define MAX (128 * 1048576)
#define MEMSTRUCT sizeof(struct mem_region)

struct mem_region {
    int free : 1;
    unsigned int size : 31;
    unsigned int pid;
    uint8_t data[0];
};

void *myMalloc(unsigned int size);

void *myMalloc_allocate(struct mem_region *first, int size);

void myFree(void *ptr);

int myFreeErrorCode(void *ptr);

struct mem_region* walk_struct(struct mem_region* this_region);

#endif