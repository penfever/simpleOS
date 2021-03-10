#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include "SDHC_FAT32_Files.h"
#include "fsInfo.h"
#include "microSD.h"
#include "FAT.h"
#include "SDCardReader.h"
#include "breakpoint.h"
#include "bootSector.h"
#include "directory for students.h"

/* All functions return an int which indicates success if 0 and an
   error code otherwise (only some errors are listed) */

/**
 * Indicates that the microSDHC card is to be made available for these
 * function calls
 *
 * Actions this function should take: 
 * - call microSDCardDetectConfig
 * - check to see if a microSDHC card is present
 * - if not present return an error; 
 * - otherwise, call microSDCardDisableCardDetectARMPullDownResistor to disable
 *   the pull-down resistor in the K70 ARM
 * - call sdhc_initialize
 * Returns an error code if the file structure is already mounted
 */
struct myfat_mount *MOUNT;

int file_structure_mount(void){ //TODO: integrate with myerror
    if(MOUNT == 0){
        MOUNT = malloc(sizeof(struct myfat_mount));
        if(MOUNT == NULL){
            printf("could not malloc\n");
            exit(1);
        }
    }
    else{
        printf("Card already mounted\n");
        return 0;
    }
    microSDCardDetectConfig();
    //check if card is inserted
    if (!microSDCardDetectedUsingSwitch()){
        printf("Card not detected with switch. \n");
        free(MOUNT);
        printf("Error, card not detected. \n");
        return 0;
    }
    microSDCardDisableCardDetectARMPullDownResistor();
    MOUNT->rca = sdhc_initialize();
    dir_set_cwd_to_root();
    return 0;
}

/**
 * Indicates that the microSDHC card is no longer accessible
 * Action this function should take: 
 * - call sdhc_command_send_set_clr_card_detect_connect to re-enable the
 *   card-present-resistor
 * Returns an error code if the file structure is not mounted
 */
int file_structure_umount(void){
	//TODO: check if buffer is clean
    if(SDHC_SUCCESS != sdhc_command_send_set_clr_card_detect_connect(MOUNT->rca)){
        printf("Could not re-enable resistor.\n");
        return 1;
    }
}

/**
 * Sets the cwd to the root directory
 * This is the initial action before the FAT32 file structure is made
 * available through these interfaces
 */
int dir_set_cwd_to_root(void){
    if (0 != MOUNT) {
        MOUNT->cwd_cluster=root_directory_cluster;
        return 0;
    }
    printf("Card not mounted\n");
    return 0;}

/**
 * Display on stdout the cwd's filenames (full == 0) or all directory
 * information (full == 1); implementing full is optional
 */

void dir_entry_print_attributes(struct dir_entry_8_3 *dir_entry){
	if(dir_entry->DIR_Attr & DIR_ENTRY_ATTR_READ_ONLY){
		printf("READ_ONLY");
	}
	if(dir_entry->DIR_Attr & DIR_ENTRY_ATTR_HIDDEN){
		printf("Hidden");
	}
	if(dir_entry->DIR_Attr & DIR_ENTRY_ATTR_SYSTEM){
		printf("System");
	}
	if(dir_entry->DIR_Attr & DIR_ENTRY_ATTR_VOLUME_ID){
		printf("Volume ID");
	}
	if(dir_entry->DIR_Attr & DIR_ENTRY_ATTR_DIRECTORY){
		printf("Directory");
	}
}

int dir_ls(int full){
    uint8_t data[512];
    struct sdhc_card_status card_status;
    //check if file system is mounted
    if (0 == MOUNT){
    	return 0; //TODO: error checking
    }
    int logicalSector = first_sector_of_cluster(MOUNT->cwd_cluster);
    if(MYFAT_DEBUG){
    	printf("First sector of cluster: %d\n", logicalSector);
    }
    int numDirEntries = bytes_per_sector/sizeof(struct dir_entry_8_3);
    if(MYFAT_DEBUG){
    	printf("Number of entries per sector: %d\n", numDirEntries);
    }
    if(SDHC_SUCCESS != sdhc_read_single_block(MOUNT->rca, logicalSector, &card_status, data)){
    	__BKPT();
    }
    int finished = 1;
    int numSector = 0;
    finished = dir_read_sector(data, logicalSector);
    while (finished == 1){
        if (numSector < sectors_per_cluster){
        	numSector ++;
        	logicalSector ++; //advance logicalsector pointer
            if(SDHC_SUCCESS != sdhc_read_single_block(MOUNT->rca, logicalSector, &card_status, data)){ //attempt another read
            	__BKPT();
            }
        }
        else{
	    	uint32_t nextAddr = read_FAT_entry(MOUNT->rca, MOUNT->cwd_cluster); //returns a FAT entry
	    	logicalSector = first_sector_of_cluster(nextAddr); //takes a cluster as an argument
	    	//update dir_entry?
        	numSector = 0;
        }
    	finished = dir_read_sector(data, logicalSector);
    }
    ;
    return 0;
}

int dir_read_sector(uint8_t data[512], int logicalSector){
		int i;
	    struct dir_entry_8_3 *dir_entry;
	    int numSector = 0;
	    int numDirEntries = bytes_per_sector/sizeof(struct dir_entry_8_3);
	    for(i = 0, dir_entry = (struct dir_entry_8_3 *) data; i < numDirEntries; i++, dir_entry++){
	    	if(dir_entry->DIR_Name[0] == DIR_ENTRY_LAST_AND_UNUSED){
	    		//we've reached the end of the directory
	    		if(MYFAT_DEBUG){
	    			printf("Reached end of directory at sector %d, entry %d \n", logicalSector, i);
	    		}
	    	return 0;
	    	}
	    	else if(dir_entry->DIR_Name[0] == DIR_ENTRY_UNUSED){
	    		//this directory is unused
	    		if(MYFAT_DEBUG){
	    			printf("Sector %d, entry %d is unused\n", logicalSector, i);
	    		}
	    	continue;
	    	}
	    	else if((dir_entry->DIR_Attr & DIR_ENTRY_ATTR_LONG_NAME_MASK)== DIR_ENTRY_ATTR_LONG_NAME){
	    		//long file name
	    		if(MYFAT_DEBUG){
	    			printf("Sector %d, entry %d has a long file name\n", logicalSector, i);
	    		}
	    	continue;
	    	}    
			int hasExtension = (0 != strncmp((const char*) &dir_entry->DIR_Name[8], "   ", 3));
			printf("%.8s%c%.3s\n", dir_entry->DIR_Name, hasExtension ? '.' : ' ', &dir_entry->DIR_Name[0]);
			printf("Attributes: ");
			dir_entry_print_attributes(dir_entry);
			printf("\n");
			uint32_t firstCluster = dir_entry->DIR_FstClusLO | (dir_entry->DIR_FstClusHI << 0);
			printf(" First Cluster: %lu\n", firstCluster);
			printf(" Size: %lu\n", dir_entry->DIR_FileSize);
			printf("\n\n");
	    }
	    return 1; //return 1 if end of sector reached without end of directory being reached
}

/**
 * Start an iterator at the beginning of the cwd's filenames
 * Returns in *statepp a pointer to a malloc'ed struct that contains necessary
 * state for the iterator
 * Optional with dir_ls_next
 */
int dir_ls_init(void **statepp){
    ;
}

/**
 * Uses statep as a pointer to a struct malloc'ed and initialized by
 * dir_ls_init that contains iterator state
 * Returns in filename the malloc'ed name of the next filename in the cwd; 
 * Caller is responsible to free the memory
 * Returns NULL for filename if at end of the directory; If returning NULL,
 * the malloc'ed struct pointed to by statep is free'd
 * Optional with dir_ls_init
 */
int dir_ls_next(void *statep, char *filename){
    ;
}

/**
 * Search for filename in cwd and return its first cluster number in
 * firstCluster
 * Returns an error code if filename is not in the cwd
 * Returns an error code if the filename is a directory
 */
int dir_find_file(char *filename, uint32_t *firstCluster){
    ;
}

/**
 * Search for filename in cwd and, if it is a directory, set the cwd to that
 * filename
 * Returns an error code if filename is not in the cwd
 * Returns an error code if the filename is not a directory
 * Implementing this function is optional
 */
int dir_set_cwd_to_filename(char *filename){
    ;
}

/**
 * Create a new empty regular file in the cwd with name filename
 * Returns an error code if a regular file or directory already exists with
 * this filename
 * Returns an error code if there is no more space to create the regular file
 */
int dir_create_file(char *filename){
    ;
}

/**
 * Delete a regular file in the cwd with name filename
 * Returns an error code if a file with this name does not exist
 * Returns an error code if a file with this filename is currently open
 * Returns an error code if the file with this name is a directory
 */
int dir_delete_file(char *filename){
    ;
}

/**
 * Create a new empty directory in the cwd with name filename
 * Returns an error code if there is no more space to create the directory
 * Implementing this function is optional
 */
int dir_create_dir(char *filename){
    ;
}

/**
 * Delete a directory in the cwd with name filename
 * Returns an error code if a file with this name does not exist
 * Returns an error code if a directory with this filename contains any files
 * or directories
 * Returns an error code if the file with this name is not a directory
 * Implementing this function is optional
 */
int dir_delete_dir(char *filename){
    ;
}
/**
 * Search for filename in the cwd and, if successful, store a file descriptor
 * for that file into *descrp
 * Returns an error code if filename is not in the cwd
 * Returns an error code if the filename is not a regular file
 */
int file_open(char *filename, file_descriptor *descrp){
    ;
}

/**
 * Close the file associated with descr and disassociate descr from that file
 * Returns an error code if the file descriptor is not open
 * Frees all dynamic storage associated with the formerly open file descriptor
 * and indicates that the descriptor is closed
 */
int file_close(file_descriptor descr){
    ;
}

/**
 * Read sequential characters at the current offset from the file associated
 * with descr into the buffer pointed to by bufp for a maximum length of buflen
 * characters; The actual number of characters read is returned in the int
 * pointed to by charsreadp
 * Returns an error code if the file descriptor is not open
 * Returns an error code if there are no more characters to be read from the
 * file (EOF, this is, End Of File)
 */
int file_getbuf(file_descriptor descr, char *bufp, int buflen, int *charsreadp){
    ;
}

/**
 * Write characters at the current offset into the file associated with descr
 * from the buffer pointed to by bufp with a length of buflen characters
 * Returns an error code if the file descriptor is not open
 * Returns an error code if the file is not writeable
 * (See DIR_ENTRY_ATTR_READ_ONLY), optional to implement read-only
 * Returns an error code if there is no more space to write the character
 */
int file_putbuf(file_descriptor descr, char *bufp, int buflen){
    ;
}
