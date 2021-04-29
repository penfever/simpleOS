/*
 * main implementation: use this 'C' sample to create your own application
 *
 */
#include "derivative.h" /* include peripheral declarations */
#include "simpleshell.h"
#include "devices.h"
#include "myerror.h"
#include "univio.h"
#include "uart.h"
#include "uartNL.h"
#include "mymalloc.h"
#include "svc.h"
#include "priv.h"
#include "intSerialIO.h"

	int main(void){
	int error;
	if ((error = init_sys()) != 0){
		if (MYFAT_DEBUG){
			printf("initialization error \n");
		}
		return error;
	}
    //main.c, after init. after calling spawn, main just enters an infinite loop. we never think of it again. It will never run again, we can forget it.
    while (TRUE){
        delay(10000);
    }
}
