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

/*This supervisor call is for opening files and devices.*/
int SVC_fopen(file_descriptor* descr, char* filename, char mode);
/*This supervisor call is for closing files and devices.*/
int SVC_fclose(file_descriptor descrf);
/*This supervisor call is for creating files.*/
int SVC_create(char* filename);
/*This supervisor call is for deleting files.*/
int SVC_delete(char* filename);
/*This supervisor call gets character-length input from devices and files.*/
int SVC_fgetc (file_descriptor descrf, char* bufp);
/*This supervisor call gets string-length input from devices and files.*/
int SVC_fgets (file_descriptor descrf, char* bufp, int buflen);
/*This supervisor call sends character-length input to devices and files.*/
int SVC_fputc (file_descriptor descrf, char bufp);
/*This supervisor call sends string-length input to devices and files.*/
int SVC_fputs (file_descriptor descrf, char* bufp, int buflen);
/*This supervisor call allocates memory to a process.*/
void* SVC_malloc(unsigned int size);
/*This supervisor call frees memory previously allocated to a process.*/
int SVC_free(void *ptr);
/*This supervisor call prints the contents of a directory.*/
int SVC_dir_ls(int full);
/*This supervisor call sets the current time.*/
int SVC_settime(unsigned long long *newTime);
/*This supervisor call sets or resets the one-shot PDB timer.*/
int SVC_pdb0oneshottimer(uint16_t* delayCount);
/*This supervisor call spawns a new process.*/
int SVC_spawn(int main(int argc, char *argv[]), int argc, char *argv[], struct spawnData* thisSpawn);
/*This supervisor call allows a process to yield its remaining quantum.*/
void SVC_yield(void);
/*This supervisor call blocks a process.*/
void SVC_block(void);
/*This supervisor call wakes a blocked process.*/
int SVC_wake(pid_t targetPid);
/*This supervisor call instructs a process to destroy itself.*/
int SVC_kill(pid_t targetPid);
/*This supervisor call allows a process to wait until another process completes.*/
void SVC_wait(pid_t targetPid);

/*Disables all interrupts to the ARM processor.*/
void disable_interrupts(void);
/*Enables all interrupts to the ARM processor.*/
void enable_interrupts(void);

#endif /* ifndef _SVC_H */
