/*
 * fsInfo.h
 * FAT32 FSInfo Header File
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021 James L. Frankel.  All rights reserved.
 */

#ifndef _FS_INFO_H
#define _FS_INFO_H

#include <stdint.h>

#include "SDCardReader.h"

/* These #define's enable debugging output */
#define FSI_DEBUG 0

/* Value for FSI_Nxt_Free that indicates it is unknown */
#define FSI_NXT_FREE_UNKNOWN 0xffffffff

/* A hint of the starting cluster number from which to search for a free
   cluster */
/* If it is equal to FSI_NXT_FREE_UNKNOWN, then this field must not be used and
   the search for a free cluster must start with cluster 2 */
/* This field is furnished to reduce the time to find a free cluster */
uint32_t FSI_Nxt_Free;

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
#define FSI_OUTPUT_BUFFER_SIZE 129

/* These macros are used to access fields as little endian even if the computer
   is big endian */
#ifndef LITTLE_ENDIAN_2_BYTES_TO_UINT16
#define LITTLE_ENDIAN_2_BYTES_TO_UINT16(p) ((p)[0] | ((p)[1] << 8))
#endif
#ifndef LITTLE_ENDIAN_4_BYTES_TO_UINT32
#define LITTLE_ENDIAN_4_BYTES_TO_UINT32(p) ((p)[0] | ((p)[1] << 8) | \
((p)[2] << 16) | ((p)[3] << 24))
#endif

/* Required signatures present in the FSInfo sector */
#define FSI_LEADSIG_REQDVAL 0x41615252
#define FSI_STRUCSIG_REQDVAL 0x61417272
#define FSI_TRAILSIG_REQDVAL 0xaa550000

/* The approximate number of free clusters */
/* If equal to FSI_FREE_COUNT_UNKNOWN, then this value must not be used and
   there is no estimate for the number of free clusters */
/* This value is furnished to approximate the free space in the FAT32 file
   structure */
uint32_t FSI_Free_Count;

/* Value for FSI_Free_Count that indicates it is unknown */
#define FSI_FREE_COUNT_UNKNOWN 0xffffffff

/* All of the FAT file systems were originally developed for the IBM
   PC machine architecture.  Hence, on disk data structures for the FAT
   format are all little endian. */

struct FSInfo_sector {
    uint8_t FSI_LeadSig[4];		/* Offset 0 */
    uint8_t FSI_Reserved1[480];		/* Offset 4 */
    uint8_t FSI_StrucSig[4];		/* Offset 484 */
    uint8_t FSI_Free_Count[4];		/* Offset 488 */
    uint8_t FSI_Nxt_Free[4];		/* Offset 492 */
    uint8_t FSI_Reserved2[12];		/* Offset 496 */
    uint8_t FSI_TrailSig[4];		/* Offset 508 */
};

/* Reads and parses the FSInfo sector */
/* Populates FSInfo global variables */
/*   rca is the Relative Card Address returned from sdhc_initialize */
/*   block_address is the sector number of the FSInfo sector */
void FSInfo_sector_read(uint32_t rca, uint32_t block_address);

/* Verifies that the values in the FSInfo sector are exactly the values found
   from reading the entire FAT*/
/* Performs output to show the results of the verification */
/*   rca is the Relative Card Address returned from sdhc_initialize */
void FSInfo_validate(uint32_t rca);

#endif /* ifndef _FS_INFO_H */
