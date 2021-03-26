/*
 * main implementation: use this 'C' sample to create your own application
 *
 */
#include "derivative.h" /* include peripheral declarations */
#include "simpleshell.h"
#include "devices.h"
#include "myerror.h"
#include "univio.h"

int main(void){
	if (UARTIO){
		file_descriptor descr;
		myfopen(&descr, "dev_UART2", 'w');
	}
    if (CONSOLEIO || MYFAT_DEBUG || MYFAT_DEBUG_LITE){
        setvbuf(stdin, NULL, _IONBF, 0); //fix for consoleIO stdin and stdout
        setvbuf(stdout, NULL, _IONBF, 0);	
    }
    int error = file_structure_mount();
    if (0 != error) {
    	uartPutsNL(UART2_BASE_PTR, "SDHC card could not be mounted. File commands unavailable. \n");
    }
    else{
    	uartPutsNL(UART2_BASE_PTR, "SDHC card mounted. \n");
        g_noFS = FALSE;
    }
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
