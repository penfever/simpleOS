/*
 * devices.h
 *
 *  Created on: Mar 11, 2021
 *   
 */

#include <stdint.h>

#ifndef DEVICES_H_
#define DEVICES_H_

#define MAXOPEN 32

extern struct pcb* currentPCB;
extern struct pcb op_sys;

struct stream { //Abstraction: what device is this, and how do I talk to it?
	enum device_type{ // Abstraction: Major IDs
		UNUSED = 0,
		FAT32 = 1,
		PUSHBUTTON = 2,
		LED = 3
	}deviceType; //Major ID
	int minorId;
	int mode;  //rw mode
	uint32_t cursor; // current position in file
	//FAT32 specific entries
	uint32_t clusterAddr;
	uint8_t *fileName;
	uint32_t fileSize;
};

struct pcb {
    char* proc_name;
    uint8_t pid;
    struct stream openFiles[MAXOPEN];
};

// Abstraction: Minor IDs

enum fat32
{
	sdhc = 1
};

enum led
{
	red = 1,
	yellow = 2,
	green = 3,
	blue = 4
};

enum pushbutton
{
	sw1 = 1,
	sw2 = 2
};

#endif
