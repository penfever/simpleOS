/*
 * breakpoint.h
 * NXP/Freescale K70 ARM Breakpoint Header File
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021 James L. Frankel.  All rights reserved.
 */

#ifndef _BREAKPOINT_H
#define _BREAKPOINT_H

#ifndef __BKPT
#define __BKPT() __asm__ __volatile__ ("bkpt #0")
#endif

#endif /* ifndef _BREAKPOINT_H */
