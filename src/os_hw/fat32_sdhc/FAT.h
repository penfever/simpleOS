/*
 * FAT.h
 * FAT32 FAT (File Allocation Table) Header File
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021 James L. Frankel.  All rights reserved.
 */

#ifndef _FAT_H
#define _FAT_H

#include <stdint.h>

#include "SDCardReader.h"

/* These #define's enable debugging output */
#define FAT_DEBUG 0
#define FAT_INFORMATIVE_PRINTF 0

/* Mask for the low-order 28 bits of a FAT entry; these are the bits that are
   meaningful */
#define FAT_ENTRY_MASK 0x0fffffff

/* Special FAT entries */
/* The entry is not used (i.e., free/available): */
#define FAT_ENTRY_FREE 0
/* The entry is defective and should not be used: */
#define FAT_ENTRY_DEFECTIVE_CLUSTER 0x0ffffff7
/* When reading a FAT entry, end of a cluster linked list is any entry
   greater-than or equal to FAT_ENTRY_RESERVED_TO_END */
#define FAT_ENTRY_RESERVED_TO_END 0x0ffffff8
/* When writing a FAT entry, FAT_ENTRY_ALLOCATED_AND_END_OF_FILE should be used
   for the last allocated cluster in a directory or file */
#define FAT_ENTRY_ALLOCATED_AND_END_OF_FILE 0x0fffffff

/* Returns the FAT entry for cluster "cluster" */
/*   rca is the Relative Card Address returned from sdhc_initialize */
/*   Any necessary I/O is performed by this function */
uint32_t read_FAT_entry(uint32_t rca, uint32_t cluster);

/* Updates (i.e., writes) the FAT entry for cluster "cluster" */
/*   rca is the Relative Card Address returned from sdhc_initialize */
/*   nextCluster is the value to be written to the FAT entry for cluster
     "cluster" */
/*   Any necessary I/O is performed by this function */
void write_FAT_entry(uint32_t rca, uint32_t cluster, uint32_t nextCluster);

/* #define's and functions below this comment are for internal use only */
/* -------------------------------------------------------------------- */

/* These #define's allow output to either the console or UART based on whether
   SDHC_USE_UART is defined */
#undef CONSOLE_PUTS
#ifdef SDHC_USE_UART
#include "uartNL.h"
#define CONSOLE_PUTS(...) uartPutsNL(UART2_BASE_PTR, __VA_ARGS__)
#else
#define CONSOLE_PUTS(...) printf(__VA_ARGS__)
#endif

/* Size of buffer used in snprintf and CONSOLE_PUTS */
#define FAT_OUTPUT_BUFFER_SIZE 129

void invalidate_FAT_by_sector(uint32_t sector_address);

int FAT_copy_verify(uint32_t rca);

#endif /* ifndef _FAT_H */
