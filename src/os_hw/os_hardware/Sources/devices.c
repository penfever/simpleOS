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

char *devTable[7] = {
	"dev_null",
	"dev_sw1",
	"dev_sw2",
	"dev_E1",
	"dev_E2",
	"dev_E3",
	"dev_E4"
};

int uart_init(int baud){
	const int IRC = 32000;					/* Internal Reference Clock */
	const int FLL_Factor = 640;
	const int moduleClock = FLL_Factor*IRC;
	const int KHzInHz = 1000;
	uartInit(UART2_BASE_PTR, moduleClock/KHzInHz, baud);
	return 0;
}
