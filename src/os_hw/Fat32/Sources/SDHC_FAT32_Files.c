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
struct dir_entry_8_3 *latest = NULL; //records most recent dir_entry of successful search
struct dir_entry_8_3 *unused = NULL; //records an unused dir_entry
struct dir_entry_8_3 *cwd = NULL;
static int g_unusedSeek = FALSE;
struct pcb* currentPCB = &op_sys; //TODO: figure out how to get this into a function

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
    printf("Card not mounted\n");
    return 0;}

int write_cache(){
	if (MOUNT->dirty == TRUE){
	    if(SDHC_SUCCESS != sdhc_write_single_block(MOUNT->rca, MOUNT->writeSector, &card_status, MOUNT->data)){
	    	return E_NOINPUT; //TODO: create new error message
	    }
		MOUNT->dirty = FALSE;
	}
	return 0;
}

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

int dir_ls(int full){
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
    //TODO: verify this is a directory, return error otherwise
    err = read_all(MOUNT->data, logicalSector, NULL);
    return err;
}

int read_all(uint8_t data[BLOCK], int logicalSector, char* search){
	int finished = 1;
	int numSector = 0;
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
	    }
		if (MYFAT_DEBUG || MYFAT_DEBUG_LITE){
			int entries = totalSector * bytes_per_sector/sizeof(struct dir_entry_8_3);
			printf("Total entries in other sectors = %d. \n", entries);
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
	    		if (g_unusedSeek == TRUE){
	    			int err = dir_extend_dir(dir_entry, i, logicalSector, currCluster);//TODO: implement directory extending function
	    			if (err != 0){
	    				__BKPT();//TODO: error check
	    			}
	    			MOUNT->writeSector = logicalSector;
	    			g_unusedSeek = FOUND_AND_RETURNING;
	    			return 0;
	    		}
	    		if(MYFAT_DEBUG || MYFAT_DEBUG_LITE){
	    			int firstSector = first_sector_of_cluster(MOUNT->cwd_cluster);
	    			printf("Reached end of directory at sector %d, entry %d. \n", logicalSector, i);
	    			//int entr = i*-1;
	    			//return entr;
	    		}
	    	return 0;
	    	}
	    	else if(dir_entry->DIR_Name[0] == DIR_ENTRY_UNUSED){
	    		//this directory is unused
	    		unused = dir_entry;
	    		if (g_unusedSeek == TRUE){
	    			if (MYFAT_DEBUG || MYFAT_DEBUG_LITE){
		    			printf("Sector %d, entry %d goes to unused as address %p\n", logicalSector, i, unused);
	    			}
	    			MOUNT->writeSector = logicalSector;
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
	    		if(MYFAT_DEBUG){
	    			//printf("Sector %d, entry %d has a long file name\n", logicalSector, i);
	    		}
	    		continue;
	    	}
	    	else if((dir_entry->DIR_Attr == DIR_ENTRY_ATTR_DIRECTORY)){
	    		if(MYFAT_DEBUG || MYFAT_DEBUG_LITE){
	    			printf("Sector %d, entry %d is a directory\n", logicalSector, i);
	    		}
	    		continue;
	    	}
	    	if(MYFAT_DEBUG){
			int hasExtension = (0 != strncmp((const char*) &dir_entry->DIR_Name[8], "   ", 3));
			printf("%.8s%c%.3s\n", dir_entry->DIR_Name, hasExtension ? '.' : ' ', &dir_entry->DIR_Name[8]);
			//TODO: if FULL is true, print attr
			printf("Attributes: ");
			dir_entry_print_attributes(dir_entry);
			printf("\n");
	    	}
			uint32_t firstCluster = dir_entry->DIR_FstClusLO | (dir_entry->DIR_FstClusHI << 0);
			if(MYFAT_DEBUG){
			printf(" First Cluster: %lu\n", firstCluster);
	    	printf("First sector of cluster: %d\n", first_sector_of_cluster(firstCluster));
			}
			int noMatch = (0 != strncmp((const char*) &dir_entry->DIR_Name, search, 11));
	    	if(!noMatch){
	    		uint32_t clusterAddr = dir_entry->DIR_FstClusLO | (dir_entry->DIR_FstClusHI << 0);
	    		if(MYFAT_DEBUG || MYFAT_DEBUG_LITE){
	    			printf("Sector %d, entry %d is a match for %s\n", logicalSector, i, search);
	    		}
	    		latest = dir_entry;
	    		return (int)clusterAddr;//TODO: risk of truncation?
	    	}
	    	if(MYFAT_DEBUG){
			printf(" Size: %lu\n", dir_entry->DIR_FileSize);
			printf("\n\n");
	    	}
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
    //if (temp->pid != getCurrentPid()){ //case: PIDs do not match
    //    return E_FREE_PERM;
    //}
    if (0 == MOUNT){
    	return E_NOINPUT; //TODO: create new error message
    }
    uint8_t data[BLOCK];
    int logicalSector = 0;
    int* lsptr = &logicalSector;
    int err;
    err = dir_get_cwd(lsptr, data);
    if (err != 0){
    	return err;
    }
    err = read_all(data, logicalSector, filename); //TODO: I could just have dir_ls take filename as its argument
    return err; //TODO: is this the best way to return this information?
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
	//uint32_t myCluster = 0;
	//uint32_t* myClusterPtr = &myCluster;
	int err;
	if ((err = filename_verify(filename, len)) != 0){
		return err;
	}
	//if (dir_find_file(filename, myClusterPtr) > 0){ TODO: re-implement this error check before submitting (skipped for speed)
	//	return E_NOINPUT;
	//}
	MOUNT->dirty = TRUE;
	if ((dir_create_dir_entry(filename, len) != 0)){
		return E_NOINPUT;
	}
	write_cache();
    return 0;
}

int dir_create_dir_entry(char* filename, int len){
	if (unused != NULL){ //if we already know where an unused dir_entry is, use that
		//dir_set_attr_newfile(filename, len);
		int i = 0;
		for (; i < 11; i++){ //per instructions, this assumes that filename is exactly 11 chars, padded with spaces
			unused->DIR_Name[i] = (uint8_t)filename[i];
		}
		unused->DIR_Attr = DIR_ENTRY_ATTR_ARCHIVE;
		unused->DIR_FileSize = (uint32_t)0;
		if (MYFAT_DEBUG){
			printf("File created at %p \n", unused);
		}
		unused = NULL;
		return 0;
	}
	g_unusedSeek = TRUE;
	dir_ls(0);
	g_unusedSeek = FALSE;
	if (unused != NULL){
		int i = 0;
		dir_set_attr_newfile(filename, len);
		if (MYFAT_DEBUG || MYFAT_DEBUG_LITE){
			printf("File created at %p \n", unused);
		}
		unused = NULL;
		return 0;
	}
	return E_NOINPUT; //free spot could not be created and was not found
}

int dir_extend_dir(struct dir_entry_8_3* dir_entry, int dirPos, uint32_t logicalSector, uint32_t currCluster){
	//Findeitheranunuseddirectoryentry (designated by DIR_ENTRY_UNUSED) 
	//or, if no unused entry is found, extend the directory by adding an entry to the end
	//•Extendingadirectorymaybeassimpleasusingtheentry tagged as DIR_ENTRY_LAST_AND_UNUSED 
	//for the new entry and tagging the next entry as DIR_ENTRY_LAST_AND_UNUSED
	//•But,if DIR_ENTRY_LAST_AND_UNUSED tags the last entry in a sector,thenextending 
	//thedirectorymayrequire writing DIR_ENTRY_LAST_AND_UNUSED into the first directory entry in the next sector
	//•However, if there are no more sectors in the current cluster, then the FAT needs to be searched to find a 
	//free cluster and the FAT has to be updated to indicate that the formerly free cluster 
	//willbelinkedafterthelastclusterinthedirectory•Then, the first directory entry in the first sector of the 
	//newly allocated cluster would need to be tagged as DIR_ENTRY_LAST_AND_UNUSED
	;
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

int dir_set_attr_firstwrite(uint8_t writeSize, struct dir_entry_8_3* writeEntry, uint32_t newFile){
	//uint32_t clusterAddr = dir_entry->DIR_FstClusLO | (dir_entry->DIR_FstClusHI << 0);
	writeEntry->DIR_FstClusHI = (newFile & 0xFF00)/1000000000000000;  //TODO: error check this math. mask the 16 low order bits of newFile;
	writeEntry->DIR_FstClusLO = (newFile & 0xFF);  //mask the 16 high order bits of newFile;
	//unused->DIR_WrtDate;
	//unused->DIR_WrtTime;
	writeEntry->DIR_FileSize = writeSize;
}

/* •The first short filename character DIR_Name[0] may not be a space (0x20)
•There is an implied period (‘.’) between the main part and the extension except then the extension is all spaces
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
	uint32_t returnCluster = 0;
	if (FSI_Nxt_Free != FSI_NXT_FREE_UNKNOWN){
		numCluster = FSI_Nxt_Free;
	}
	while (numCluster < total_data_clusters+1 && returnCluster != FAT_ENTRY_FREE){
    	 returnCluster = read_FAT_entry(MOUNT->rca, numCluster);
    	 numCluster ++;
	}
	if (numCluster >= total_data_clusters+1){
		return 0; //free cluster not found
	}
	FSI_Nxt_Free = numCluster; //Will this work?
	return returnCluster;
}

/**
 * Delete a regular file in the cwd with name filename
 * Returns an error code if a file with this name does not exist
 * Returns an error code if a file with this filename is currently open
 * Returns an error code if the file with this name is a directory
 */
int dir_delete_file(char *filename){
    //find file
	//if it's 0, return an error
	//otherwise, change file entry to UNUSED
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
			return E_NOINPUT; //TODO: error handling
		}
	struct stream* userptr = find_open_stream();
	if (userptr == NULL){
		return E_UNFREE;
	}
	//if it is a directory, return error. TODO: figure out how to open directories
    //TODO: Do I need a dynamic array of which files are open? To prevent double opening? this is a PSET4 issue, right now we only have one proc open
	//populate struct in PCB with file data
	userptr->deviceType = FAT32;
	userptr->minorId = sdhc;
	userptr->fileName = latest->DIR_Name;
	userptr->clusterAddr = fileCluster;
	userptr->fileSize =latest->DIR_FileSize;
	userptr->cursor = 0;
    *descrp = userptr;
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

struct stream* find_curr_stream(struct stream* fileptr){
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
	if (find_curr_stream(userptr) == FALSE || buflen <= 0){
		return E_NOINPUT;
	}
	int i = 0;
    uint8_t data[BLOCK];
    uint32_t numCluster = userptr->clusterAddr;
    int logicalSector = first_sector_of_cluster(numCluster); //first sector of file
    uint32_t numSector = curr_sector_from_offset(userptr); //how many sectors to offset
    while (numSector > sectors_per_cluster){ //skips ahead clusters as needed
    	numCluster = read_FAT_entry(MOUNT->rca, numCluster); //returns a FAT entry
    	logicalSector = first_sector_of_cluster(numCluster); //takes a cluster as an argument
    	numSector -= sectors_per_cluster;
    }
    logicalSector += numSector;
    // if(SDHC_SUCCESS != sdhc_read_single_block(MOUNT->rca, logicalSector, &card_status, data)){
    	//__BKPT(); TODO: Breakpoint triggers before function?
    //}
    uint32_t remSize = userptr->fileSize - userptr->cursor;
	if (buflen > remSize){
		buflen = remSize;
	}
	while (i < buflen){
	    if(SDHC_SUCCESS != sdhc_read_single_block(MOUNT->rca, logicalSector, &card_status, data)){
	    	__BKPT();
	    }
		if (buflen - i < bytes_per_sector){
			for (int j = 0; j < buflen - i; j++){
				char c = data[j];
				sscanf(c, "%s", bufp);
			}
			char c = '\0';
			sscanf(c, "%s", bufp); //TODO: do IO buffers have to be exactly the same size for sscanf to work?
			userptr->cursor += i;
			*charsreadp = i;
		    return 0;
		}
		else{
			memcpy(bufp, data, BLOCK); //TODO: Should I be copying this much memory? Not always
		}
		i += bytes_per_sector;
		logicalSector ++;
		numSector ++;
	    if (numSector > sectors_per_cluster){
	    	numCluster = read_FAT_entry(MOUNT->rca, numCluster); //returns a FAT entry
	    	logicalSector = first_sector_of_cluster(numCluster); //takes a cluster as an argument
        	numSector = 0;
	    }
	}
	//char c = '/0'; //TODO: do IO buffers have to be exactly the same size for sscanf to work?
	//sscanf(c, "%s", bufp);
	userptr->cursor += i;
	*charsreadp = i;
    return 0;
}

int curr_sector_from_offset(struct stream* userptr){
	int numSector = 0;
	int cursor = userptr->cursor;
	while (cursor > bytes_per_sector){
		numSector ++;
		cursor -= bytes_per_sector;
	}
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
    uint8_t data[BLOCK];
    int logicalSector = 0;
    int* lsptr = &logicalSector;
    int err = dir_get_cwd(lsptr, data);
	if (buflen <= 0 || err != 0){
		return E_NOINPUT;
	}
    struct dir_entry_8_3* cwd = (struct dir_entry_8_3*)data;
	struct stream* userptr = (struct stream*)descr; //TODO: errcheck
	// if it is a dir_entry AND the file it points to is filesize 0, then ...
	uint32_t emptyCluster = find_free_cluster();
    if (emptyCluster == 0){
    	return E_FREE; //TODO: check error number
    }
    write_FAT_entry(MOUNT->rca, emptyCluster, FAT_ENTRY_ALLOCATED_AND_END_OF_FILE); //I think this creates a FAT entry for emptyCluster with no pointer to next cluster?
    err = dir_set_attr_firstwrite((uint8_t)buflen, cwd, emptyCluster);
    if (err != 0){
    	return E_NOINPUT; //file entry not found in directory?
    }
    return 0;
}
