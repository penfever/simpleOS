/*
 * bootSector.h
 * MBR & VBR (Boot Sector) Header File
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021 James L. Frankel.  All rights reserved.
 *
 * Last updated: 3:33 PM 16-Mar-2021
 */

#ifndef _BOOT_SECTOR_H
#define _BOOT_SECTOR_H

#include <stdint.h>

#include "SDCardReader.h"

/* These #define's enable debugging output */
#define BS_DEBUG 0
#define BS_INFORMATIVE_PRINTF 0

/* Number of bytes per sector */
extern uint16_t bytes_per_sector;
/* Number of sectors per cluster */
extern uint32_t sectors_per_cluster;
/* Cluster number of the root directory */
extern uint32_t root_directory_cluster;
/* Total number of data clusters; Remember that the number of the first data
   cluster is 2 */
extern uint32_t total_data_clusters;

/* Returns the number of the first sector in a cluster "cluster" */
uint32_t first_sector_of_cluster(uint32_t cluster);

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
#define BS_OUTPUT_BUFFER_SIZE 129

/* These macros are used to access fields as little endian even if the computer
   is big endian */
#ifndef LITTLE_ENDIAN_2_BYTES_TO_UINT16
#define LITTLE_ENDIAN_2_BYTES_TO_UINT16(p) ((p)[0] | ((p)[1] << 8))
#endif
#ifndef LITTLE_ENDIAN_4_BYTES_TO_UINT32
#define LITTLE_ENDIAN_4_BYTES_TO_UINT32(p) ((p)[0] | ((p)[1] << 8) | \
((p)[2] << 16) | ((p)[3] << 24))
#endif

/* Sector number of the FSInfo sector */
extern uint16_t FSInfo_sector_number;
/* Sector number of the first FAT sector */
extern uint32_t first_FAT_sector;
/* Number of sectors per FAT */
extern uint32_t sectors_per_FAT;
/* Number of FATs */
extern uint8_t number_of_FATs;
/* Indicates the type of media used */
extern uint8_t BPB_Media;
/* Sector number of the first sector of the copy of the boot record */
extern uint16_t BackupBootSector;

#define BPB_BKBOOTSEC_RECOMMENDED 6
#define BS_REQUIRED_BYTES_PER_SECTOR 512
#define BS_MAX_BYTES_PER_CLUSTER 32768
#define BPB_REQUIRED_MAJOR_VERSION 0
#define BPB_REQUIRED_MINOR_VERSION 0

/* All of the FAT file systems were originally developed for the IBM
   PC machine architecture.  Hence, on disk data structures for the FAT
   format are all little endian. */

struct boot_sector {
  uint8_t BS_jmpBoot[3];		/* Offset 0 */
  uint8_t BS_OEMName[8];		/* Offset 3 */
  uint8_t BPB_BytsPerSec[2];		/* Offset 11 */
  uint8_t BPB_SecPerClus;		/* Offset 13 */
  uint8_t BPB_RsvdSecCnt[2];		/* Offset 14 */
  uint8_t BPB_NumFATs;			/* Offset 16 */
  uint8_t BPB_RootEntCnt[2];		/* Offset 17 */
  uint8_t BPB_TotSec16[2];		/* Offset 19 */
  uint8_t BPB_Media;			/* Offset 21 */
  uint8_t BPB_FATSz16[2];		/* Offset 22 */
  uint8_t BPB_SecPerTrk[2];		/* Offset 24 */
  uint8_t BPB_NumHeads[2];		/* Offset 26 */
  uint8_t BPB_HiddSec[4];		/* Offset 28 */
  uint8_t BPB_TotSec32[4];		/* Offset 32 */
  uint8_t BPB_FATSz32[4];		/* Offset 36 */
  uint8_t BPB_ExtFlags[2];		/* Offset 40 */
  uint8_t BPB_FSVer[2];			/* Offset 42 */
  uint8_t BPB_RootClus[4];		/* Offset 44 */
  uint8_t BPB_FSInfo[2];		/* Offset 48 */
  uint8_t BPB_BkBootSec[2];		/* Offset 50 */
  uint8_t BPB_Reserved[12];		/* Offset 52 */
  uint8_t BS_DrvNum;			/* Offset 64 */
  uint8_t BS_Reserved1;			/* Offset 65 */
  uint8_t BS_BootSig;			/* Offset 66 */
  uint8_t BS_VolID[4];			/* Offset 67 */
  uint8_t BS_VolLab[11];		/* Offset 71 */
  uint8_t BS_FilSysType[8];		/* Offset 82 */
  uint8_t BS_Reserved2[420];		/* Offset 90 */
  uint8_t Signature_word[2];		/* Offset 510 */
};

#define MBR_COPY_PROTECTED 0x5A5A

struct modern_standard_MBR {
  uint8_t MBR_jmpBoot[3];		/* Offset 0 */
  uint8_t MBR_Bootstrap_code_area[437];	/* Offset 3 */
  uint8_t MBR_Disk_signature[4];	/* Offset 440 */
  uint8_t MBR_Copy_protect[2];		/* Offset 444 */
  uint8_t MBR_Partition_entry_1[16];    /* Offset 446 */
  uint8_t MBR_Partition_entry_2[16];    /* Offset 462 */
  uint8_t MBR_Partition_entry_3[16];    /* Offset 478 */
  uint8_t MBR_Partition_entry_4[16];    /* Offset 494 */
  uint8_t MBR_Signature_word[2];	/* Offset 510 */
};

#define PARTITION_NUM_ENTRIES 4

#define PARTITION_STATUS_NOTBOOTABLE 0x00
#define PARTITION_STATUS_BOOTABLE 0x80

#define PARTITION_TYPE_EMPTY 0x00
#define PARTITION_TYPE_FAT32_CHS 0x0B
#define PARTITION_TYPE_FAT32_LBA 0x0C

/* Note: Using bit fields to map hardware bit field format relies on
   unspecified and implementation-defined behavior.  This code will
   probably not work correctly on a big endian computer. */

/* Sector numbers start at 1 (not 0) */

struct chs_field {
  uint8_t head;				/* Head */
  uint8_t sector: 6;			/* Sector (in bits 5..0) */
  uint8_t cylinderMSBs: 2;		/* Cylinder bits 9..8 (in bits 7..6) */
  uint8_t cylinderLSBs;			/* Cylinder bits 7..0 */
};

struct partition_entry {
  uint8_t status;
  struct chs_field first_sector_CHS_address;	/* Cylinder, Head, Sector */
  uint8_t type;
  struct chs_field last_sector_CHS_address;	/* Cylinder, Head, Sector */
  uint8_t first_sector_LBA[4];			/* Logical Block Address */
  uint8_t sectors_in_partition[4];
};

/* Reads and parses the MBR & boot sector (VBR) */
/* Populates boot sector global variables */
/*   rca is the Relative Card Address returned from sdhc_initialize */
void boot_sector_read(uint32_t rca);

/* Verifies that the primary and copy boot records are identical */
/* Performs output to show the results of the verification */
/*   rca is the Relative Card Address returned from sdhc_initialize */
/* Returns true if the records are identical and false otherwise */
int boot_record_copy_verify(uint32_t rca);

#endif /* ifndef _BOOT_SECTOR_H */
