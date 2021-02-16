#ifndef _MYMALLOC_H
#define _MYMALLOC_H
#define TRUE 1
#define FALSE 0
#define MEGA_BYTE 1048576
#define MAX (128 * 1048576) - 1

void *myMalloc(unsigned int size);

void myFree(void *ptr);

int myFreeErrorCode(void *ptr);

#endif