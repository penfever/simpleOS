#include "derivative.h"
#include "microSD.h"
#include "SDHC_FAT32_Files.h"
#include "bootSector.h"
#include "breakpoint.h"
#include "devices.h"
#include <stdio.h>

int main(void){
	struct pcb* currentPCB = &op_sys; //TODO: copy this code into simpleshell
    int error = file_structure_mount();
    if (0 != error) { //TODO: error check
        __BKPT();
    }
    printf("Card mounted. \n");
    dir_set_cwd_to_root();
    printf("CWD changed \n");
    //dir_ls(0);
    //printf("Directory listed \n");
    char *filename = "DRUMSE~1";
    printf("%d \n", file_open(filename, MOUNT->cwd_cluster));
    file_structure_umount();
    return 0;
}
