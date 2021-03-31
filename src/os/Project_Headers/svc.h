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

void svcInit_SetSVCPriority(unsigned char priority);
void svcHandler(void);

void SVCEndive(void);
void SVCBroccoliRabe(int arg0);
int SVCJicama(int arg0);
int SVCArtichoke(int arg0, int arg1, int arg2, int arg3);
int SVC_fopen(file_descriptor* descr, char* filename, char mode);

#endif /* ifndef _SVC_H */
