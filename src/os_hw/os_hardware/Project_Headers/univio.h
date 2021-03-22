#include "SDHC_FAT32_Files.h"

#ifndef UNIVIO_H_
#define UNIVIO_H_

int myfopen (file_descriptor descr, char* filename, char mode);

int myfclose (char* filename);

int myfgetc (file_descriptor descr, char bufp);

char* myfgets (file_descriptor descr, int buflen);

int myfputc (file_descriptor descr, char bufp);

int myfputs (char* bufp, file_descriptor descr, int buflen);

int mycreate(char* filename);

int mydelete(char* filename);

int seek(file_descriptor descr, uint32_t pos);

int add_device_to_PCB(file_descriptor fd);

file_descriptor check_dev_table(char* filename);

#endif /* UNIVIO_H_ */
