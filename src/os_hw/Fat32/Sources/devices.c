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
				{0}
		}
};
