/*
 * univio.c
 *
 *  Created on: Mar 11, 2021
 *      Author: penfe
 */
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "univio.h"
#include "devices.h"
#include "SDHC_FAT32_Files.h"

static int pid = 0; //temporarily set everything to OS

/*Devices to support: pushbuttons, LEDs, FAT32*/

/*fopen returns 0 in case of error. Enable MYFAT_DEBUG for more information. */

file_descriptor fopen (char* filename, char mode){
	if (pid != currentPCB->pid){
		return E_NOINPUT; //TODO: error checking
	}
	int isDevice = FALSE;
	int err;
	char* device = "_";
	if (strncmp("dev_", filename, 4) != 0){ //if filename starts with dev_
		continue;
	}
	else{
		file_descriptor fd = check_dev_table(filename);
		if (fd == 0){
			if (MYFAT_DEBUG){
				printf("Invalid device name \n");
			}
		}
		//put fd into the PCB
		err = add_device_to_PCB(fd);
		if (err != 0){
			return 0;
		}
		return fd;
	}
	 //treat as FAT32
	//if mode is read, call file_open with a null descriptor, print error code if appropriate, return descriptor
	file_descriptor descr;
	err = file_open(filename, descr);
	if (err != 0){
		if (MYFAT_DEBUG){
			printf("File_open error %d \n", err);
		}
		return 0;
	}
	struct stream* userptr = (struct stream*)descr;
	if (find_curr_stream(userptr) == FALSE){
		descr = 0;
	}
	userptr->mode = mode;
	return descr;
}

int add_device_to_PCB(file_descriptor fd){
	struct stream* userptr = find_open_stream();
	if (userptr == NULL){
		if (MYFAT_DEBUG){
			printf("You have too many file streams open \n");
		}
		return E_NOINPUT;
	}
    //TODO: Do I need a dynamic array of which files are open? To prevent double opening? this is a PSET4 issue, right now we only have one proc open
	//populate struct in PCB with file data
	if (fd >= PUSHB_MIN && fd <= PUSHB_MAX){
		userptr->majorId = PUSHBUTTON;
		pushbuttonInitAll();
	}
	if (fd >= LED_MIN && fd <= LED_MAX){
		userptr->majorId = LED;
		ledInitAll();
	}
	else{
		if (MYFAT_DEBUG){
			printf("Invalid device name \n");
		}
		return E_NOINPUT;
	}
	userptr->minorId = fd; //This should macro to the correct minorId
	return 0;
}

file_descriptor check_dev_table(char* filename){
	for (int i = 0; i < DEV; ++i){
		if (strncmp(devTable[i], filename, strlen(filename)) != 0){
			continue;
		}
		else{
			if (i == 1){
				return sw1;
			}
			if (i == 2){
				return sw2;
			}
			if (i == 3){
				return E1;
			}
			if (i == 4){
				return E2;
			}
			if (i == 5){
				return E3;
			}
			if (i == 6){
				return E4;
			}
		}
	}
	return 0;
}

int fclose (char* filename){
	if (pid != currentPCB->pid){
		return E_NOINPUT; //TODO: error checking
	}
	int isDevice = FALSE;
	int err;
	char* device = "_";
	if (strncmp("dev_", filename, 4) != 0){ //if filename starts with dev_
		continue;
	}
	else{
		file_descriptor fd = check_dev_table(filename);
		if (fd == 0){
			if (MYFAT_DEBUG){
				printf("Invalid device name \n");
			}
		}
		//put fd into the PCB
		err = remove_device_from_PCB(fd);
		if (err != 0){
			if (MYFAT_DEBUG){
				printf("Remove from PCB error %d \n", err);
			}
			return 0;
		}
		return fd;
	}
	//treat as FAT32
	file_descriptor descr;
	err = file_close(filename, descr);
	if (err != 0){
		return err;
	}
	return 0;
}

int remove_device_from_PCB(file_descriptor fd){
	struct stream* userptr = (struct stream*)fd;
	if (find_curr_stream(userptr) == FALSE){
		return E_NOINPUT;
	}
	userptr->majorId = UNUSED;
	userptr->minorId = dev_null;
	return 0;
}
/* fgetc() reads the next character from stream and 
 * returns it as an unsigned char cast to an int, or 
 * EOF on end of file or error.  */

int fgetc (file_descriptor descr){
	int err;
	if (pid != currentPCB->pid){
		return E_NOINPUT; //TODO: error checking
	}
	struct stream* userptr = (struct stream*)descr;
	if (find_curr_stream(userptr) == FALSE){
		return E_NOINPUT;
	}
	if (userptr->cursor >= userptr->fileSize){
		return E_EOF;
	}
	if (userptr->deviceType == PUSHBUTTON){
		err = pushb_fgetc(descr);
	}
	if (userptr->deviceType == LED){
		err = led_fgetc(descr);
	}
	int charsreadp;
	err = file_getbuf(descr, &bufp, 1, &charsreadp);
	return err;
}

int pushb_fgetc(file_descriptor descr){
	if (sw1In()){
		uartPutsNL(UART2_BASE_PTR, "Pushbutton sw1 is pressed \n");
	}
	else{
		uartPutsNL(UART2_BASE_PTR, "Pushbutton sw1 is not pressed \n");
	}
	if (sw2In()){
		uartPutsNL(UART2_BASE_PTR, "Pushbutton sw2 is pressed \n");
	}
	else{
		uartPutsNL(UART2_BASE_PTR, "Pushbutton sw2 is not pressed \n");
	}
	return 0;
}

int led_fgetc(file_descriptor descr){
	struct stream* userptr = (struct stream*)descr;
	if (userptr->minorId == dev_E1){
        ledOrangeOn();
	}
	if (userptr->minorId == dev_E2){
        ledBlueOn();
	}
	if (userptr->minorId == dev_E3){
        ledOrangeOn();
	}
	if (userptr->minorId == dev_E4){
        ledYellowOn();
	}
	return 0;
}

/*fgets() reads in at most one less than size characters from stream and 
 * stores them into the buffer pointed to by s. Reading stops after an 
 * EOF or a newline. If a newline is read, it is stored into the buffer. 
 * A terminating null byte (\0) is stored after the last character in the buffer. */

char* fgets (file_descriptor descr, int buflen){
	if (pid != currentPCB->pid){
		return E_NOINPUT; //TODO: error checking
	}
	struct stream* userptr = (struct stream*)descr;
	if (find_curr_stream(userptr) == FALSE || userptr->deviceType != FAT32){
		return E_NOINPUT;
	}
	if (userptr->cursor + buflen >= userptr->fileSize){
		return E_EOF;
	}
	int charsreadp;
	char bufp[buflen];
	err = file_getbuf(descr, &bufp, buflen, &charsreadp);
	bufp[buflen] = '\0';
	return bufp;
}

int fputc (char bufp, file_descriptor descr){
	int err;
	if (pid != currentPCB->pid){
		return E_NOINPUT; //TODO: error checking
	}
	struct stream* userptr = (struct stream*)descr;
	if (find_curr_stream(userptr) == FALSE){
		return E_NOINPUT;
	}
	if (userptr->deviceType == PUSHBUTTON){
		return E_NOINPUT;
	}
	if (userptr->deviceType == LED){
		err = led_fputc(descr);
	}
	err = file_putbuf(descr, &bufp, 1);
	return err;
}

int led_fputc(file_descriptor descr){
	struct stream* userptr = (struct stream*)descr;
	if (userptr->minorId == dev_E1){
        ledOrangeOff();
	}
	if (userptr->minorId == dev_E2){
        ledBlueOff();
	}
	if (userptr->minorId == dev_E3){
        ledOrangeOff();
	}
	if (userptr->minorId == dev_E4){
        ledYellowOff();
	}
	return 0;
}

int fputs (char* bufp, file_descriptor descr, int buflen){
	if (pid != currentPCB->pid){
		return E_NOINPUT; //TODO: error checking
	}
	struct stream* userptr = (struct stream*)descr;
	if (find_curr_stream(userptr) == FALSE || userptr->deviceType != FAT32){
		return E_NOINPUT;
	}
	int err;
	err = file_putbuf(descr, &bufp, buflen);
	return err;
}

int create(char* filename){
	if (pid != currentPCB->pid){
		return E_NOINPUT; //TODO: error checking
	}
	int isDevice = FALSE;
	int err;
	char* device = "_";
	if (strncmp("dev_", filename, 4) != 0){ //if filename starts with dev_
		continue;
	}
	else{
		return E_NOINPUT;
	}
	return dir_create_file(*filename);
}

int delete(char* filename){
	if (pid != currentPCB->pid){
		return E_NOINPUT; //TODO: error checking
	}
	int isDevice = FALSE;
	int err;
	char* device = "_";
	if (strncmp("dev_", filename, 4) != 0){ //if filename starts with dev_
		continue;
	}
	else{
		return E_NOINPUT;
	}
	return dir_delete_file(*filename);
}

int seek(file_descriptor descr, uint32_t pos){
	if (pid != currentPCB->pid){
		return E_NOINPUT; //TODO: error checking
	}
	struct stream* userptr = (struct stream*)descr;
	if (find_curr_stream(userptr) == FALSE || userptr->deviceType != FAT32){
		return E_NOINPUT;
	}
	return file_set_cursor(descr, pos);
}
