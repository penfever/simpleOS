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
#include "uart.h"
#include "uartNL.h"

/* All functions return an int which indicates success if 0 and an
   error code otherwise (only some errors are listed) */

struct myfat_mount *MOUNT;
struct sdhc_card_status card_status;
struct dir_entry_8_3 *latest = NULL; //records most recent dir_entry
uint32_t latestSector; //records sector number of most recent dir_entry
struct dir_entry_8_3 *unused = NULL; //records an unused dir_entry
static int g_unusedSeek = FALSE;
struct dir_entry_8_3 *cwd = NULL;
static int g_deleteFlag = FALSE;
static int g_printAll = FALSE;
static int* g_numSector = 0;
struct pcb* currentPCB = &op_sys;

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
int file_structure_mount(void){
    if(MOUNT == 0){
        MOUNT = myMalloc(sizeof(struct myfat_mount));
        if(MOUNT == NULL){
        	if (MYFAT_DEBUG){
                printf("could not malloc\n");
        	}
            exit(1); //TODO: desired behavior?
        }
    }
    else{
    	if (MYFAT_DEBUG){
            printf("Card already mounted\n");
    	}
        return 0;
    }
    microSDCardDetectConfig();
    //check if card is inserted
    if (!microSDCardDetectedUsingSwitch()){
    	if (MYFAT_DEBUG){
            printf("Error, card not detected. \n");
    	}
        myFree(MOUNT);
        return E_NOINPUT; //TODO: errcheck
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
	for (int i = 3; i < MAXOPEN; i++){ //0,1,2 reserved for stdin, stdout, stderr
		if (currentPCB->openFiles[i].deviceType == FAT32){
			currentPCB->openFiles[i].deviceType = UNUSED;
		}
	}
    if(SDHC_SUCCESS != sdhc_command_send_set_clr_card_detect_connect(MOUNT->rca)){
    	if (MYFAT_DEBUG){
            printf("Could not re-enable resistor.\n");
    	}
        return E_NOINPUT; //TODO: errcheck
    }
    myFree(MOUNT); //TODO: attempt to free causes bkpt?
    return 0;
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
    if (MYFAT_DEBUG){
        printf("Card not mounted\n");
    }
    return E_NOINPUT; //TODO: errcheck
}

/*write_cache writes the contents of the cache to disk and sets the dirty flag to clean, or returns an error in case of write failure.*/
int write_cache(){
	if (MOUNT->dirty == TRUE){
	    if(SDHC_SUCCESS != sdhc_write_single_block(MOUNT->rca, MOUNT->writeSector, &card_status, MOUNT->data)){
	    	return E_NOINPUT; //TODO: create new error message
	    }
		MOUNT->dirty = FALSE;
	}
	//MOUNT->data = {0};
	return 0;
}

/**
 * Prints the cwd's filenames (full == 0) or all directory
 * information (full == 1);
 */

void dir_entry_print_attributes(struct dir_entry_8_3 *dir_entry){
	if(dir_entry->DIR_Attr & DIR_ENTRY_ATTR_READ_ONLY){
		if (UARTIO){
			uartPutsNL(UART2_BASE_PTR, "READ_ONLY");
		}
		else if (MYFAT_DEBUG_LITE){
			printf("READ_ONLY");	
		}
	}
	if(dir_entry->DIR_Attr & DIR_ENTRY_ATTR_HIDDEN){
		if (UARTIO){
			uartPutsNL(UART2_BASE_PTR, "Hidden");
		}
		else if (MYFAT_DEBUG_LITE){
			printf("Hidden");	
		}
	}
	if(dir_entry->DIR_Attr & DIR_ENTRY_ATTR_SYSTEM){
		if (UARTIO){
			uartPutsNL(UART2_BASE_PTR, "System");
		}
		else if (MYFAT_DEBUG_LITE){
			printf("System");	
		}
	}
	if(dir_entry->DIR_Attr & DIR_ENTRY_ATTR_VOLUME_ID){
		if (UARTIO){
			uartPutsNL(UART2_BASE_PTR, "Volume ID");
		}
		else if (MYFAT_DEBUG_LITE){
			printf("Volume ID");	
		}
	}
}

/*updates logicalSector with the value for the current working directory and reads the data from that dir-entry into 'data'.*/
int dir_get_cwd(int* logicalSector, uint8_t data[BLOCK]){
	*logicalSector = first_sector_of_cluster(MOUNT->cwd_cluster);
    if(MYFAT_DEBUG){
    	printf("First sector of cluster: %d\n", *logicalSector);
    }
    if(SDHC_SUCCESS != sdhc_read_single_block(MOUNT->rca, *logicalSector, &card_status, data)){
    	return E_NOINPUT; //TODO: create new error message
    }
    return 0;
}

/*Prints cwd contents to the UART and/or console IO.*/
int dir_ls(int full){
	if (full == TRUE){
		g_printAll = TRUE;
	}
	else{
		g_printAll = FALSE;
	}
    int err;
    //check if file system is mounted
    if (0 == MOUNT){
    	return E_NOINPUT; //TODO: create new error message
    }
    int logicalSector = first_sector_of_cluster(MOUNT->cwd_cluster);
    int* lsptr = &logicalSector;
    err = dir_get_cwd(lsptr, MOUNT->data);
    if (err != 0){
    	return err;
    }
//    struct dir_entry_8_3 *dir_entry = (struct dir_entry_8_3 *)MOUNT->data;
//    if (dir_entry->DIR_Attr != DIR_ENTRY_ATTR_DIRECTORY){ //TODO: implement other CWD at some point
//    	return E_NOINPUT;
//    }
    err = read_all(MOUNT->data, logicalSector, NULL);
    return err;
}

int read_all(uint8_t data[BLOCK], int logicalSector, char* search){
	int finished = 1;
	int numSector = 0;
	g_numSector = &numSector;
	int totalSector = 0;
	uint32_t currCluster = MOUNT->cwd_cluster;
	finished = dir_read_sector_search(data, logicalSector, search, currCluster);
	if (g_unusedSeek == FOUND_AND_RETURNING){
		return 0;
	}
	    while (finished == 1){
	        if (numSector < sectors_per_cluster){
	        	totalSector ++;
	        	numSector ++;
	        	logicalSector ++; //advance logicalsector pointer
	            if(SDHC_SUCCESS != sdhc_read_single_block(MOUNT->rca, logicalSector, &card_status, data)){ //attempt another read
	            	__BKPT();
	            }
	        }
	        else{
		    	currCluster = read_FAT_entry(MOUNT->rca, currCluster); //returns a FAT entry
		    	logicalSector = first_sector_of_cluster(currCluster); //takes a cluster as an argument
	        	numSector = 0;
	        	totalSector ++;
	        }
	    	finished = dir_read_sector_search(data, logicalSector, search, currCluster);
	    	if (g_unusedSeek == FOUND_AND_RETURNING){
	    		return 0;
	    	}
			latestSector = logicalSector;
	    }
		int entries = totalSector * bytes_per_sector/sizeof(struct dir_entry_8_3);
		char output[64] = {' '};
		sprintf(output, "Total entries in other sectors = %d. \n", entries);
		if(UARTIO){
			uartPutsNL(UART2_BASE_PTR, output);
		}
		else if (MYFAT_DEBUG || MYFAT_DEBUG_LITE){
			printf(output);
		}
	    return finished;
}

int dir_read_sector_search(uint8_t data[BLOCK], int logicalSector, char* search, uint32_t currCluster){//make attr printing optional
		int i;
	    struct dir_entry_8_3 *dir_entry;
	    int numDirEntries = bytes_per_sector/sizeof(struct dir_entry_8_3);
	    for(i = 0, dir_entry = (struct dir_entry_8_3 *) data; i < numDirEntries; i++, dir_entry++){
	    	if(dir_entry->DIR_Name[0] == DIR_ENTRY_LAST_AND_UNUSED){
	    		//we've reached the end of the directory
	    		if (search != NULL){
	    			return E_NOINPUT; //TODO: file not found error
	    		}
	    		if (g_unusedSeek == TRUE){
	    	    	memcpy(&MOUNT->data, &data, BLOCK);
	    			MOUNT->writeSector = logicalSector;
	    			int err = dir_extend_dir(i, currCluster);//TODO: implement directory extending function
	    			if (err != 0){
	    				__BKPT();//TODO: error check
	    			}
	    			err = load_cache(dir_entry, logicalSector);
	    			if (err != 0){
	    				__BKPT();
	    			}
	    			g_unusedSeek = FOUND_AND_RETURNING;
	    			return 0;
	    		}
    			int firstSector = first_sector_of_cluster(MOUNT->cwd_cluster);
    			if(UARTIO){
    	    		char output[64] = {' '};
        			sprintf(output, "Reached end of directory at sector %d, entry %d. \n", logicalSector, i);
        			uartPutsNL(UART2_BASE_PTR, output);
    			}
	    	return 0;
	    	}
	    	else if(dir_entry->DIR_Name[0] == DIR_ENTRY_UNUSED){
	    		//this directory is unused
	    		if (unused == NULL){
	    			int err = load_cache(dir_entry, logicalSector);
	    			if (err != 0){
	    				__BKPT();
	    			}
	    			if (MYFAT_DEBUG || MYFAT_DEBUG_LITE){
		    			printf("Sector %d, entry %d goes to unused as address %p\n", logicalSector, i, unused);
	    			}
	    		}
	    		if (g_unusedSeek == TRUE){
	    			int err = load_cache(dir_entry, logicalSector);
	    			if (err != 0){
	    				__BKPT();
	    			}
	    			g_unusedSeek = FOUND_AND_RETURNING;
	    			return 0;
	    		}
	    		if(MYFAT_DEBUG){
	    			printf("Sector %d, entry %d is unused\n", logicalSector, i);
	    		}
	    		continue;
	    	}
	    	else if((dir_entry->DIR_Attr & DIR_ENTRY_ATTR_LONG_NAME_MASK)== DIR_ENTRY_ATTR_LONG_NAME){
	    		//long file name
	    		char output[64] = {' '};
    			sprintf(output, "Sector %d, entry %d has a long file name\n", logicalSector, i);
    			if (g_printAll){
        			uartPutsNL(UART2_BASE_PTR, output);
    			}
    			else if (MYFAT_DEBUG){
	    			printf(output);
	    		}
	    	}
	    	else if((dir_entry->DIR_Attr == DIR_ENTRY_ATTR_DIRECTORY)){
	    		char output[64] = {' '};
    			sprintf(output, "Sector %d, entry %d is a directory\n", logicalSector, i);
    			if (g_printAll){
        			uartPutsNL(UART2_BASE_PTR, output);
    			}
    			else if (MYFAT_DEBUG || MYFAT_DEBUG_LITE){
	    			printf(output);
	    		}
	    		continue;
	    	}
			int hasExtension = (0 != strncmp((const char*) &dir_entry->DIR_Name[8], "   ", 3));
	    	if(UARTIO && search == NULL && g_deleteFlag == FALSE){ //TODO: enable for console IO
	    		char output[64] = {' '};
    			sprintf(output, "%.8s%c%.3s\n", dir_entry->DIR_Name, hasExtension ? '.' : ' ', &dir_entry->DIR_Name[8]);
    			uartPutsNL(UART2_BASE_PTR, output);
				if(g_printAll){
	    			uartPutsNL(UART2_BASE_PTR, "Attributes: ");
					dir_entry_print_attributes(dir_entry);
	    			uartPutsNL(UART2_BASE_PTR, "\n");
				}
	    	}
			uint32_t firstCluster = dir_entry->DIR_FstClusLO | (dir_entry->DIR_FstClusHI << 16);
			if(MYFAT_DEBUG){
				printf(" First Cluster: %lu\n", firstCluster);
				printf("First sector of cluster: %d\n", first_sector_of_cluster(firstCluster));
			}
			int noMatch = (0 != strncmp((const char*) &dir_entry->DIR_Name, search, 11));
	    	if(!noMatch){
	    		if (g_deleteFlag == TRUE){
	    			MOUNT->writeSector = logicalSector; //updates writeSector
	    		    if(SDHC_SUCCESS != sdhc_read_single_block(MOUNT->rca, MOUNT->writeSector, &card_status, MOUNT->data)){ //updates cache
	    		    	return E_NOINPUT; //TODO: error checking
	    		    }
	    			dir_entry->DIR_Name[0] = DIR_ENTRY_UNUSED;
	    			memcpy(MOUNT->data, data, BLOCK);
	    		}
	    		uint32_t clusterAddr = dir_entry->DIR_FstClusLO | (dir_entry->DIR_FstClusHI << 16);
    			if(UARTIO){
    	    		char output[64] = {' '};
        			sprintf(output, "Sector %d, entry %d is a match for %s\n", logicalSector, i, search);
        			uartPutsNL(UART2_BASE_PTR, output);
    			}
    			else if(MYFAT_DEBUG || MYFAT_DEBUG_LITE){
	    			printf("Sector %d, entry %d is a match for %s\n", logicalSector, i, search);
	    		}
	    		latest = dir_entry;
	    		if (dir_entry->DIR_Attr == DIR_ENTRY_ATTR_DIRECTORY){
	    			return -20; //TODO: label this as E_DIRENTRY
	    		}
	    		return 0;//TODO: I shouldn't need the cluster address. I should be able to get it from latest
	    	}
    		char output[64] = {' '};
			sprintf(output, " Size: %lu\n\n\n", dir_entry->DIR_FileSize);
			if (g_printAll){
    			uartPutsNL(UART2_BASE_PTR, output);
			}
			else if (MYFAT_DEBUG){
    			printf(output);
    		}
	    }
	    return 1; //return 1 if end of sector reached without end of directory being reached
}

int load_cache(struct dir_entry_8_3* dir_entry, uint32_t logicalSector){
	MOUNT->writeSector = logicalSector; //updates writeSector
    if(SDHC_SUCCESS != sdhc_read_single_block(MOUNT->rca, MOUNT->writeSector, &card_status, MOUNT->data)){ //updates cache
    	return E_NOINPUT; //TODO: error checking
    }
	unused = (struct dir_entry_8_3*)MOUNT->data; //points unused at the write cache in MOUNT
    return 0;
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
int dir_find_file(char *filename, uint32_t *firstCluster){ //TODO: this function should return error codes, it doesn't. It should be taking a cluster as an arg, it doesn't.
    //if (temp->pid != getCurrentPid()){ //case: PIDs do not match
    //    return E_FREE_PERM;
    //}
    if (0 == MOUNT){
    	return E_NOINPUT; //TODO: create new error message
    }
    uint8_t data[BLOCK];
    int logicalSector = 0;
    int err;
    err = dir_get_cwd(&logicalSector, data);
    if (err != 0){
    	return err;
    }
    if (UARTIO){
    	uartPutsNL(UART2_BASE_PTR, "Beginning file search. \n");
    }
    else if (CONSOLEIO){
    	printf("Beginning file search. \n");
    }
    if ((err = read_all(data, logicalSector, filename)) != 0){
    	return err; //TODO: I could just have dir_ls take filename as its argument
    }
    *firstCluster = latest->DIR_FstClusLO | (latest->DIR_FstClusHI << 16);
    return 0;
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
	int len = strlen(filename);
	int err;
	if ((err = filename_verify(filename, len)) != 0){
		return err;
	}
	uint32_t myCluster;
	if (dir_find_file(filename, &myCluster) == 0){
		return E_NOINPUT; //if file is found to exist, return error -- cannot create
	}
	if (unused == NULL){ //if we already know where an unused dir_entry is, use that
		g_unusedSeek = TRUE;
		dir_ls(0); //fills unused slot and copies data to MOUNT->data
		if (unused == NULL){
			return E_NOINPUT; //no free dir_entry could be created (out of space)
		}
		g_unusedSeek = FALSE;
	}
    if(SDHC_SUCCESS != sdhc_read_single_block(MOUNT->rca, MOUNT->writeSector, &card_status, MOUNT->data)){ //verify cache is correct
    	return E_NOINPUT; //TODO: error checking
    }
	dir_set_attr_newfile(filename, len);
	MOUNT->dirty = TRUE;
	if ((err = write_cache()) != 0){
		return err;
	}
	else if (UARTIO){
		uartPutsNL(UART2_BASE_PTR, "File created. \n");
	}
	else if (MYFAT_DEBUG || MYFAT_DEBUG_LITE){
		printf("File created. \n");
	}
	unused = NULL;
    return 0;
}

int dir_extend_dir(int dirPos, uint32_t currCluster){
	unused = (struct dir_entry_8_3*)MOUNT->data;
	//CASE 1: more entries exist in sector
	if (dirPos < bytes_per_sector/sizeof(struct dir_entry_8_3)){
		unused->DIR_Name[0] = DIR_ENTRY_UNUSED;
		unused ++;
		unused->DIR_Name[0] = DIR_ENTRY_LAST_AND_UNUSED;
		unused --;
	}
	//CASE 2: more sectors exist in cluster
	else if (dirPos == bytes_per_sector/sizeof(struct dir_entry_8_3) && *g_numSector < sectors_per_cluster){
		unused->DIR_Name[0] = DIR_ENTRY_UNUSED;
		MOUNT->dirty = TRUE;
	    write_cache();
	    
		uint8_t nextData[512]; //load next sector in cluster
	    if(SDHC_SUCCESS != sdhc_read_single_block(MOUNT->rca, MOUNT->writeSector + 1, &card_status, nextData)){
	    	return E_NOINPUT;
	    }
	    struct dir_entry_8_3* nextDirEntry = (struct dir_entry_8_3*)nextData[512];
	    nextDirEntry->DIR_Name[0] = DIR_ENTRY_LAST_AND_UNUSED;
	    MOUNT->writeSector += 1;
    	memcpy(&MOUNT->data, &nextData, BLOCK);
	}
	//CASE 3, new cluster reqd
	else if (dirPos == bytes_per_sector/sizeof(struct dir_entry_8_3) && *g_numSector == sectors_per_cluster){
		unused->DIR_Name[0] = DIR_ENTRY_UNUSED;
		MOUNT->dirty = TRUE;
	    write_cache();
	    uint32_t nextCluster;
		write_FAT_entry(MOUNT->rca, currCluster, nextCluster);
		if(nextCluster == 0){ //TODO: check slides. Is this correct error checking?
			if(MYFAT_DEBUG || MYFAT_DEBUG_LITE){
				printf("No free cluster found. The disk may be full \n");
			}
			return E_NOINPUT; //disk is probably full
		}
		MOUNT->writeSector = first_sector_of_cluster(nextCluster);
	    if(SDHC_SUCCESS != sdhc_read_single_block(MOUNT->rca, MOUNT->writeSector, &card_status, MOUNT->data)){
	    	return E_NOINPUT;
	    }
	    struct dir_entry_8_3* nextDirEntry = (struct dir_entry_8_3*)MOUNT->data;
	    nextDirEntry->DIR_Name[0] = DIR_ENTRY_LAST_AND_UNUSED;
	}
	else{
		if(MYFAT_DEBUG || MYFAT_DEBUG_LITE){
			printf("An error occurred when attempting to extend the directory \n");
		}
		return E_NOINPUT; //some unexpected case came up
	}
	MOUNT->dirty = TRUE;
	write_cache();
	return 0;
}

/*•Terminate and fill both the main filename part and the extension name field with spaces (0x20)
•If the extension field is all spaces, then the period (‘.’) 
separating the main filename part and the extension is not part of the filename */

int dir_set_attr_newfile(char* filename, int len){
	int i = 0;
	for (; i < 11; i++){ //per instructions, this assumes that filename is exactly 11 chars, padded with spaces
		unused->DIR_Name[i] = (uint8_t)filename[i];
	}
	unused->DIR_Attr = (uint8_t)0x00;
	unused->DIR_NTRes = 0;		/* Offset 12 */
	unused->DIR_CrtTimeHundth = 0;		/* Offset 13 */
	unused->DIR_CrtTime = 0;			/* Offset 14 */
	unused->DIR_CrtDate = 0;			/* Offset 16 */
	unused->DIR_LstAccDate = 0;		/* Offset 18 */
	unused->DIR_FstClusHI = 0;		/* Offset 20 */
	unused->DIR_WrtTime = 0;			/* Offset 22 */
	unused->DIR_WrtDate = 0;			/* Offset 24 */
	unused->DIR_FstClusLO = 0;		/* Offset 26 */
	unused->DIR_FileSize = 0;
	return 0;
}

int dir_set_attr_firstwrite(uint32_t writeSize, struct dir_entry_8_3* writeEntry, uint32_t newFile){
	writeEntry->DIR_FstClusHI = newFile >> 16;  //TODO: error check this math. mask the 16 low order bits of newFile;
	writeEntry->DIR_FstClusLO = newFile & 0x0000FFFF;  //mask the 16 high order bits of newFile;
	writeEntry->DIR_FileSize = writeSize;
	return 0;
}

int dir_set_attr_postwrite(uint32_t writeSize, struct dir_entry_8_3* writeEntry){
	writeEntry->DIR_FileSize = writeSize;
	return 0;
}

/* •The first short filename character DIR_Name[0] may not be a space (0x20)
•There is an implied period (‘.’) between the main part and the extension except when the extension is all spaces
•No lowercase characters may be in the short filename
•The following characters may not appear in the short filename: lowercase characters, 0x22, 
0x2A through 0x2F, 0x3A through 0x3F, 0x5B through 0x5D, and 0x7C
•For our implementation, we will not allow any character with values less than 0x20
•Implies: No special characters */

int filename_verify(char* filename, int len){
	if (len != 11){
		return E_NOINPUT;
	}
	for (int i = 0; i < len; i++){
		char c = filename[i];
		if (c < 0x20 || c == 0x22 || c == 0x7C){
			return E_NOINPUT;
		}
		if (c >= 0x2A && c <= 0x2F){
			return E_NOINPUT;
		}
		if (c >= 0x3A && c <= 0x3F){
			return E_NOINPUT;
		}
		if (c >= 0x5B && c <= 0x5D){
			return E_NOINPUT;
		}
		if (c >= 0x61 && c <= 0x7A) {
			return E_NOINPUT;
		}
	}
	return 0;
}

uint32_t find_free_cluster(){
	uint32_t numCluster = 2;
	uint32_t returnCluster = 1;
	if (FSI_Nxt_Free != FSI_NXT_FREE_UNKNOWN){ //numCluster starts at next free, if it exists
		numCluster = FSI_Nxt_Free;
	}
	while (numCluster < total_data_clusters+1 && returnCluster != FAT_ENTRY_FREE){
    	 returnCluster = read_FAT_entry(MOUNT->rca, numCluster);
    	 if (returnCluster == FAT_ENTRY_FREE){
        	write_FAT_entry(MOUNT->rca, numCluster, FAT_ENTRY_ALLOCATED_AND_END_OF_FILE); 
    		return numCluster;
    	 }
    	 numCluster ++;
	}
	if (numCluster >= total_data_clusters+1){
		if (MYFAT_DEBUG){
			printf("Drive is probably full. \n");
		}
		return 0; //free cluster not found
	}
}

/**
 * Delete a regular file in the cwd with name filename
 * Returns an error code if a file with this name does not exist
 * Returns an error code if a file with this filename is currently open
 * Returns an error code if the file with this name is a directory
 */
int dir_delete_file(char *filename){
	//For current PCB only
	for (int i = 3; i < MAXOPEN; i++){ //leave space for stdin, stdout, stderr
		if (strncmp(&currentPCB->openFiles[i].fileName, filename, 11) == 0){
			return E_NOINPUT; //TODO: error, file already open
		}
	}
	uint32_t myCluster = 0;
	g_deleteFlag = TRUE;
	if ((myCluster = dir_find_file(filename, &myCluster)) != 0){ //if file is not found
		g_deleteFlag = FALSE;
		return E_NOINPUT;
	}
	int err;
	MOUNT->dirty = TRUE;
	if ((err = write_cache()) != 0){
		g_deleteFlag = FALSE;
		return err;
	}
	else if (UARTIO){
		uartPutsNL(UART2_BASE_PTR, "File deleted. \n");
	}
	else if (MYFAT_DEBUG || MYFAT_DEBUG_LITE){
		printf("File deleted. \n");
	}
	g_deleteFlag = FALSE;
	return 0;
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
	uint32_t fileCluster;
	int err;
	if (((err = dir_find_file(filename, &fileCluster))) != 0){
		if (err == -20 && MYFAT_DEBUG){
			printf("Opening directories has not yet been implemented. \n");
		}
		return err;
	}
	struct stream* userptr = find_open_stream();
	if (userptr == NULL){
		return E_UNFREE;
	}
    //TODO: Do I need a dynamic array of which files are open? To prevent double opening? this is a PSET4 issue, right now we only have one proc open
	//populate struct in PCB with file data
	userptr->deviceType = FAT32;
	userptr->minorId = sdhc;
	userptr->fileName = filename;
	userptr->clusterAddr = fileCluster;
	userptr->fileSize =latest->DIR_FileSize;
	userptr->cursor = 0;
    *descrp = (file_descriptor*)userptr;
	return 0;
}

struct stream* find_open_stream(){
	for (int i = 3; i < MAXOPEN; i++){ //leave space for stdin, stdout, stderr
		if (currentPCB->openFiles[i].deviceType == UNUSED){
			struct stream *fileptr = &(currentPCB->openFiles[i]); //fileptr now points to the allocated Stream
			return fileptr;
		}
	}
	return NULL;
}

int find_curr_stream(struct stream* fileptr){
	for (int i = 3; i < MAXOPEN; i++){ //leave space for stdin, stdout, stderr
		if (&(currentPCB->openFiles[i]) == fileptr){
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * Close the file associated with descr and disassociate descr from that file
 * Returns an error code if the file descriptor is not open
 * Frees all dynamic storage associated with the formerly open file descriptor
 * and indicates that the descriptor is closed
 */
int file_close(file_descriptor descr){
	struct stream* userptr = (struct stream*)descr;
	if (find_curr_stream(userptr) == FALSE){
		return E_NOINPUT;
	}
	for (int i = 3; i < MAXOPEN; i++){ //0,1,2 reserved for stdin, stdout, stderr
		if (userptr == &(currentPCB->openFiles[i])){ //match found, release the file
			//TODO: write buffer?
			userptr->deviceType = UNUSED;
			return 0;
		}
	}
	return E_FREE;
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
	struct stream* userptr = (struct stream*)descr;
	*charsreadp = 0;
	if (find_curr_stream(userptr) == FALSE || buflen <= 0 || userptr->clusterAddr == 0){
		return E_NOINPUT;
	}
    uint32_t numCluster = userptr->clusterAddr;
    int logicalSector = first_sector_of_cluster(numCluster); //first sector of file
    uint32_t numSector = curr_sector_from_offset(userptr, &logicalSector, numCluster); //how many sectors to offset
	if (buflen > (userptr->fileSize - userptr->cursor)){ //Prevent attempts to read past EOF
		return E_NOINPUT;
	}
	int pos = userptr->cursor;
	const int end = buflen + userptr->cursor;
    uint8_t data[BLOCK];
    int offBlock = userptr->cursor - BLOCK*numSector; //offset of data block and cursor mod 512
    //offset of bufp will be tracked by charsreadp
	while (pos < end){
		uint32_t diff = end - pos;
		uint32_t blockDiff = BLOCK - offBlock;
	    if (numSector > sectors_per_cluster){ //jump to next cluster, if needed
	    	numCluster = read_FAT_entry(MOUNT->rca, numCluster); //returns a FAT entry
	    	logicalSector = first_sector_of_cluster(numCluster); //takes a cluster as an argument
        	numSector = 0;
	    }
	    if(SDHC_SUCCESS != sdhc_read_single_block(MOUNT->rca, logicalSector, &card_status, data)){ //read next block into memory
	    	return E_NOINPUT;
	    }
	    if (diff <= blockDiff){ //if fewer than 512 chars remain and last sector has been read
	    	memcpy(&bufp[*charsreadp], &data[offBlock], diff);
	    	*charsreadp += diff;
			pos  += diff;
	    	break;
	    }
	    else if (diff < BLOCK && diff > blockDiff){ //if fewer than 512 chars remain and last sector has NOT been read
			memcpy(&bufp[*charsreadp], &data[offBlock], blockDiff); //read remaining chars from block into buffer
			pos  += blockDiff;
			*charsreadp += blockDiff;
			offBlock = 0;
	    }
	    else{ //>512 chars remain
	    	memcpy(&bufp[*charsreadp], &data[offBlock], blockDiff);
			pos  += blockDiff;
			*charsreadp += blockDiff;
			offBlock = 0;
	    }
		logicalSector ++;
		numSector ++;
	}
	userptr->cursor = pos;
	return 0;
}

int curr_sector_from_offset(struct stream* userptr, int* logicalSector, uint32_t numCluster){
	int numSector = 0;
	int cursor = userptr->cursor;
	while (cursor > bytes_per_sector){
		numSector ++;
		cursor -= bytes_per_sector;
	}
    while (numSector > sectors_per_cluster){ //skips ahead clusters as needed
    	numCluster = read_FAT_entry(MOUNT->rca, numCluster); //returns a FAT entry
    	*logicalSector = first_sector_of_cluster(numCluster); //takes a cluster as an argument
    	numSector -= sectors_per_cluster;
    }
    *logicalSector += numSector;
	return numSector;
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
	if (buflen <= 0){ //Negative or zero chars entered
		return E_NOINPUT;
	}
	struct stream* userptr = (struct stream*)descr;
	if (find_curr_stream(userptr) == FALSE || userptr->deviceType != FAT32){
		return E_NOINPUT; //File is not open, or wrong type, or does not belong to this PID
	}
	if (userptr->mode == 'r' || userptr->mode == 'R'){
		return E_NOINPUT; //TODO: error handling
	}
	if (userptr->mode == 'a' || userptr->mode == 'A'){
		userptr->cursor = userptr->fileSize; //TODO: append mode -- move cursor to EOF before writing
	}
    int dirLogicalSector = 0;
    int* dirLogSecPtr = &dirLogicalSector;
    int err = 0;
    uint8_t dirData[BLOCK];
    struct dir_entry_8_3* dir_entry = (struct dir_entry_8_3*)dirData;
	int charsread = 0;
	int *charsreadp = &charsread;
    err = dir_get_cwd(dirLogSecPtr, dirData);
    if (err != 0){
    	return err;
    }
    if ((err = read_all(dirData, *dirLogSecPtr, userptr->fileName)) != 0){ //get logicalSector for dir of current file
    	return err;
    }
    if(SDHC_SUCCESS != sdhc_read_single_block(MOUNT->rca, dirLogicalSector, &card_status, dirData)){ //read next block into memory
    	return E_NOINPUT;
    }
    if (userptr->clusterAddr == 0){ //Assign first cluster
    	uint32_t numCluster = find_free_cluster();
    	if (numCluster == 0){
    	 	return E_FREE; //TODO: check error number
    	}
    	int err;
    	if ((err == dir_set_attr_firstwrite(buflen, dir_entry, numCluster)) != 0){
    	    return err; //file entry not found in directory?
    	    //TODO: undo all the writing?
    	}
    	memcpy(&MOUNT->data, &dirData, BLOCK);
    	MOUNT->writeSector = dirLogicalSector;
    	MOUNT->dirty = TRUE;
    	if ((err = write_cache()) != 0){
    		return err;
    	}
    	userptr->clusterAddr = numCluster;
    }
	uint32_t travelCluster;
	int pos = userptr->cursor;
	const int end = buflen + userptr->cursor; //TODO: errcheck on end? Too large?
    uint32_t clusJump = pos / 512; //Skip ahead this many clusters
    uint32_t clusReq = end / 512; //TODO: note that the read_cursor and write_cursor are shared
    if (end % 512 != 0){
    	clusReq += 1;
    }
    uint32_t numCluster = travelCluster = userptr->clusterAddr;
    for (int i = 0; i < clusReq; i ++){
    	if (i == clusJump){
    		numCluster = travelCluster; //Cluster where cursor should start
    	}
    	if (travelCluster == 0){ //TODO: make sure clusterAddr is always zeroed out in dir_file_attr
    		int err = find_and_assign_clusters(clusReq - i, userptr, travelCluster, dir_entry);
    		if (err != 0){
    			return E_NOINPUT; //TODO: errcheck
    		}
    		break;
    	}
		travelCluster = read_FAT_entry(MOUNT->rca, travelCluster); //returns a FAT entry
    }
    //uint8_t fileData[BLOCK];
	uint32_t fileLogicalSector = first_sector_of_cluster(numCluster);
    uint32_t numSector = curr_sector_from_offset(userptr, &fileLogicalSector, numCluster); //how many sectors to offset (also changes logicalSector)
    if(SDHC_SUCCESS != sdhc_read_single_block(MOUNT->rca, fileLogicalSector, &card_status, MOUNT->data)){ //read next block into memory
    	return E_NOINPUT;
    }
    //offset of bufp will be tracked by charsreadp
	while (pos < end){
	    int offBlock = userptr->cursor - BLOCK*numSector; //offset of data block and cursor mod 512
		uint32_t diff = end - pos;
		uint32_t blockDiff = BLOCK - offBlock;
	    if (numSector > sectors_per_cluster){ //jump to next cluster, if needed
	    	numCluster = read_FAT_entry(MOUNT->rca, numCluster); //returns a FAT entry
	    	fileLogicalSector = first_sector_of_cluster(numCluster); //takes a cluster as an argument
        	numSector = 0;
	    }
	    if (diff <= blockDiff){ //if fewer than 512 chars remain and last sector to be written is in memory
	    	memcpy(&MOUNT->data[offBlock], &bufp[*charsreadp], diff);
	    	*charsreadp += diff;
			pos  += diff;
	    	MOUNT->writeSector = fileLogicalSector;
	    	MOUNT->dirty = TRUE;
	    	if ((err = write_cache()) != 0){
	    		return err;
	    	}
	    	break;
	    }
	    else if (diff < BLOCK && diff > blockDiff){ //if fewer than 512 chars remain and last sector to be written is NOT in memory
			memcpy(&MOUNT->data[offBlock], &bufp[*charsreadp], blockDiff); //read remaining chars from block into buffer
			pos  += blockDiff;
			*charsreadp += blockDiff;
			offBlock = 0;
	    }
	    else{ //>512 chars remain
	    	memcpy(&MOUNT->data[offBlock], &bufp[*charsreadp], blockDiff);
			pos  += blockDiff;
			*charsreadp += blockDiff;
			offBlock = 0;
	    }
	    //TODO: what if amt to write is larger or smaller than 512 bytes? This shouldn't matter, I think?
    	MOUNT->writeSector = fileLogicalSector;
    	MOUNT->dirty = TRUE;
    	if ((err = write_cache()) != 0){
    		return err;
    	}
    	fileLogicalSector ++;
		numSector ++;
	}
	userptr->fileSize = userptr->cursor = pos; //update filesize and cursor in PCB struct and dir_entry
	if ((err = dir_set_attr_postwrite(userptr->fileSize, dir_entry)) != 0){
	    return err; //file entry not found in directory?
	}
	memcpy(&MOUNT->data, &dirData, BLOCK);
	MOUNT->writeSector = dirLogicalSector;
	MOUNT->dirty = TRUE;
	if ((err = write_cache()) != 0){
		return err;
	}
    return 0;
}

/*Gets clusReq new clusters from the FAT and assigns them to the file pointed to at userptr.*/
int find_and_assign_clusters(int clusReq, struct stream* userptr, uint32_t numCluster, struct dir_entry_8_3* dir_entry){
	while (clusReq > 0){
    	uint32_t numCluster = find_free_cluster(); 	//TODO: read_FAT_entry on numCluster
    	if (numCluster == 0){
    	 	return E_FREE; //TODO: check error number
    	}
    	write_FAT_entry(MOUNT->rca, numCluster, FAT_ENTRY_ALLOCATED_AND_END_OF_FILE);
		clusReq --;
	}
	return E_NOINPUT;
}

/*Sets cursor to position pos in file descr*/
int file_set_cursor(file_descriptor descr, uint32_t pos){
	struct stream* userptr = (struct stream*) descr;
	if (find_curr_stream(userptr) != TRUE){
		return E_NOINPUT;
		if(MYFAT_DEBUG){
			printf("This user does not have this file open. \n");
		}
	}
	if (userptr->deviceType != FAT32){
		return E_NOINPUT;
		if(MYFAT_DEBUG){
			printf("This is not a FAT32 device. \n");
		}
	}
	if (pos >= userptr->fileSize){
		if(MYFAT_DEBUG){
			printf("Requested position is larger than file size. \n");
		}
		return E_NOINPUT;
	}
	userptr->cursor = pos;
	return 0;
}

