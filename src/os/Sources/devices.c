/*
 * devices.c
 *
 *  Created on: Mar 11, 2021
 *      Author: penfe
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "devices.h"
#include "uart.h"
#include "mcg.h"
#include "sdram.h"

struct pcb* currentPCB;
struct pcb op_sys;
struct pcb op_sys = 
{
		"OS", //NAME
		0, //PID
		{ //NULL INIT STRUCT "STREAM" ARRAY
				{0},
				{0},
				{0},
				{0},
				{0},
				{0},
				{0},
				{0}
		}
};

/* Note below that the counterRegister field is declared to be a pointer
 * to an unsigned 16-bit value that is the counter register for the
 * appropriate channel of TSI0.  Further note that TSI0_CNTR5, for
 * example, is a 32-bit register that contains two 16-bit counters --
 * one for channel 4 in the low half and one for channel 5 in the high
 * half.  So, once &TSI0_CNTR5 is cast to be a pointer to a 16-bit
 * unsigned int, it is then a pointer to the 16-bit counter for
 * channel 4 of TSI0.  That pointer then needs to be incremented by
 * one so that it points to the 16-bit counter for channel 5 of TSI0.
 * This same technique is used for all the other counterRegister
 * field values as well. */
struct electrodeHW {
	int channel;
	uint32_t mask;
	uint16_t threshold;
	uint16_t *counterRegister;
} electrodeHW[ELECTRODE_COUNT] =
	{{5, TSI_PEN_PEN5_MASK, 0, (uint16_t *)&TSI0_CNTR5+1},	/* E1 */
	 {8, TSI_PEN_PEN8_MASK, 0, (uint16_t *)&TSI0_CNTR9},	/* E2 */
	 {7, TSI_PEN_PEN7_MASK, 0, (uint16_t *)&TSI0_CNTR7+1},	/* E3 */
	 {9, TSI_PEN_PEN9_MASK, 0, (uint16_t *)&TSI0_CNTR9+1}};	/* E4 */

dev_id_t devTable[DEV] = {
	{dev_null, "dev_null"},
	{dev_sdhc, "dev_sdhc"},
	{dev_sw1, "dev_sw1"},
	{dev_sw2, "dev_sw2"},
	{dev_E1, "dev_E1"},
	{dev_E2, "dev_E2"},
	{dev_E3, "dev_E3"},
	{dev_E4, "dev_E4"},
	{dev_UART2, "dev_UART2"},
	{dev_pot, "dev_pot"},
	{dev_temp, "dev_temp"},
	{dev_TSI1, "dev_TSI1"},
	{dev_TSI2, "dev_TSI2"},
	{dev_TSI3, "dev_TSI3"},
	{dev_TSI4, "dev_TSI4"}
};

int uart_init_noMCG(int baud){
	const int IRC = 32000;					/* Internal Reference Clock */
	const int FLL_Factor = 640;
	const int moduleClock = FLL_Factor*IRC;
	const int KHzInHz = 1000;
	uartInit(UART2_BASE_PTR, moduleClock/KHzInHz, baud);
	return 0;
}

int uart_init(int baud){
	const uint32_t moduleClock = 60000000; /*in hz. Per Table 5-2 on labeled page 225 (PDF page 232) of the K70 Sub-Family
	 * Reference Manual, Rev. 4, Oct 2015 */
	const int KHzInHz = 1000;
	uartInit(UART2_BASE_PTR, moduleClock/KHzInHz, baud);
	return 0;
}


int init_clocks_sdram(){
	mcgInit();
	  /* So now,
	   *  Core clock = 120 MHz
	   *  Bus (peripheral) clock = 60 MHz
	   *  FlexBus clock = 40 MHz
	   *  FLASH clock = 20 MHz
	   *  DDR clock = 150 MHz
	   *  MCGIRCLK (internal reference clock) is inactive */
	sdramInit();
	return 0;
}

void adc_init(void) {
   SIM_SCGC3 |= SIM_SCGC3_ADC1_MASK;
   ADC1_CFG1 = ADC_CFG1_MODE(ADC_CFG1_MODE_12_13_BIT);
   ADC1_SC3 = ADC_SC3_AVGE_MASK | ADC_SC3_AVGS(ADC_SC3_AVGS_32_SAMPLES);
}

unsigned int adc_read(uint8_t channel) {
   ADC1_SC1A = channel;
   while(!(ADC1_SC1A & ADC_SC1_COCO_MASK)) {
   }
   return ADC1_RA;
}

/* Initialize the capacitive touch sensors */
void TSI_Init(void) {
    SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK | SIM_SCGC5_TSI_MASK;
    									// Turn on the clock to ports A & B and
    									//		to the TSI module
    PORTA_PCR4 = PORT_PCR_MUX(0);		// Enable port A, bit 4 as TSI0_CH5
    PORTB_PCR3 = PORT_PCR_MUX(0);		// Enable port B, bit 3 as TSI0_CH8
    PORTB_PCR2 = PORT_PCR_MUX(0);		// Enable port B, bit 2 as TSI0_CH7
    PORTB_PCR16 = PORT_PCR_MUX(0);		// Enable port B, bit 16 as TSI0_CH9

    TSI0_GENCS |= TSI_GENCS_EOSF_MASK;	// Clear the EOSF (End of Scan) flag

    TSI0_GENCS |= TSI_GENCS_NSCN(10) |	// Set number of consecutive scans per electrode to 11
    		TSI_GENCS_PS(4) |			// Set electrode oscillator prescaler to divide by 16
    		TSI_GENCS_STPE_MASK |		// Keep TSI running when MCU goes into low power mode
    		TSI_GENCS_LPSCNITV(7);		// Low power mode scan interval is set to 50 msec
    TSI0_SCANC |= (TSI_SCANC_EXTCHRG(8) |	// Set ext oscillator charge current to 18 uA
    		TSI_SCANC_REFCHRG(15) |		// Set reference osc charge current to 32 uA
    		TSI_SCANC_SMOD(10) |		// Set scan period modulus to 10
    		TSI_SCANC_AMPSC(1) |		// Divide input clock by 2
    		TSI_SCANC_AMCLKS(0));		// Set active mode clock source to LPOSCCLK

    //TSI0_GENCS |= TSI_GENCS_LPCLKS_MASK; // Set low power clock source to VLPOSCCLK

/* Electrode E1 is aligned with the orange LED */
#define Electrode_E1_EN_MASK TSI_PEN_PEN5_MASK
/* Electrode E2 is aligned with the yellow LED */
#define Electrode_E2_EN_MASK TSI_PEN_PEN8_MASK
/* Electrode E3 is aligned with the green LED */
#define Electrode_E3_EN_MASK TSI_PEN_PEN7_MASK
/* Electrode E4 is aligned with the blue LED */
#define Electrode_E4_EN_MASK TSI_PEN_PEN9_MASK

    TSI0_PEN = Electrode_E1_EN_MASK | Electrode_E2_EN_MASK |
    		Electrode_E3_EN_MASK | Electrode_E4_EN_MASK;
    
    /* In low power mode only one pin may be active; this selects electrode E4 */
    TSI0_PEN |= TSI_PEN_LPSP(9);
    TSI0_GENCS |= TSI_GENCS_TSIEN_MASK;  // Enables TSI
}

/* Calibrate the capacitive touch sensors */
void TSI_Calibrate(void) {
	int i;
	uint16_t baseline;
	
	TSI0_GENCS |= TSI_GENCS_SWTS_MASK;	/* Software Trigger Start */
	while(!(TSI0_GENCS & TSI_GENCS_EOSF_MASK)) {
	}
    TSI0_GENCS |= TSI_GENCS_EOSF_MASK;	// Clear the EOSF (End of Scan) flag

    for(i = 0; i < ELECTRODE_COUNT; i++) {
        baseline = *(electrodeHW[i].counterRegister);
        electrodeHW[i].threshold = baseline + THRESHOLD_OFFSET;
    }
}
	
int electrode_in(int electrodeNumber) {
	uint16_t oscCount;
	
	TSI0_GENCS |= TSI_GENCS_SWTS_MASK;	/* Software Trigger Start */
	while(!(TSI0_GENCS & TSI_GENCS_EOSF_MASK)) {
	}
    TSI0_GENCS |= TSI_GENCS_EOSF_MASK;	// Clear the EOSF (End of Scan) flag

    oscCount = *(electrodeHW[electrodeNumber].counterRegister);

    /* Returns 1 when pushbutton is depressed and 0 otherwise */
	
	return oscCount > electrodeHW[electrodeNumber].threshold;
}


