/**
 * switchcmd.h
 * switch definitions and functions to permit user control 
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021, 2017, 2015, 2014, 2012 James L. Frankel.  All rights reserved.
 */

#ifndef _SWITCHCMD_H
#define _SWITCHCMD_H

/* Application definitions for status word */
#define SW1_ACTIVE	0x1
#define SW2_ACTIVE	0x2

/* Status word for all pushbuttons */
extern unsigned char gbl_key_state;

/* Application definitions for command interpreting function */
enum switchState {noChange,
		  switch1Down, switch1Up,
		  switch2Down, switch2Up};

/* Function prototypes */

void switchcmdInit(void);

/* Frequently run polling function to react to user action */
enum switchState switchScan(void);

#endif /* ifndef _SWITCHCMD_H */
