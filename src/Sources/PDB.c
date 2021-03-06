/**
 * PDB.c
 * interrupt handler and hardware Programmable Delay Block (PDB) init
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021, 2017, 2015, 2014, 2012 James L. Frankel.  All rights reserved.
 *
 * Last revised 1:51 PM 22-Mar-2021
 */

#include <stdint.h>
#include "derivative.h"
#include "nvic.h"
#include "PDB.h"
#include "mcg.h"
#include "simpleshell.h"
#include "myerror.h"
#include "devices.h"

/* For an overall description of the Programmable Delay Block (PDB), see
 * Chapter 43 on page 1193 of the K70 Sub-Family Reference Manual, Rev. 2,
 * Dec 2011) */

/* Table 5-2 on page 220 indicates that the clock used by the PDB is
 * always the Bus clock.  Before calling mcgInit, the processor is
 * running in FEI (FLL Engaged Internal) mode and with default settings
 * (DRST_DRS = 00, DMX32 = 0), the MCGOUTCLK (MCG (Multipurpose Clock
 * Generator) clock) and Bus (peripheral) clock are set to 640*32768 Hz
 * [See K70 Sub-Family Reference Manual, Rev. 2, Section 25.4.1.1,
 * Table 25-22 on page 657 and MCG Control 4 Register (MCG_C4) Section
 * 25.3.4 on page 641].  After calling mcgInit, the Bus (peripheral)
 * clock is set to 60 MHz. */

/* Note: mcgInit sets all of the fields in SIM_CLKDIV1 including
 * SIM_CLKDIV1_OUTDIV2 -- the Clock 2 (peripheral clock) output
 * divider. */

/**
 * initialize PDB 0 hardware
 *
 * count is the number of 1/16384 second time periods
 * continuous is a boolean value which determines if the counter will work in one-shot or
 *   		  continuous mode
 */
void PDB0Init(uint16_t count, int continuous) {
    /* Enable the clock for PDB0 (PDBTimer 0) using the SIM_SCGC6 register
     * (System Clock Gating Control Register 6) (See 12.2.14 on page 344 of
     * the K70 Sub-Family Reference Manual, Rev. 2, Dec 2011) */
    SIM_SCGC6 |= SIM_SCGC6_PDB_MASK;

    /* Disable PDBTimer 0 and clear any pending PDB Interrupt Flag using
     * the PDB0_SC register (Status and Control register for PDBTimer 0)
     * (See 43.3.1 on page 1199 of the K70 Sub-Family Reference Manual,
     * Rev. 2, Dec 2011) */
    PDB0_SC = 0;

    /* After MCGInit, we end up with a peripheral clock of
     * 60,000,000 Hz.
     Prescaler 128 = 468,750
      */

    /* Load timer count (16-bit value) into the modulo register */
    PDB0_MOD = count;

    /* Load timer count (16-bit value) into the interrupt delay register */
    PDB0_IDLY = count;

    /* Prescaler to divide by 128, Software trigger is selected,
     * PDB interrupt enabled, Multiplication factor is 1,
     * Continuous mode, Load OK, Enable the PDB */
    PDB0_SC = PDB_SC_PRESCALER(PDB_SC_PRESCALER_DIVIDE_BY_128) |
              PDB_SC_TRGSEL(PDB_SC_TRGSEL_SOFTWARE_TRIGGER) |
              PDB_SC_PDBIE_MASK |
              PDB_SC_MULT(PDB_SC_MULT_BY_20) |
              (continuous ? PDB_SC_CONT_MASK : 0) |
              PDB_SC_LDOK_MASK | PDB_SC_PDBEN_MASK;

    /* Enable interrupts from PDB0 and set its interrupt priority */
    NVICEnableIRQ(PDB0_IRQ_NUMBER, PDB0_INTERRUPT_PRIORITY);
}

/**
 * start PDB 0
 */
void PDB0Start(void) {
    /* Enable the PDB */
    PDB0_SC |= PDB_SC_PDBEN_MASK;

    /* With the PDB already enabled, set the PDB Software Trigger */
    PDB0_SC |= PDB_SC_SWTRIG_MASK;
}

/**
 * stop PDB 0
 */
void PDB0Stop(void) {
    /* Disable the PDB */
    PDB0_SC &= ~PDB_SC_PDBEN_MASK;
}

/**
 * PDB 0 Interrupt Service Routine (ISR)
 */
void PDB0Isr(void) {
    systick_pause();
    /* Clear the Interrupt Flag */
    PDB0_SC &= ~PDB_SC_PDBIF_MASK;

    /* Perform the user's action */
    PDB0Stop();
    g_timerExpired = TRUE; //timerExpired is now true
    systick_resume();
}

/*pdb0 one shot timer sets the pdb timer to a one-shot value determined by delayCount (ranging ~7.8ms to 1000ms)
This function assumes count is in 1/16384 intervals.*/
int pdb0_one_shot_timer(uint16_t* delayCount) {
    uint16_t count = *delayCount;
    count += 1;
    count *= PDB0_FACTOR;
    if (count < 1) {
        return E_NOINPUT; //check overflow
    }
    PDB0Init(count, FALSE);
    PDB0Start(); //timer starts
    g_timerExpired = FALSE; //timerExpired is now false
    return 0;
}
