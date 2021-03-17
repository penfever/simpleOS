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
//    dir_ls(0);
//    printf("Directory listed \n");
    //FILE_OPEN_CLOSE_TESTING
//    file_descriptor myFile = 0;
//    char *filename = "MARS    TXT";
//    file_open(filename, &myFile);
//    printf("%p \n", myFile);
//    //FILE_READ TESTING
//    char bufp[512];
//    int read = 0;
//    int* charsreadp = &read;
//    error = file_getbuf(myFile, bufp, 512, charsreadp);
//    bufp[512] = '\0';
//    printf("%s \n", bufp); //TODO: fix this so it prints?
//    printf("%d \n", read);
//    file_close(myFile);
    //CREATE_FILE TESTING
    file_descriptor fileptrTwo = 0;
    char *filenameTwo = "TESTD   TXT";
    int err = dir_create_file(filenameTwo);
    printf("Result of file create was %d \n", err);
    err = file_open(filenameTwo, &fileptrTwo);    
    printf("Result of file open was %d \n", err);
    err = file_close(fileptrTwo);
    printf("Result of file close was %d \n", err);
    err = dir_delete_file(filenameTwo);
    printf("Result of file delete was %d \n", err);
    err = file_open(filenameTwo, &fileptrTwo);    
    printf("Result of file open was %d \n", err);
    file_structure_umount();
    return 0;
}
