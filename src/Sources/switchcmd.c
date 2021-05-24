/**
 * switchcmd.c
 * switch function code to query and handle user action
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021, 2017, 2015, 2014, 2012 James L. Frankel.  All rights reserved.
 */

#include "switchcmd.h"
#include "pushbutton.h"

unsigned char gbl_key_state = 0;


/**
 * busy-check service function.  detects a transition from unpressed
 * to pressed and vice versa.
 */
enum switchState switchScan(void) {
    /* Check Switch 1 */
    if(sw1In()) {				/* Switch is pressed */
        /* Look for switch transition */
        if(!(gbl_key_state & SW1_ACTIVE)) {
            /* Switch has changed to now be pressed */
            gbl_key_state |= SW1_ACTIVE;	/* Update switch state */
            return switch1Down;
        }
    } else {				/* Switch is not pressed */
        /* Look for switch transition */
        if(gbl_key_state & SW1_ACTIVE) {
            /* Switch has changed to now not be pressed */
            gbl_key_state &= ~SW1_ACTIVE;	/* Update switch state */
            return switch1Up;
        }
    }

    /* Check Switch 2 */
    if(sw2In()) {				/* Switch is pressed */
        /* Look for switch transition */
        if(!(gbl_key_state & SW2_ACTIVE)) {
            /* Switch has changed to now be pressed */
            gbl_key_state |= SW2_ACTIVE;	/* Update switch state */
            return switch2Down;
        }
    } else {				/* Switch is not pressed */
        /* Look for switch transition */
        if(gbl_key_state & SW2_ACTIVE) {
            /* Switch has changed to now not be pressed */
            gbl_key_state &= ~SW2_ACTIVE;	/* Update switch state */
            return switch2Up;
        }
    }

    return noChange;
}


/**
 * Indicate that the pins used for the switches are inputs.
 */
void switchcmdInit(void) {
    pushbuttonInitAll();
}
