#ifndef _SDHC_FAT32_FILES_H_
#define _SDHC_FAT32_FILES_H_

#include <stdint.h>
#include "directory for students.h"

#define LOOP_CONTD 101
#define FOUND_AND_RETURNING 2
#define BLOCK 512

struct myfat_mount{
   uint32_t rca;
   uint32_t cwd_cluster;
   uint32_t writeSector;
   uint8_t data[BLOCK];
   uint8_t dirty;
};

extern struct myfat_mount *MOUNT;

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
int file_structure_mount(void);

/**
 * Indicates that the microSDHC card is no longer accessible
 * Action this function should take: 
 * - call sdhc_command_send_set_clr_card_detect_connect to re-enable the
 *   card-present-resistor
 * Returns an error code if the file structure is not mounted
 */
int file_structure_umount(void);

/**
 * Sets the cwd to the root directory
 * This is the initial action before the FAT32 file structure is made
 * available through these interfaces
 */
int dir_set_cwd_to_root(void);

/**
 * Display on stdout the cwd's filenames (full == 0) or all directory
 * information (full == 1); implementing full is optional
 */
int dir_ls(int full);

/**
 * Start an iterator at the beginning of the cwd's filenames
 * Returns in *statepp a pointer to a malloc'ed struct that contains necessary
 * state for the iterator
 * Optional with dir_ls_next
 */
int dir_ls_init(void **statepp);

/**
 * Uses statep as a pointer to a struct malloc'ed and initialized by
 * dir_ls_init that contains iterator state
 * Returns in filename the malloc'ed name of the next filename in the cwd; 
 * Caller is responsible to free the memory
 * Returns NULL for filename if at end of the directory; If returning NULL,
 * the malloc'ed struct pointed to by statep is free'd
 * Optional with dir_ls_init
 */
int dir_ls_next(void *statep, char *filename);

/**
 * Search for filename in cwd and return its first cluster number in
 * firstCluster
 * Returns an error code if filename is not in the cwd
 * Returns an error code if the filename is a directory
 */
int dir_find_file(char *filename, uint32_t *firstCluster);

/**
 * Search for filename in cwd and, if it is a directory, set the cwd to that
 * filename
 * Returns an error code if filename is not in the cwd
 * Returns an error code if the filename is not a directory
 * Implementing this function is optional
 */
int dir_set_cwd_to_filename(char *filename);

/**
 * Create a new empty regular file in the cwd with name filename
 * Returns an error code if a regular file or directory already exists with
 * this filename
 * Returns an error code if there is no more space to create the regular file
 */
int dir_create_file(char *filename);

/**
 * Delete a regular file in the cwd with name filename
 * Returns an error code if a file with this name does not exist
 * Returns an error code if a file with this filename is currently open
 * Returns an error code if the file with this name is a directory
 */
int dir_delete_file(char *filename);

/**
 * Create a new empty directory in the cwd with name filename
 * Returns an error code if there is no more space to create the directory
 * Implementing this function is optional
 */
int dir_create_dir(char *filename);

/**
 * Delete a directory in the cwd with name filename
 * Returns an error code if a file with this name does not exist
 * Returns an error code if a directory with this filename contains any files
 * or directories
 * Returns an error code if the file with this name is not a directory
 * Implementing this function is optional
 */
int dir_delete_dir(char *filename);

/**
 * A file_descriptor is used as an index into an array of structures describing
 * open files
 * All entries in the array are initially closed and not associated with any
 * open files
 */
typedef uint32_t file_descriptor;

/**
 * Search for filename in the cwd and, if successful, store a file descriptor
 * for that file into *descrp
 * Returns an error code if filename is not in the cwd
 * Returns an error code if the filename is not a regular file
 */
int file_open(char *filename, file_descriptor *descrp);

/**
 * Close the file associated with descr and disassociate descr from that file
 * Returns an error code if the file descriptor is not open
 * Frees all dynamic storage associated with the formerly open file descriptor
 * and indicates that the descriptor is closed
 */
int file_close(file_descriptor descr);

/**
 * Read sequential characters at the current offset from the file associated
 * with descr into the buffer pointed to by bufp for a maximum length of buflen
 * characters; The actual number of characters read is returned in the int
 * pointed to by charsreadp
 * Returns an error code if the file descriptor is not open
 * Returns an error code if there are no more characters to be read from the
 * file (EOF, this is, End Of File)
 */
int file_getbuf(file_descriptor descr, char *bufp, int buflen, int *charsreadp);

/**
 * Write characters at the current offset into the file associated with descr
 * from the buffer pointed to by bufp with a length of buflen characters
 * Returns an error code if the file descriptor is not open
 * Returns an error code if the file is not writeable
 * (See DIR_ENTRY_ATTR_READ_ONLY), optional to implement read-only
 * Returns an error code if there is no more space to write the character
 */
int file_putbuf(file_descriptor descr, char *bufp, int buflen);


/**
	Sets cursor position of FILE* descr to the position specified in pos.
 */
int file_set_cursor(file_descriptor descr, uint32_t pos);

int dir_read_sector_search(uint8_t data[BLOCK], int logicalSector, char* search, uint32_t currCluster);

int read_all(uint8_t data[BLOCK], int logicalSector, char* search);

struct stream* find_open_stream();

int find_curr_stream(struct stream* fileptr);

int curr_sector_from_offset(struct stream* userptr, int* logicalSector, uint32_t numCluster);

char* filename_clean(char* filename);

uint32_t find_free_cluster();

int dir_extend_dir(int dirPos, uint32_t currCluster);

int dir_set_attr_firstwrite(uint32_t writeSize, struct dir_entry_8_3* writeEntry, uint32_t newFile);

int dir_set_attr_newfile(char* filename, int len);

int dir_set_attr_postwrite(uint32_t writeSize, struct dir_entry_8_3* writeEntry);

int filename_verify(char* filename, int len);

int load_cache(uint32_t logicalSector, uint8_t data[BLOCK], int i);

int search_match(struct dir_entry_8_3* dir_entry, int logicalSector, int i, uint8_t data[BLOCK]);

void print_attr(struct dir_entry_8_3* dir_entry, char* search);

int find_and_assign_clusters(int clusReq, struct stream* userptr, uint32_t numCluster, struct dir_entry_8_3* dir_entry);

char* clean_dir_name(char* dirtyName);

#endif /* ifndef _SDHC_FAT32_FILES_H_ */
