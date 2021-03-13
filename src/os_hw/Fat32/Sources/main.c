#include "derivative.h"
#include "microSD.h"
#include "SDHC_FAT32_Files.h"
#include "bootSector.h"
#include "breakpoint.h"
#include "devices.h"
#include <stdio.h>

int main(void){
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
    file_descriptor* fileptr;
    file_open(filename, fileptr);
    printf("%p \n", *fileptr);
    char bufp[512];
    int read = 0;
    int* charsreadp = &read;
    error = file_getbuf(*fileptr, *bufp, 512, charsreadp);
    bufp[512] = '\0';
    //printf("%s \n", bufp); //TODO: fix this so it prints?
    printf("%d \n", read);
    file_close(*fileptr);
    file_structure_umount();
    return 0;
}
