/*
 * univio.h
 *
 *  Created on: Mar 20, 2021
 *      Author: penfe
 */

#ifndef UNIVIO_H_
#define UNIVIO_H_

file_descriptor fopen (char* filename, char mode);

file_descriptor fclose (char* filename);

char fgetc (file_descriptor descr);

int fputc (char c, file_descriptor descr);

int add_device_to_PCB(file_descriptor fd);

file_descriptor check_dev_table(char* filename);

int seek(file_descriptor descr, uint32_t pos);

#endif /* UNIVIO_H_ */
