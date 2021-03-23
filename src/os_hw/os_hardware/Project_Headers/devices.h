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
#define DEV sizeof(devTable)/sizeof(devTable[0])
#define PUSHB_MIN 0x00FF0000
#define PUSHB_MAX 0x00FF0001
#define LED_MIN 0x00EF0000
#define LED_MAX 0x00EF0003

extern struct pcb* currentPCB;
extern struct pcb op_sys;
extern char* devTable;

struct stream { //Abstraction: what device is this, and how do I talk to it?
	enum device_type{ // Abstraction: Major IDs
		UNUSED = 0,
		FAT32 = 1,
		PUSHBUTTON = 2,
		LED = 3
	}deviceType; //Major ID
	enum minor_id{
		dev_null = 0,
		sdhc = 0x99FF0000,
		dev_sw1 = 0x00FF0000,
		dev_sw2 = 0x00FF0001,
		dev_E1 = 0x00EF0000,
		dev_E2 = 0x00EF0001,
		dev_E3 = 0x00EF0002,
		dev_E4 = 0x00EF0003
	}minorId;
	int mode;  //rw mode	
	//FAT32 specific entries
	uint32_t cursor; // current position in file (shared read/write)
	uint32_t clusterAddr;
	uint8_t *fileName;
	uint32_t fileSize;
	//LED-specific entries
	enum led_color
	{
		red = 1,
		yellow = 2,
		green = 3,
		blue = 4
	}ledcolor;
};

struct pcb {
    char* proc_name;
    uint8_t pid;
    struct stream openFiles[MAXOPEN];
};

int uart_init(int baud);

#endif
