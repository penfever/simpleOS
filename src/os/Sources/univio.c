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
#include "myerror.h"
#include "mymalloc.h"
#include "simpleshell.h"
#include "SDHC_FAT32_Files.h"
#include "uart.h"
#include "uartNL.h"
#include "led.h"
#include "pushbutton.h"
#include "intSerialIO.h"
#include "PDB.h"
#include "flexTimer.h"
#include "procs.h"

//static int pid = 0; //temporarily set everything to OS
int g_tsInit = FALSE;

/*Devices to support: pushbuttons, LEDs, FAT32*/

/*myfopen takes as arguments a FILE*, a filename, and a mode. It then attempts
 * to open file filename, log it in the current PID's PCB, and return a pointer
 * handler. It returns an error code on failure.*/
int myfopen (file_descriptor* descr, char* filename, char mode){
	int len = strlen(filename);
	int err;
	/*CASE: device*/
	if (strncmp("dev_", filename, 4) != 0){
		;
	}
	else{
		uint32_t devicePtr = check_dev_table(filename);
		if (devicePtr == 0){
			if (MYFAT_DEBUG){
				printf("Invalid device name \n");
			}
			return E_DEV;
		}
		err = add_device_to_PCB(devicePtr, descr);
		return err;
	}
	/*CASE: FAT32
	if mode is read, call file_open with a null descriptor, print error code if appropriate, return descriptor*/
	if (g_noFS){
		return E_NOFS;
	}
	if (len > 12 || filename[len-4] != '.'){
		return E_NOINPUT;
	}
	//string processing to ensure FAT32 compliance
	char* fileProc = "           ";
	process_strname(fileProc, filename);
	err = file_open(fileProc, descr);
	if (err != 0){
		return err;
	}
	struct stream* userptr = (struct stream *)*descr;
	if (find_curr_stream(userptr) == FALSE){
		return E_FREE_PERM;
	}
	userptr->mode = mode;
	return 0;
}

void process_strname(char* fileProc, char* filename){
	int len = strlen(filename);
	if (len == 12){
		for (int i = 0; i < len-4; ++i){
			fileProc[i] = filename[i];
			fileProc[i] = mytoupper(fileProc[i]);
		}
		for (int j = len - 4; j < len; ++j){
			fileProc[j] = filename[j+1];
			fileProc[j] = mytoupper(fileProc[j]);
		}
	}
	else{
		fileProc[10] = mytoupper(filename[len-1]);
		fileProc[9] = mytoupper(filename[len-2]);
		fileProc[8] = mytoupper(filename[len-3]);
		for (int i = 0; i < len-4; ++i){
			fileProc[i] = mytoupper(filename[i]);
		}
	}
}

int add_device_to_PCB(uint32_t devicePtr, file_descriptor* fd){
	if (devicePtr == dev_UART2){
//		struct stream userptr = currentPCB->openFiles[0]; 
//		if (currentPCB->openFiles[0].minorId == dev_UART2){
			struct stream* userptr = find_open_stream(); //If STDIN is defined, open new UART2 stream
			if (userptr == NULL){
				return E_FREE;
			}
			userptr->deviceType = IO;
			userptr->minorId = devicePtr;
			*fd = (file_descriptor)userptr;
			intSerialIOInit();
			return 0;
//		}
//		userptr.deviceType = IO; //STDIN
//		userptr.minorId = dev_UART2;
//		userptr = currentPCB->openFiles[1]; 
//		userptr.deviceType = IO; //STDOUT
//		userptr.minorId = dev_UART2;
//		userptr = currentPCB->openFiles[2]; 
//		userptr.deviceType = IO; //STDERR
//		userptr.minorId = dev_UART2;
//		return 0;
	}
	struct stream * userptr = find_open_stream();
	if (userptr == NULL){
		if (MYFAT_DEBUG){
			printf("You have too many file streams open \n");
		}
		return E_NOINPUT;
	}
    //TODO: Do I need a dynamic array of which files are open? To prevent double opening? this is a PSET4 issue, right now we only have one proc open
	//populate struct in PCB with file data
	if (devicePtr >= PUSHB_MIN && devicePtr <= PUSHB_MAX){
		userptr->deviceType = PUSHBUTTON;
		pushbuttonInitAll();
		if (MYFAT_DEBUG){
			printf("Pushbutton initialized. \n");
		}
	}
	else if (devicePtr >= LED_MIN && devicePtr <= LED_MAX){
		userptr->deviceType = LED;
		ledInitAll();
		if (MYFAT_DEBUG){
			printf("LED initialized. \n");
		}
	}
	else if (devicePtr >= ADC_MIN && devicePtr <= ADC_MAX){
		userptr->deviceType = ADC;
		adc_init();
		if (MYFAT_DEBUG){
			printf("ADC initialized. \n");
		}
	}
	else if (devicePtr >= TSI_MIN && devicePtr <= TSI_MAX){
		userptr->deviceType = TSI;
		if (g_tsInit == FALSE){
			TSI_Init();
			TSI_Calibrate();
			g_tsInit = TRUE;
		}
		if (MYFAT_DEBUG){
			printf("TSI initialized. \n");
		}
	}
	else{
		if (MYFAT_DEBUG){
			printf("Invalid device name \n");
		}
		return E_DEV;
	}
	userptr->minorId = devicePtr; //This should macro to the correct minorId
	*fd = (file_descriptor)userptr; //device pointer becomes user pointer
	return 0;
}

file_descriptor check_dev_table(char* filename){
	for (int i = 0; i < DEV; ++i){
		if (strncmp(devTable[i].dev_name, filename, strlen(filename)) != 0){
			continue;
		}
		else{
			return devTable[i].minorId;
		}
	}
	return 0;
}

int myfclose (file_descriptor descr){
	struct stream* userptr = (struct stream*)descr;
	if (find_curr_stream(userptr) == FALSE){
		return E_FREE_PERM;
	}
	if (userptr->deviceType != FAT32){
		return remove_device_from_PCB(descr);
	}
	if (g_noFS){
		return E_NOFS; //TODO: errcheck E_NOFS
	}
	//treat as FAT32	
	return file_close(descr);
}

int remove_device_from_PCB(file_descriptor fd){
	struct stream* userptr = (struct stream*)fd;
	if (find_curr_stream(userptr) == FALSE){
		return E_FREE_PERM;
	}
	userptr->deviceType = UNUSED;
	userptr->minorId = dev_null;
	return 0;
}
/* myfgetc() reads the next character from stream and 
 * passes it to a buffer, while also echoing it back to the
 * user's screen.
 * Returns an error code if it encounters an error.  */
int myfgetc (file_descriptor descr, char* bufp){
	int err = 0;
	struct stream* userptr = (struct stream*)descr;
	if (find_curr_stream(userptr) == FALSE){
		return E_FREE_PERM;
	}
	if (userptr->deviceType == FAT32){
		if (userptr->cursor >= userptr->fileSize){
			return E_EOF;
		}
	}
	//device type checks
	if (userptr->minorId == dev_UART2){
		*bufp = getcharFromBuffer();
		putcharIntoBuffer(*bufp); //per instructor, echo-back handled at device level
		return 0;
	}
	int charsreadp = 0;
	if (userptr->deviceType == PUSHBUTTON){
		err = pushb_fgetc(descr);
	}
	else if (userptr->deviceType == LED){
		err = led_fgetc(descr);
	}
	else if (userptr->deviceType == ADC){
		*bufp = adc_fgetc(descr);
		err = 0;
	}
	else if (userptr->deviceType == TSI){
		*bufp = tsi_fgetc(descr);
		err = 0;
	}
	else{ //CASE: FAT32
		if (g_noFS){
			return E_NOFS;
		}
		err = file_getbuf(descr, bufp, 1, &charsreadp);
	}
	return err;
}

int tsi_fgetc(file_descriptor descr) {
	struct stream* userptr = (struct stream*)descr;
	int res;
	if (userptr->minorId == dev_TSI1){
		res = electrode_in(0);
	}
	else if (userptr->minorId == dev_TSI2){
		res = electrode_in(1);
	}
	else if (userptr->minorId == dev_TSI3){
		res = electrode_in(2);
	}
	else if (userptr->minorId == dev_TSI4){
		res = electrode_in(3);
	}
	else {
		res = E_DEV;
	}
	return res;
}

int adc_fgetc(file_descriptor descr) {
	struct stream* userptr = (struct stream*)descr;
	if (userptr->minorId == dev_pot){
		return adc_read(ADC_CHANNEL_POTENTIOMETER);
	}
	if (userptr->minorId == dev_temp){
		unsigned int temp = adc_read(ADC_CHANNEL_TEMPERATURE_SENSOR);
		return temp;
	}
	return E_DEV;
}

/*pushb_fgetc returns 1 if sw1 is pressed, 2 if sw2 is pressed, 3 if both are pressed, otherwise 0*/
int pushb_fgetc(file_descriptor descr){
	if (sw1In() && sw2In()){
		if (MYFAT_DEBUG){
			printf("Pushbutton sw1, sw2 are pressed \n");
		}
		return 3;
	}
	else if (sw1In()){
		if (MYFAT_DEBUG){
			printf("Pushbutton sw1 is pressed \n");
		}
		return 1;
	}
	else if (sw2In()){
		if (MYFAT_DEBUG){
			printf("Pushbutton sw2 is pressed \n");
		}
		return 2;
	}
	if (MYFAT_DEBUG){
		printf("Pushbutton sw1, sw2 are not pressed \n");
	}
	return 0;
}

/*myfgets() reads in at most one less than size characters from a stream and 
 * stores them into the buffer pointed to by s. Reading stops after an 
 * EOF or a newline. If a newline is read, it is stored into the buffer. 
 * A terminating null byte (\0) is added after the last character in the buffer.*/
int myfgets (file_descriptor descr, char* bufp, int buflen){
	struct stream* userptr = (struct stream*)descr;
	if (find_curr_stream(userptr) == FALSE){
		return E_NOINPUT;
	}
	if (userptr->minorId == dev_UART2){ //get string terminating in newline from UART, up to a buffer size limit
		int count = 0;
		char c = getcharFromBuffer();
		while (count < buflen - 1 && c != '\r' && c != '\n'){
			bufp[count] = c;
			putcharIntoBuffer(bufp[count]); //per instructor, echo-back handled at device level
			count ++;
			c = getcharFromBuffer();
		}
		if (count == buflen - 1){
			return E_NOINPUT;
		}
		else{
			putcharIntoBuffer('\r'); //per instructor, echo-back handled at device level
			putcharIntoBuffer('\n'); //per instructor, echo-back handled at device level
			bufp[count] = NULLCHAR;
		}
		return 0;
	}
	if (userptr->deviceType != FAT32){
		return E_DEV;
	}
	if (g_noFS){
		return E_NOFS;
	}
	int err;
	if (userptr->cursor + buflen >= userptr->fileSize){
		return E_EOF;
	}
	int charsreadp;
	if ((err = file_getbuf(descr, bufp, buflen, &charsreadp)) != 0){
		return err;
	}
	bufp[buflen] = '\0';
	return 0; //TODO: why was this return bufp?
}

int myfputc (file_descriptor descr, char bufp){
	int err = 0;
	struct stream* userptr = (struct stream*)descr;
	if (find_curr_stream(userptr) == FALSE){
		return E_NOINPUT;
	}
	if (userptr->minorId == dev_UART2){
		if (bufp == '\r' || bufp == '\n'){ //TODO: correct?
			putcharIntoBuffer('\r');
			putcharIntoBuffer('\n');
		}
		else{
			putcharIntoBuffer(bufp);
		}
	}
	else if (userptr->deviceType == PUSHBUTTON || userptr->deviceType == ADC || userptr->deviceType == TSI){
		return E_DEV;
	}
	else if (userptr->deviceType == LED){
		err = led_fputc(descr);
		if (err == 0 && MYFAT_DEBUG){
			printf("fputc success\n");
		}
	}
	//CASE: FAT32
	else{
		if (g_noFS){
			return E_NOFS;
		}
		err = file_putbuf(descr, &bufp, 1);
	}
	if (err == 0 && MYFAT_DEBUG){
		printf("fputc success\n");
	}
	return err;
}

/*LED_fgetc is defined as 'on'*/
int led_fgetc(file_descriptor descr){
	struct stream* userptr = (struct stream*)descr;
	if (userptr->minorId == dev_E1){
        ledOrangeOn();
	}
	if (userptr->minorId == dev_E2){
        ledBlueOn();
	}
	if (userptr->minorId == dev_E3){
        ledGreenOn();
	}
	if (userptr->minorId == dev_E4){
        ledYellowOn();
	}
	return 0;
}
/*led_fputc is defined as off*/
int led_fputc(file_descriptor descr){
	struct stream* userptr = (struct stream*)descr;
	if (userptr->minorId == dev_E1){
        ledOrangeOff();
	}
	if (userptr->minorId == dev_E2){
        ledBlueOff();
	}
	if (userptr->minorId == dev_E3){
        ledGreenOff();
	}
	if (userptr->minorId == dev_E4){
        ledYellowOff();
	}
	return 0;
}

int myfputs (file_descriptor descr, char* bufp, int buflen){
	int err = -1;
	struct stream* userptr = (struct stream*)descr;
	if (find_curr_stream(userptr) == FALSE){
		return E_NOINPUT;
	}
	if (userptr->minorId == dev_UART2){
		putsNLIntoBuffer(bufp);
	}
	else if (userptr->deviceType == PUSHBUTTON || userptr->deviceType == ADC || userptr->deviceType == TSI || userptr->deviceType == LED){
		return E_DEV;
	}
	else{
		if (g_noFS){
			return E_NOFS;
		}
		err = file_putbuf(descr, &bufp, buflen);
	}
	if (err == 0 && MYFAT_DEBUG){
		printf("fputc success\r\n");
	}
	return err;
}

int mycreate(char* filename){
	//TODO: no pid restrictions on delete and create. OK?
	if (strncmp("dev_", filename, 4) != 0){ //if filename starts with dev_, reject
		;
	}
	else{
		return E_DEV;
	}
	if (g_noFS){
		return E_NOFS;
	}
	//filename processing for FAT32 compliance
	char* fileProc = "           ";
	process_strname(fileProc, filename);
	return dir_create_file(fileProc);
}

int mydelete(char* filename){
	//TODO: no pid restrictions on delete and create. OK?
	if (strncmp("dev_", filename, 4) != 0){ //if filename starts with dev_, reject
		;
	}
	else{
		return E_DEV;
	}
	if (g_noFS){
		return E_NOFS; //TODO: errcheck E_NOFS
	}
	char* fileProc = "           ";
	process_strname(fileProc, filename);
	return dir_delete_file(fileProc);
}

int myseek(file_descriptor descr, uint32_t pos){
	struct stream* userptr = (struct stream*)descr;
	if (find_curr_stream(userptr) == FALSE || userptr->deviceType != FAT32){
		return E_UNFREE;
	}
	return file_set_cursor(descr, pos);
}

int close_all_devices(void){
	for (int i = 0; i < MAXOPEN; i++){ //0,1,2 reserved for stdin, stdout, stderr
		currentPCB->openFiles[i].deviceType = UNUSED;
	}
	//stop any running timers
	PDB0Stop();
	flexTimer0Stop();
	//deactivate any active LEDs
	ledOrangeOff();
	ledYellowOff();
	ledBlueOff();
	ledGreenOff();
    return 0;
}
