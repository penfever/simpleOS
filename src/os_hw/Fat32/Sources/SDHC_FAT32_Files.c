#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "SDHC_FAT32_Files.h"
#include "fsInfo.h"
#include "microSD.h"
#include "FAT.h"
#include "SDCardReader.h"
#include "breakpoint.h"
#include "bootSector.h"
#include "directory for students.h"
#include "mymalloc.h"
#include "myerror.h"
#include "devices.h"

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
struct sdhc_card_status card_status;
struct dir_entry_8_3 *latest; //records most recent dir_entry of successful search

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
	//TODO: check if buffer is clean, if dirty, call putbuf or write?
	//if any files aren't closed, close them
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
    //TODO: verify this is a directory, return error otherwise
    int err = read_all(data, logicalSector, NULL);
    return err;
}

int read_all(uint8_t data[512], int logicalSector, char* search){
	int finished = 1;
	int numSector = 0;
	finished = dir_read_sector_search(data, logicalSector, search);
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
	    	finished = dir_read_sector_search(data, logicalSector, search);
	    }
	    return finished;
}

int dir_read_sector_search(uint8_t data[512], int logicalSector, char* search){//make attr printing optional
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
			//TODO: if FULL is true, print attr
			printf("Attributes: ");
			dir_entry_print_attributes(dir_entry);
			
			printf("\n");
			uint32_t firstCluster = dir_entry->DIR_FstClusLO | (dir_entry->DIR_FstClusHI << 0);
			printf(" First Cluster: %lu\n", firstCluster);
	    	if(strncmp(dir_entry->DIR_Name, search, 8)==0){ //TODO: this may have trouble matching because of file extensions
	    		uint32_t clusterAddr = dir_entry->DIR_FstClusLO | (dir_entry->DIR_FstClusHI << 0);
	    		if(MYFAT_DEBUG){
	    			printf("Sector %d, entry %d is a match for %s\n", logicalSector, i, search);
	    		}
	    		latest = dir_entry;
	    		return (int)clusterAddr;//TODO: risk of truncation?
	    	}
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
 * The idea of this is that when you're trying to do an LS of the directory
 * This mallocs storage for a struct including everything that this dir_ls_next needs to iterate
 * through the directory entry by entry.
 * It initializes the memory and puts a bunch of stuff there that says, I haven't read any entries yet.
 * The first time you call dir_ls_next, it will return the next file in the directory.
 * 
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
 * This gives you each entry, but as a filename only. But this could be expanded
 * so that it gives you a pointer to a struct that contained all the info from that
 * entry.
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
int dir_find_file(char *filename, uint32_t *firstCluster){ //TODO: error check
    uint8_t data[512];
    if (temp->pid != getCurrentPid()){ //case: PIDs do not match
        return E_FREE_PERM;
    }
    if (0 == MOUNT){
        	return 0; //TODO: error checking
        }
    int logicalSector = first_sector_of_cluster(MOUNT->cwd_cluster);
    int numDirEntries = bytes_per_sector/sizeof(struct dir_entry_8_3);
    if(SDHC_SUCCESS != sdhc_read_single_block(MOUNT->rca, logicalSector, &card_status, data)){
    	__BKPT();
    }
    uint32_t err = read_all(data, logicalSector, filename);
    return err;
}

/**
 * Search for filename in cwd and, if it is a directory, set the cwd to that
 * filename
 * Returns an error code if filename is not in the cwd
 * Returns an error code if the filename is not a directory
 * Implementing this function is optional
 */
int dir_set_cwd_to_filename(char *filename){
	//calls find_file
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
 * This should only be receiving FAT32 files and directories
 */
int file_open(char *filename, file_descriptor *descrp){
	uint32_t fileCluster = dir_find_file(filename, MOUNT->cwd_cluster);
		if (fileCluster <= 2){
			//TODO: error handling
			__BKPT();
		}
	int err;
	struct stream* userptr;
	if ((err = find_open_stream(userptr)) != 0){ //Now *descrp has an open slot
		return err;
	}
	//if it is a directory, return error. TODO: figure out how to open directories
    //TODO: Do I need a dynamic array of which files are open? To prevent double opening? this is a PSET4 issue, right now we only have one proc open
	//populate struct in PCB with file data
	userptr->fileName = latest->DIR_Name;
	userptr->clusterAddr = fileCluster;
	userptr->fileSize =latest->DIR_FileSize;
	userptr->pid = getCurrentPid();
	userptr->open = TRUE;
	userptr->cursor = 0;
    *descrp = (file_descriptor *)userptr;
	return 0;
}

int get_file_data(uint8_t read_buffer[512]){
    struct dir_entry_8_3 *dir_entry;
}

int find_open_stream(struct stream *fileptr){
	for (int i = 3; i < MAXOPEN; i++){ //leave space for stdin, stdout, stderr
		if (currentPCB->openFiles[i] == 0){
			fileptr = currentPCB->openFiles[i]; //fileptr now points to the allocated Stream
			return 0;
		}
	}
	return E_UNFREE;
}

/**
 * Close the file associated with descr and disassociate descr from that file
 * Returns an error code if the file descriptor is not open
 * Frees all dynamic storage associated with the formerly open file descriptor
 * and indicates that the descriptor is closed
 */
int file_close(file_descriptor descr){
    return 0;
    //walk currentPCB and null out descr. If there's no match, return error -- that isn't your file
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
	//copy from stream buffer into char buffer
    return 0;
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
    return 0;
}
