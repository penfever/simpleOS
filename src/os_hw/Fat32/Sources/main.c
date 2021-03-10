#include "derivative.h"
#include "microSD.h"
#include "SDHC_FAT32_Files.h"
#include "bootSector.h"
#include "breakpoint.h"
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
    char *file_name = "FUN     ";
    printf("%d \n", dir_find_file(file_name, MOUNT->cwd_cluster));
    file_structure_umount();
    return 0;
}
