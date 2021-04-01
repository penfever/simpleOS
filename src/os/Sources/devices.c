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
	{dev_temp, "dev_temp"}
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

#define ADC_CHANNEL_POTENTIOMETER   	0x14
#define ADC_CHANNEL_TEMPERATURE_SENSOR  0x1A

#define ADC_CFG1_MODE_8_9_BIT       0x0
#define ADC_CFG1_MODE_12_13_BIT     0x1
#define ADC_CFG1_MODE_10_11_BIT     0x2
#define ADC_CFG1_MODE_16_BIT        0x3
#define ADC_SC3_AVGS_32_SAMPLES     0x3

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

//int main(void) {
//   adc_init();
//   
//   while(TRUE) { 
//      printf("pot: %4u\ttemp: %4u\n",
//    		 adc_read(ADC_CHANNEL_POTENTIOMETER), 
//    		 adc_read(ADC_CHANNEL_TEMPERATURE_SENSOR));
//   }
//
//   return 0;
//}
