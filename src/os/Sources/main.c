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
	//error = intSerIoProj();
	if ((error = init_clocks_sdram()) != 0){
		if (MYFAT_DEBUG){
			printf("sdram/MCG error \n");
		}
		exit(-4);
	}
//	if (UARTIO){
//		file_descriptor descr;
//		if ((error = myfopen(&descr, "dev_UART2", 'w')) != 0){
//			if (MYFAT_DEBUG){
//				printf("UART initialization error \n");
//			}
//			exit(-4);
//		}
//	}
	//error = intSerIoProj();
    if (CONSOLEIO || MYFAT_DEBUG || MYFAT_DEBUG_LITE){
        setvbuf(stdin, NULL, _IONBF, 0); //fix for consoleIO stdin and stdout
        setvbuf(stdout, NULL, _IONBF, 0);	
    }
    if ((error = file_structure_mount()) != 0 && MYFAT_DEBUG){
    	printf("SDHC card could not be mounted. File commands unavailable. \n");
    }
    else if (MYFAT_DEBUG){
    	printf("SDHC card mounted. \n");
        g_noFS = FALSE;
    }
    else{
        g_noFS = FALSE;
    }
    svcInit_SetSVCPriority(15);
    privUnprivileged();
    int err = shell();
    if (MYFAT_DEBUG){
    	printf("Shell exits with code %d \n", err);
    }
	close_all_devices();   //TODO: turn off all LEDs?
	if (!g_noFS){
		file_structure_umount();
	}
	return err;
}
