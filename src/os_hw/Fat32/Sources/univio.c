/*
 * univio.c
 *
 *  Created on: Mar 11, 2021
 *      Author: penfe
 */

int fopen (char* filename, char mode){
	//TODO: automatically set PID to 0
	//TODO: if filename contains /dev, scan devices struct
		//find and call matching function/driver
	//else, filename is a file or directory, call FAT32 struct
		//if mode is read, call file_open with a null descriptor, print error code if appropriate, return descriptor
		//if mode is write, do something else
}
