/**
 * svc.h
 * Routines for supervisor calls
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021, 2014 James L. Frankel.  All rights reserved.
 */

#include "SDHC_FAT32_Files.h"
#include "procs.h"

#ifndef _SVC_H
#define _SVC_H

#define SVC_MaxPriority 15
#define SVC_PriorityShift 4

// Implemented SVC numbers
#define SVC_SPAWN 1
#define SVC_YIELD 2
#define SVC_BLOCK 3
#define SVC_FOPEN 4
#define SVC_FCLOSE 5
#define SVC_CREATE 6
#define SVC_DELETE 7
#define SVC_FGETC 8
#define SVC_FPUTC 9
#define SVC_MALLOC 10
#define SVC_FREE 11
#define SVC_FPUTS 12
#define SVC_FGETS 13
#define SVC_WAKE 14
#define SVC_DIR_LS 15
#define SVC_SETTIME 16
#define SVC_PDBONESHOT 17
#define SVC_KILL 18
#define SVC_WAIT 19

void svcInit_SetSVCPriority(unsigned char priority);
void svcHandler(void);
void pendSVInit_SetpendSVPriority(unsigned char priority);

int SVC_fopen(file_descriptor* descr, char* filename, char mode);
int SVC_fclose(file_descriptor descrf);
int SVC_create(char* filename);
int SVC_delete(char* filename);
int SVC_fgetc (file_descriptor descrf, char* bufp);
int SVC_fgets (file_descriptor descrf, char* bufp, int buflen);
int SVC_fputc (file_descriptor descrf, char bufp);
int SVC_fputs (file_descriptor descrf, char* bufp, int buflen);
int SVC_ischar(file_descriptor descrf);
int SVC_ischarImpl(file_descriptor descrf);
void* SVC_malloc(unsigned int size);
int SVC_free(void *ptr);
int SVC_dir_ls(int full);
int SVC_settime(long long *newTime);
int SVC_pdb0oneshottimer(uint16_t* delayCount);
int SVC_spawn(int main(int argc, char *argv[]), int argc, char *argv[], struct spawnData* thisSpawn);
void SVC_yield(void);
void SVC_block(void);
int SVC_wake(pid_t targetPid);
int SVC_kill(pid_t targetPid);
void SVC_wait(pid_t targetPid);

void disable_interrupts(void);
void enable_interrupts(void);

#endif /* ifndef _SVC_H */
