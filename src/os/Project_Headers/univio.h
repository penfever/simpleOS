#include "SDHC_FAT32_Files.h"

#ifndef UNIVIO_H_
#define UNIVIO_H_

/*pushb_fgetc returns 1 if sw1 is pressed, 2 if sw2 is pressed, 3 if both are pressed, otherwise 0*/
#define SW1PRESSED 1
#define SW2PRESSED 2
#define BOTHSWPRESSED 3
#define NOSWPRESSED 0

extern char g_sw;

extern int g_relTim;

int myfopen (file_descriptor* descr, char* filename, char mode);

int myfclose (file_descriptor descr);

int myfgetc (file_descriptor descr, char* bufp);

int myfgets (file_descriptor descr, char* bufp, int buflen);

int myfputc (file_descriptor descr, char bufp);

int pushb_fgetc(file_descriptor descr);

int led_fgetc(file_descriptor descr);

int led_fputc(file_descriptor descr);

int myfputs (file_descriptor descr, char* bufp, int buflen);

int mycreate(char* filename);

int mydelete(char* filename);

int myseek(file_descriptor descr, uint32_t pos);

int add_device_to_PCB(uint32_t devicePtr, file_descriptor* fd, char mode);

file_descriptor check_dev_table(char* filename);

int remove_device_from_PCB(file_descriptor fd);

int close_all_devices(void);

char mytoupper(char c);

void process_strname(char* fileProc, char* filename);

int tsi_fgetc(file_descriptor descr);

int adc_fgetc(file_descriptor descr);

#endif /* UNIVIO_H_ */
