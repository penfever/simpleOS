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
//    char *myOutFileName = "MARS    TXT";
//    file_open(myOutFileName, &myFile);
//    printf("%p \n", myFile);
    //FILE_READ TESTING
//    char bufp1[512];
    int read = 0;
//    error = file_getbuf(myFile, bufp1, 512, charsreadp);
//    bufp1[512] = '\0';
//    printf("%s \n", bufp1);
//    printf("%d \n", read);
//    read = 0;
//    file_close(myFile);
//    file_open(myOutFileName, &myFile);
//    char bufp2[381];
//    error = file_getbuf(myFile, bufp2, 381, &read);
//    printf("Result of getbuf was %d, read %d chars \n", error, read);
//    read = 0;
//    bufp2[381] = '\0';
//    printf("%s \n", bufp2);
//    char bufp4[381];
//    error = file_getbuf(myFile, bufp4, 381, &read);
//    printf("Result of getbuf was %d, read %d chars \n", error, read);
//    read = 0;
//    bufp4[381] = '\0';
//    printf("%s \n", bufp4);
//    char bufp3[1024];
//    error = file_getbuf(myFile, bufp3, 1024, &read);
//    printf("Result of getbuf was %d, read %d chars \n", error, read);
//    bufp3[1024] = '\0';
//    printf("%s \n", bufp3);
    //CREATE_FILE TESTING
//    file_descriptor fileptrTwo = 0;
//    char *myInFileName = "TESTD   TXT";
//    int err = dir_create_file(myInFileName);
//    printf("Result of file create was %d \n", err);
//    err = file_open(myInFileName, &fileptrTwo);    
//    printf("Result of file open was %d \n", err);
//    err = file_close(fileptrTwo);
//    printf("Result of file close was %d \n", err);
//    err = dir_delete_file(myInFileName);
//    printf("Result of file delete was %d \n", err);
//    err = file_open(myInFileName, &fileptrTwo);    
//    printf("Result of file open was %d \n", err);
    //WRITE FILE TESTING
    file_descriptor myOutFile = 0;
    char *myOutFileName = "MARS    TXT";
    file_open(myOutFileName, &myOutFile);
    int read = 0;
    char myBufpOut[4096];
    error = file_getbuf(myOutFile, myBufpOut, 4096, &read);
    myBufpOut[4096] = '\0';
    file_descriptor myInFile = 0;
    char *myInFileName = "TESTD   TXT";
    error = dir_create_file(myInFileName);
    error = file_open(myInFileName, &myInFile);
    file_putbuf(myInFile, myBufpOut, 125);
    file_putbuf(myInFile, myBufpOut, 125);
    file_putbuf(myInFile, myBufpOut, 512);
    int readTwo = 0;
    char* myBufpIn[762];
    error = file_getbuf(myInFile, myBufpIn, 762, &readTwo);
    printf("Result of getbuf was %d, read %d chars \n", error, read);
    printf("%s \n", myBufpIn);
    file_close(myInFile);
    file_close(myOutFile);
    error = dir_delete_file(myInFile);
    file_structure_umount();
    return 0;
}
