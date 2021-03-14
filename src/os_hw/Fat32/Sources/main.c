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
    //DIR_LS_TESTING
    //dir_ls(0);
    //printf("Directory listed \n");
    //FILE_OPEN_CLOSE_TESTING
    //file_descriptor* fileptr = NULL;
    //char *filename = "HOWMIL~1";
    //int err = 0;
    //err = file_open(filename, fileptr);
    //printf("%p \n", *fileptr);
    //FILE_READ TESTING
    //char bufp[512];
    //int read = 0;
    //int* charsreadp = &read;
    //error = file_getbuf(*fileptr, *bufp, 512, charsreadp);
    //bufp[512] = '\0';
    //printf("%s \n", bufp); //TODO: fix this so it prints?
    //printf("%d \n", read);
    //file_close(*fileptr);
    //CREATE_FILE TESTING
    file_descriptor* fileptrTwo = NULL;
    char *filenameTwo = "TEST    TXT";
    dir_create_file(filenameTwo);
    file_open(filenameTwo, fileptrTwo);
    printf("%p \n", *fileptrTwo);
    file_structure_umount();
    return 0;
}
