/*
 * devices.c
 *
 *  Created on: Mar 11, 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "devices.h"
#include "uart.h"
#include "mcg.h"
#include "sdram.h"
#include "svc.h"
#include "intSerialIO.h"
#include "myerror.h"
#include "simpleshell.h"
#include "procs.h"
#include "mymalloc.h"
#include "priv.h"
#include "lcdc.h"
#include "lcdcConsole.h"
#include "profont.h"
#include "dac12bit.h"

uint32_t g_firstrun_flag = 0;
uint32_t g_pause_counter = 0;

/*TWR-LCD-RGB display console*/
struct console console;

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

/*This is a list of all devices available to the operating system.*/
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
	{dev_TSI4, "dev_TSI4"},
	{dev_DAC0, "dev_DAC0"},
	{dev_DAC1, "dev_DAC1"}
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
	intSerialIOInit();
	return 0;
}

/*init_sys initializes all necessary devices for the system to run.*/
int init_sys(){
	int error = 0;
	/*SDRAM and full clock speed*/
		/* after this,
	*  Core clock = 120 MHz
	*  Bus (peripheral) clock = 60 MHz
	*  FlexBus clock = 40 MHz
	*  FLASH clock = 20 MHz
	*  DDR clock = 150 MHz
	*  MCGIRCLK (internal reference clock) is inactive */
	mcgInit();
	sdramInit();
	/*File IO and console IO*/
	if (CONSOLEIO || MYFAT_DEBUG || MYFAT_DEBUG_LITE){
        setvbuf(stdin, NULL, _IONBF, 0); //fix for consoleIO stdin and stdout
        setvbuf(stdout, NULL, _IONBF, 0);	
    }
    if ((error = file_structure_mount()) != 0 && MYFAT_DEBUG){
    	printf("SDHC card could not be mounted. File commands unavailable. \n");
    }
    else if (MYFAT_DEBUG){
    	printf("SDHC card mounted. \n");
        g_noFS = FALSE;
    }
    else{
        g_noFS = FALSE;
    }
	/*SVC, pendSV interrupt priority, pid*/
	svcInit_SetSVCPriority(15);
	pendSVInit_SetpendSVPriority(14);
	pid_t shellPid;
	/*TWR-LCD-RGB console init*/
	lcdcInit();
	lcdcConsoleInit(&console);
	/*launch shell*/
	struct spawnData mySpawnData = {"cmd_shell", NEWPROC_DEF, &shellPid};
	error = spawn(cmd_shell, 0, NULL, &mySpawnData);
	/*scheduler*/
	systick_init();
    privUnprivileged();
	return error;
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

/*Systick interrupt handlers and routines*/

void SysTickHandler(void){
  uint32_t copyOfSP;

  copyOfSP = 0;

  /* The following assembly language will push registers r4 through r11 onto the stack */
  __asm("push {r4,r5,r6,r7,r8,r9,r10,r11}");

  /* The following assembly language will push the SVCALLACT and
   * SVCALLPENDED bits onto the stack (See Application Note AN6:
   * Interrupt handlers in general and the quantum interrupt handler
   * in particular) */
  __asm("ldr  r0, [%[shcsr]]"     "\n"
	"mov  r1, %[active]"      "\n"
	"orr  r1, r1, %[pended]"  "\n"
	"and  r0, r0, r1"         "\n"
	"push {r0}"
	:
	: [shcsr] "r" (&SCB_SHCSR),
	  [active] "I" (SCB_SHCSR_SVCALLACT_MASK),
	  [pended] "I" (SCB_SHCSR_SVCALLPENDED_MASK)
	: "r0", "r1", "memory", "sp");

  /* The following assembly language will put the current main SP
   * value into the local, automatic variable 'copyOfSP' */
  __asm("mrs %[mspDest],msp" : [mspDest]"=r"(copyOfSP));

  if (MYFAT_DEBUG){
	  printf("The current value of MSP is %08x\n", (unsigned int)copyOfSP);
  }

  /* Call the scheduler to find the saved SP of the next process to be
   * executed. Scheduler must return a new SP, which is the SP of the process to be executed (whose process stack will be pre-formatted to look like the previous stack)*/
  copyOfSP = rr_sched(copyOfSP);

  /* The following assembly language will write the value of the
   * local, automatic variable 'copyOfSP' into the main SP */
  __asm("msr msp,%[mspSource]" : : [mspSource]"r"(copyOfSP) : "sp");

  /* The following assembly language will pop the SVCALLACT and
   * SVCALLPENDED bits off the stack (and into their registers)*/
  __asm("pop {r0}"               "\n"
	"ldr r1, [%[shcsr]]"     "\n"
	"bic r1, r1, %[active]"  "\n"
	"bic r1, r1, %[pended]"  "\n"
	"orr r0, r0, r1"         "\n"
	"str r0, [%[shcsr]]"
	:
	: [shcsr] "r" (&SCB_SHCSR),
	  [active] "I" (SCB_SHCSR_SVCALLACT_MASK),
	  [pended] "I" (SCB_SHCSR_SVCALLPENDED_MASK)
	: "r0", "r1", "sp", "memory");

  /* The following assembly language will pop registers r4 through
   * r11 off of the stack (and into their registers)*/
  __asm("pop {r4,r5,r6,r7,r8,r9,r10,r11}");

  return;
}
    //call scheduler
//     int countflag_test = (SYST_CSR & ~SysTick_CSR_COUNTFLAG_MASK) >> SysTick_CSR_COUNTFLAG_SHIFT;
//     if (test1 != 0){
//    	 //the systick was interrupted, handle accordingly
//     }
void systick_init(void){
	SCB_SHPR3 = (SCB_SHPR3 & ~SCB_SHPR3_PRI_15_MASK) |
			SCB_SHPR3_PRI_15(QUANTUM_INTERRUPT_PRIORITY << SVC_PriorityShift);

    SYST_RVR = QUANTUM;
    SYST_CVR = 0;
    SYST_CSR |= CSRINIT;
    return;
}

/*Toggles systick resume. Note: atomic operation, because g_pause_counter is a semaphore.*
Assign correct value with = operators. */
void systick_resume(void){
	disable_interrupts();
	g_pause_counter --;
	if (g_pause_counter > 0){
	    SYST_CSR ^ SysTick_CSR_ENABLE_MASK; //TODO: fix this so it does not read CSR
	}
	enable_interrupts();
}

/*Toggles systick pause. Note: atomic operation, because g_pause_counter is a semaphore.*/
void systick_pause(void){
	disable_interrupts();
	g_pause_counter ++;
	if (g_pause_counter == 1){
	    SYST_CSR ^ SysTick_CSR_ENABLE_MASK; //TODO: fix this so it does not read CSR
	}
	enable_interrupts();
}

/**/
void dacInit(){
	dac0_1_clk_enable();  //enabled system clock to DAC module.
	DACx_reset_dac0_1_reg_values();//reset DAC0 and DAC1 value to default reset value;
}
/**/
