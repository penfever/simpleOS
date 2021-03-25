/*
 * devices.c
 *
 *  Created on: Mar 11, 2021
 *      Author: penfe
 */
#include "devices.h"
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

char* devTable = {
	"dev_null",
	"dev_sw1",
	"dev_sw2",
	"dev_E1",
	"dev_E2",
	"dev_E3",
	"dev_E4"
};
