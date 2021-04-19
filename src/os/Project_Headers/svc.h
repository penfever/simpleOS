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

#ifndef _SVC_H
#define _SVC_H

#define SVC_MaxPriority 15
#define SVC_PriorityShift 4

// Implemented SVC numbers
#define SVC_ENDIVE 0
#define SVC_BROCCOLIRABE 1
#define SVC_JICAMA 2
#define SVC_ARTICHOKE 3
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
#define SVC_ISCHAR 14
#define SVC_DIR_LS 15
#define SVC_SETTIME 16
#define SVC_PDBONESHOT 17

void svcInit_SetSVCPriority(unsigned char priority);
void svcHandler(void);

void SVCEndive(void);
void SVCBroccoliRabe(int arg0);
int SVCJicama(int arg0);
int SVCArtichoke(int arg0, int arg1, int arg2, int arg3);
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

#endif /* ifndef _SVC_H */
