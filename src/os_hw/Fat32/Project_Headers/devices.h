/*
 * devices.h
 *
 *  Created on: Mar 11, 2021
 *      Author: penfe
 */

#ifndef DEVICES_H_
#define DEVICES_H_

#define MAXOPEN 32

extern struct pcb* currentPCB;
extern struct pcb op_sys;

struct pcb {
    char* proc_name;
    uint8_t pid;
    struct stream openFiles[MAXOPEN];
};

struct stream { //Abstraction: what device is this, and how do I talk to it?
	enum device_type deviceType; //Major ID
	int minorId;
	int mode;  //rw mode
	uint32_t cursor; // current position in file
	uint8_t pid; //who has this open
	uint8_t open; //is file open
	//FAT32 specific entries
	uint32_t clusterAddr;
	uint8_t fileName[11];
	uint32_t fileSize;
};

// Abstraction: Major IDs

enum device_type
{
	FAT32 = 1,
	PUSHBUTTON = 2,
	LED = 3
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

#endif DEVICES_H_
