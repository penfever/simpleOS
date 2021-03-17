/*
 * bootSector.c
 * MBR & VBR (Boot Sector) Implementation
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021 James L. Frankel.  All rights reserved.
 *
 * Last updated: 3:35 PM 16-Mar-2021
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "microSD.h"
#include "bootSector.h"
#include "breakpoint.h"

uint16_t bytes_per_sector;
uint16_t FSInfo_sector_number;
uint32_t sectors_per_cluster;
uint32_t first_FAT_sector;
uint32_t sectors_per_FAT;
uint8_t number_of_FATs;
uint32_t root_directory_cluster;
uint32_t total_data_clusters;
uint8_t BPB_Media;
uint16_t BackupBootSector;

uint32_t first_data_sector;

static int chs_to_cylinder(struct chs_field *CHS_field_p);
static int chs_to_head(struct chs_field *CHS_field_p);
static int chs_to_sector(struct chs_field *CHS_field_p);

void boot_sector_read(uint32_t rca) {
  char output_buffer[BS_OUTPUT_BUFFER_SIZE];

  uint32_t sector_address;
  struct sdhc_card_status card_status;
  uint8_t data[512], copyOfBootSector[512];

  struct boot_sector *boot_sector_p;

  uint16_t uint16, RsvdSecCnt;
  uint32_t uint32, TotSec32, BPB_RootClus;
  uint64_t DiskSize;

  sector_address = 0;

  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Reading absolute sector %lu...\n", sector_address);
    CONSOLE_PUTS(output_buffer);
  }

  /* starting with IBM PC XT computers, the boot sector was read into
     memory starting at location 0x7C00; we're not going to execute the
     code in the boot sector, so the memory address doesn't matter */

  if(SDHC_SUCCESS != sdhc_read_single_block(rca, sector_address, &card_status, data)) {
    __BKPT();
  }

  boot_sector_p = (struct boot_sector *)data;

  int card_format_mbr = 1, acceptableFormat = 1;

  if(boot_sector_p->Signature_word[0] == 0x55) {
    if(BS_DEBUG)
      CONSOLE_PUTS("The Signature_word[0] field is 0x55 as required\n");
  } else {
    if(BS_INFORMATIVE_PRINTF) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "Signature_word[0]: 0x%02X\n", boot_sector_p->Signature_word[0]);
      CONSOLE_PUTS(output_buffer);
      CONSOLE_PUTS("*****The Signature_word[0] field is not 0x55, this is incorrect*****\n");
    }
    acceptableFormat = 0;
  }

  if(boot_sector_p->Signature_word[1] == 0xaa) {
    if(BS_DEBUG)
      CONSOLE_PUTS("The Signature_word[1] field is 0xAA as required\n");
  } else {
    if(BS_INFORMATIVE_PRINTF) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "Signature_word[1]: 0x%02X\n", boot_sector_p->Signature_word[1]);
      CONSOLE_PUTS(output_buffer);
      CONSOLE_PUTS("*****The Signature_word[1] field is not 0xAA, this is incorrect*****\n");
    }
    acceptableFormat = 0;
  }

  if(!acceptableFormat) {
    if(BS_INFORMATIVE_PRINTF) {
      CONSOLE_PUTS("*****Absolute sector 0 is not an MBR or a VBR; terminating*****\n");
      exit(1);	/* exit with an error condition (i.e., non-zero) code */
    } else {
      __BKPT();
    }
  }

  struct modern_standard_MBR *MBR_p;
  struct partition_entry *part_entry_p;
  uint8_t part_status, part_type, part_num;

  MBR_p = (struct modern_standard_MBR *)data;

  part_entry_p = (struct partition_entry *)(MBR_p->MBR_Partition_entry_1);

  /* this is the test that Linux performs to determine if this sector is
     an MBR or a VBR */
  for(part_num = 1; part_num <= PARTITION_NUM_ENTRIES; part_num++) {
    part_status = part_entry_p->status;
    if((part_status != PARTITION_STATUS_NOTBOOTABLE) &&
       (part_status != PARTITION_STATUS_BOOTABLE)) {
      card_format_mbr = 0;
    }
    part_entry_p++;
  }

  if(card_format_mbr) {
    /* this is probably an MBR (Master Boot Record) that contains
       partition information */
    if(BS_INFORMATIVE_PRINTF) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "All four partition entry status bytes in sector 0 are either 0x%02X or 0x%02X;\n\tthis is probably a valid MBR\n",
	       PARTITION_STATUS_NOTBOOTABLE, PARTITION_STATUS_BOOTABLE);
      CONSOLE_PUTS(output_buffer);
    }
  } else {
    if(BS_INFORMATIVE_PRINTF) {
      CONSOLE_PUTS("Because at least one of the four partition entry status bytes in sector 0 is not valid,\n");
      CONSOLE_PUTS("\tthis is probably an unpartitioned FAT32 boot sector\n");
    }
  }

  if(card_format_mbr) {
    int number_of_bootable_partitions;
    uint32_t Disk_signature;
    uint16_t Copy_protect;

    /* this is probably an MBR (Master Boot Record) that contains partition
       information, let's verify that */
    if(BS_INFORMATIVE_PRINTF)
      CONSOLE_PUTS("The Card-Specific Data indicates that the card format contains an MBR (i.e., a paritioned disk)\n");

    if(MBR_p->MBR_Signature_word[0] == 0x55) {
      if(BS_DEBUG)
	CONSOLE_PUTS("The MBR_Signature_word[0] field is 0x55 as required\n");
    } else {
      if(BS_INFORMATIVE_PRINTF) {
	snprintf(output_buffer, sizeof(output_buffer),
		 "MBR_Signature_word[0]: 0x%02X\n",
		 MBR_p->MBR_Signature_word[0]);
	CONSOLE_PUTS(output_buffer);
	CONSOLE_PUTS("*****The MBR_Signature_word[0] field is not 0x55, this is incorrect*****\n");
      }
      acceptableFormat = 0;
    }

    if(MBR_p->MBR_Signature_word[1] == 0xaa) {
      if(BS_DEBUG)
	CONSOLE_PUTS("The MBR_Signature_word[1] field is 0xAA as required\n");
    } else {
      if(BS_INFORMATIVE_PRINTF) {
	snprintf(output_buffer, sizeof(output_buffer),
		 "MBR_Signature_word[1]: 0x%02X\n",
		 MBR_p->MBR_Signature_word[1]);
	CONSOLE_PUTS(output_buffer);
	CONSOLE_PUTS("*****The MBR_Signature_word[1] field is not 0xAA, this is incorrect*****\n");
      }
      acceptableFormat = 0;
    }

    if(!acceptableFormat) {
      if(BS_INFORMATIVE_PRINTF) {
	CONSOLE_PUTS("*****Unacceptable MBR format; terminating*****\n");
	exit(1);	/* exit with an error condition (i.e., non-zero) code */
      } else {
	__BKPT();
      }
    }

    if(BS_DEBUG) {
      Disk_signature =
	LITTLE_ENDIAN_4_BYTES_TO_UINT32(MBR_p->MBR_Disk_signature);
      snprintf(output_buffer, sizeof(output_buffer),
	       "Disk signature: 0x%08lX\n", Disk_signature);
      CONSOLE_PUTS(output_buffer);
      Copy_protect =
	LITTLE_ENDIAN_2_BYTES_TO_UINT16(MBR_p->MBR_Copy_protect);
      snprintf(output_buffer, sizeof(output_buffer),
	       "Copy protect: 0x%04X (%scopy protected)\n",
	       Copy_protect, Copy_protect == MBR_COPY_PROTECTED ? "" : "not ");
      CONSOLE_PUTS(output_buffer);
    }
    
    part_entry_p = (struct partition_entry *)(MBR_p->MBR_Partition_entry_1);
    number_of_bootable_partitions = 0;

    for(part_num = 1; part_num <= PARTITION_NUM_ENTRIES; part_num++) {
      uint32_t first_sector_LBA, sectors_in_partition;

      part_status = part_entry_p->status;
      part_type = part_entry_p->type;
      first_sector_LBA =
	LITTLE_ENDIAN_4_BYTES_TO_UINT32(part_entry_p->first_sector_LBA);
      sectors_in_partition =
	LITTLE_ENDIAN_4_BYTES_TO_UINT32(part_entry_p->sectors_in_partition);

      if(BS_INFORMATIVE_PRINTF) {
	snprintf(output_buffer, sizeof(output_buffer),
		 "Partition %d: Type: 0x%02X%s,\n", part_num,
		 part_type,
		 part_type == PARTITION_TYPE_EMPTY ? " (Empty)" :
		 part_type ==
		 PARTITION_TYPE_FAT32_CHS ? " (DOS 7.1+ FAT32 with CHS Addressing)" :
		 part_type ==
		 PARTITION_TYPE_FAT32_LBA ? " (DOS 7.1+ FAT32 with LBA)" :
		 "");
	CONSOLE_PUTS(output_buffer);
      }
      if(part_status & PARTITION_STATUS_BOOTABLE)
	number_of_bootable_partitions++;
      if(BS_INFORMATIVE_PRINTF) {
	snprintf(output_buffer, sizeof(output_buffer),
		 "\tStatus: 0x%02X (%s), First LBA: %lu,\n",
		 part_status,
		 (part_status & PARTITION_STATUS_BOOTABLE) ? "Bootable/Active" : "Not Bootable/Inactive",
		 first_sector_LBA);
	CONSOLE_PUTS(output_buffer);
	snprintf(output_buffer, sizeof(output_buffer),
		 "\tSectors in partition: %lu, First sector CHS: %d:%d:%d, Last sector CHS: %d:%d:%d\n",
		 sectors_in_partition,
		 chs_to_cylinder(&part_entry_p->first_sector_CHS_address),
		 chs_to_head(&part_entry_p->first_sector_CHS_address),
		 chs_to_sector(&part_entry_p->first_sector_CHS_address),
		 chs_to_cylinder(&part_entry_p->last_sector_CHS_address),
		 chs_to_head(&part_entry_p->last_sector_CHS_address),
		 chs_to_sector(&part_entry_p->last_sector_CHS_address));
	CONSOLE_PUTS(output_buffer);
      }
      if((part_status != PARTITION_STATUS_NOTBOOTABLE) &&
	 (part_status != PARTITION_STATUS_BOOTABLE)) {
	acceptableFormat = 0;
	if(BS_DEBUG)
	  CONSOLE_PUTS("*****Status byte in partition entry is invalid\n");
      }
      part_entry_p++;
      if((part_type == PARTITION_TYPE_FAT32_CHS) &&
	 (first_sector_LBA != 0)) {
	file_structure_first_sector = first_sector_LBA;
      } else if((part_type == PARTITION_TYPE_FAT32_LBA) &&
		(first_sector_LBA != 0)) {
	file_structure_first_sector = first_sector_LBA;
      }
    }

    if(BS_DEBUG && (number_of_bootable_partitions > 1))
      CONSOLE_PUTS("Warning: Partition table contains more than one bootable/active partition\n");
    if(BS_DEBUG) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "Reading sector %lu of FAT32 partition...\n", 0L);
      CONSOLE_PUTS(output_buffer);
    }

    if(file_structure_first_sector == 0)
      if(BS_DEBUG)
	CONSOLE_PUTS("*****No FAT32_CHS or FAT32_LBA partition found; assume there are no partitions*****\n");

    /* now read the boot sector of the FAT32_LBA partition */

    if(SDHC_SUCCESS != sdhc_read_single_block(rca, 0, &card_status, data)) {
      __BKPT();
    }

  }

  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Jump instruction to boot code (BS_jmpBoot): 0x%02X 0x%02X 0x%02X\n",
	     boot_sector_p->BS_jmpBoot[0],
	     boot_sector_p->BS_jmpBoot[1],
	     boot_sector_p->BS_jmpBoot[2]);
    CONSOLE_PUTS(output_buffer);
  }
  if(boot_sector_p->BS_jmpBoot[0] == 0xEB) {
    if(boot_sector_p->BS_jmpBoot[2] != 0x90) {
      if(BS_INFORMATIVE_PRINTF)
	CONSOLE_PUTS("*****BS_jmpBoot has first byte of 0xEB, but third byte is not 0x90*****\n");
      acceptableFormat = 0;
    }
  } else if(boot_sector_p->BS_jmpBoot[0] != 0xE9) {
    if(BS_INFORMATIVE_PRINTF)
      CONSOLE_PUTS("*****First byte of BS_jmpBoot is not 0xEB or 0xE9*****\n");
    acceptableFormat = 0;
  }

  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "OEM Name Identifier (BS_OEMName): %.8s\n",
	     boot_sector_p->BS_OEMName);
    CONSOLE_PUTS(output_buffer);
  }

  bytes_per_sector =
    LITTLE_ENDIAN_2_BYTES_TO_UINT16(boot_sector_p->BPB_BytsPerSec);
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Count of bytes per sector (BPB_BytsPerSec): %u\n",
	     bytes_per_sector);
    CONSOLE_PUTS(output_buffer);
  }
  if(bytes_per_sector != BS_REQUIRED_BYTES_PER_SECTOR) {
    if(BS_INFORMATIVE_PRINTF)
      CONSOLE_PUTS("*****Bytes per sector is not 512; unsupported format*****\n");
    acceptableFormat = 0;
  }

  sectors_per_cluster = boot_sector_p->BPB_SecPerClus;
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Number of sectors per allocation unit/cluster (BPB_SecPerClus): %lu\n",
	     sectors_per_cluster);
    CONSOLE_PUTS(output_buffer);
  }
  if(sectors_per_cluster == 0) {
    if(BS_INFORMATIVE_PRINTF) {
      CONSOLE_PUTS("*****Number of sectors per allocation unit/cluster (BPB_SecPerClus) is\n");
      snprintf(output_buffer, sizeof(output_buffer),
	       "\t%lu, but cannot be zero*****\n", sectors_per_cluster);
      CONSOLE_PUTS(output_buffer);
    }
    acceptableFormat = 0;
  } else if((sectors_per_cluster != 1) &&
	    (sectors_per_cluster != 2) &&
	    (sectors_per_cluster != 4) &&
	    (sectors_per_cluster != 8) &&
	    (sectors_per_cluster != 16) &&
	    (sectors_per_cluster != 32) &&
	    (sectors_per_cluster != 64) &&
	    (sectors_per_cluster != 128)) {
    if(BS_INFORMATIVE_PRINTF) {
      CONSOLE_PUTS("*****Number of sectors per allocation unit/cluster (BPB_SecPerClus) is\n");
      snprintf(output_buffer, sizeof(output_buffer),
	       "\t%lu, but must be a power of two*****\n", sectors_per_cluster);
      CONSOLE_PUTS(output_buffer);
    }
    acceptableFormat = 0;
  } else if(bytes_per_sector*sectors_per_cluster > BS_MAX_BYTES_PER_CLUSTER) {
    if(BS_INFORMATIVE_PRINTF) {
      CONSOLE_PUTS("*****Total number of bytes per cluster is\n");
      snprintf(output_buffer, sizeof(output_buffer),
	       "\t%lu, but must not be greater than %u*****\n",
	       bytes_per_sector*sectors_per_cluster, BS_MAX_BYTES_PER_CLUSTER);
      CONSOLE_PUTS(output_buffer);
    }
    acceptableFormat = 0;
  }

  RsvdSecCnt = LITTLE_ENDIAN_2_BYTES_TO_UINT16(boot_sector_p->BPB_RsvdSecCnt);
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Number of reserved sectors in the reserved region (BPB_RsvdSecCnt): %u\n", RsvdSecCnt);
    CONSOLE_PUTS(output_buffer);
  }

  number_of_FATs = boot_sector_p->BPB_NumFATs;
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "The count of file allocation tables (FATs) on the volume (BPB_NumFATs): %u\n",
	     number_of_FATs);
    CONSOLE_PUTS(output_buffer);
  }
  if(number_of_FATs != 2) {
    if(BS_INFORMATIVE_PRINTF)
      CONSOLE_PUTS("*****BPB_NumFATs is not 2; unsupported format*****\n");
    acceptableFormat = 0;
  }

  uint16 = LITTLE_ENDIAN_2_BYTES_TO_UINT16(boot_sector_p->BPB_RootEntCnt);
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "BPB_RootEntCnt: For FAT32, must be zero: %u\n", uint16);
    CONSOLE_PUTS(output_buffer);
  }
  if(uint16 != 0) {
    if(BS_INFORMATIVE_PRINTF) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "*****BPB_RootEntCnt is %u, but must be zero*****\n", uint16);
      CONSOLE_PUTS(output_buffer);
    }
    acceptableFormat = 0;
  }

  uint16 = LITTLE_ENDIAN_2_BYTES_TO_UINT16(boot_sector_p->BPB_TotSec16);
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "BPB_TotSec16: For FAT32, must be zero: %u\n", uint16);
    CONSOLE_PUTS(output_buffer);
  }
  if(uint16 != 0) {
    if(BS_INFORMATIVE_PRINTF) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "*****BPB_TotSec16 is %u, but must be zero*****\n", uint16);
      CONSOLE_PUTS(output_buffer);
    }
    acceptableFormat = 0;
  }

  BPB_Media = boot_sector_p->BPB_Media;
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "BPB_Media: 0x%02X\n", BPB_Media);
    CONSOLE_PUTS(output_buffer);
  }
  if((BPB_Media != 0xF0) &&
     (BPB_Media != 0xF8) &&
     (BPB_Media != 0xF9) &&
     (BPB_Media != 0xFA) &&
     (BPB_Media != 0xFB) &&
     (BPB_Media != 0xFC) &&
     (BPB_Media != 0xFD) &&
     (BPB_Media != 0xFE) &&
     (BPB_Media != 0xFF)) {
    if(BS_INFORMATIVE_PRINTF) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "*****BPB_Media has an invalid value: 0x%02X*****\n",
	       BPB_Media);
      CONSOLE_PUTS(output_buffer);
    }
    acceptableFormat = 0;
  }

  uint16 = LITTLE_ENDIAN_2_BYTES_TO_UINT16(boot_sector_p->BPB_FATSz16);
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "BPB_FATSz16: For FAT32, must be zero: %u\n", uint16);
    CONSOLE_PUTS(output_buffer);
  }
  if(uint16 != 0) {
    if(BS_INFORMATIVE_PRINTF) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "*****BPB_FATSz16 is %u, but must be zero*****\n", uint16);
      CONSOLE_PUTS(output_buffer);
    }
    acceptableFormat = 0;
  }

  uint16 = LITTLE_ENDIAN_2_BYTES_TO_UINT16(boot_sector_p->BPB_SecPerTrk);
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Sectors per track for interrupt 0x13 (BPB_SecPerTrk): %u\n",
	     uint16);
    CONSOLE_PUTS(output_buffer);
  }

  uint16 = LITTLE_ENDIAN_2_BYTES_TO_UINT16(boot_sector_p->BPB_NumHeads);
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Number of heads for interrupt 0x13 (BPB_NumHeads): %u\n", uint16);
    CONSOLE_PUTS(output_buffer);
  }

  uint32 = LITTLE_ENDIAN_4_BYTES_TO_UINT32(boot_sector_p->BPB_HiddSec);
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Count of hidden sectors (BPB_HiddSec): %lu\n", uint32);
    CONSOLE_PUTS(output_buffer);
  }

  TotSec32 = LITTLE_ENDIAN_4_BYTES_TO_UINT32(boot_sector_p->BPB_TotSec32);
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Total count of sectors on the volume (BPB_TotSec32): %lu\n",
	     TotSec32);
    CONSOLE_PUTS(output_buffer);
  }
  if(TotSec32 == 0) {
    if(BS_INFORMATIVE_PRINTF) {
      CONSOLE_PUTS("*****Total count of sectors on the volume (BPB_TotSec32) is\n");
      snprintf(output_buffer, sizeof(output_buffer),
	       "\t%lu, but must be non-zero*****\n",
	       TotSec32);
      CONSOLE_PUTS(output_buffer);
    }
    acceptableFormat = 0;
  }

  sectors_per_FAT = LITTLE_ENDIAN_4_BYTES_TO_UINT32(boot_sector_p->BPB_FATSz32);
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Count of sectors occupied by one FAT (BPB_FATSz32): %lu\n",
	     sectors_per_FAT);
    CONSOLE_PUTS(output_buffer);
  }

  uint16 =
    LITTLE_ENDIAN_2_BYTES_TO_UINT16(boot_sector_p->BPB_ExtFlags);
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Zero-based number of active FAT (BPB_ExtFlags): %u\n",
	     uint16 & 0xf);
    CONSOLE_PUTS(output_buffer);
    if(uint16 & 0x80) {
      CONSOLE_PUTS("Only one FAT is active (BPB_ExtFlags)\n");
    } else {
      CONSOLE_PUTS("The FAT is mirrored at runtime into all FATs (BPB_ExtFlags)\n");
    }
  }

  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Major version number (BPB_FSVer[1]): %u\n",
	     boot_sector_p->BPB_FSVer[1]);	/* high byte */
    CONSOLE_PUTS(output_buffer);
    snprintf(output_buffer, sizeof(output_buffer),
	     "Minor version number (BPB_FSVer[0]): %u\n",
	     boot_sector_p->BPB_FSVer[0]);	/* low byte */
    CONSOLE_PUTS(output_buffer);
  }
  if(boot_sector_p->BPB_FSVer[1] != BPB_REQUIRED_MAJOR_VERSION) {
    if(BS_INFORMATIVE_PRINTF) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "*****Major version number (BPB_FSVer[1]) is %u, but must be %u*****\n",
	       boot_sector_p->BPB_FSVer[1], BPB_REQUIRED_MAJOR_VERSION);
      CONSOLE_PUTS(output_buffer);
    }
    acceptableFormat = 0;
  }
  if(boot_sector_p->BPB_FSVer[0] != BPB_REQUIRED_MINOR_VERSION) {
    if(BS_INFORMATIVE_PRINTF) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "*****Minor version number (BPB_FSVer[0]) is %u, but must be %u*****\n",
	       boot_sector_p->BPB_FSVer[0], BPB_REQUIRED_MINOR_VERSION);
      CONSOLE_PUTS(output_buffer);
    }
    acceptableFormat = 0;
  }

  BPB_RootClus = LITTLE_ENDIAN_4_BYTES_TO_UINT32(boot_sector_p->BPB_RootClus);
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Cluster number of the first cluster of the root directory (BPB_RootClus): %lu\n",
	     BPB_RootClus);
    CONSOLE_PUTS(output_buffer);
  }
  if(BPB_RootClus < 2) {
    if(BS_INFORMATIVE_PRINTF) {
      CONSOLE_PUTS("*****Cluster number of the first cluster of the root directory (BPB_RootClus) is\n");
      snprintf(output_buffer, sizeof(output_buffer),
	       "\t%lu, must be equal to or greater than 2*****\n",
	       BPB_RootClus);
      CONSOLE_PUTS(output_buffer);
    }
    acceptableFormat = 0;
  }

  FSInfo_sector_number =
    LITTLE_ENDIAN_2_BYTES_TO_UINT16(boot_sector_p->BPB_FSInfo);
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Sector number of FSINFO structure in the reserved area of the FAT32 volume (BPB_FSInfo): %u\n", FSInfo_sector_number);
    CONSOLE_PUTS(output_buffer);
  }

  BackupBootSector =
    LITTLE_ENDIAN_2_BYTES_TO_UINT16(boot_sector_p->BPB_BkBootSec);
  if(BackupBootSector) {
    if(BS_DEBUG) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "Sector number of a copy of the boot record in the reserved area of the FAT32 volume (BPB_BkBootSec): %u\n", BackupBootSector);
      CONSOLE_PUTS(output_buffer);
    }
    if(BackupBootSector != BPB_BKBOOTSEC_RECOMMENDED)
      if(BS_INFORMATIVE_PRINTF) {
	CONSOLE_PUTS("Sector number of a copy of the boot record in the reserved area of the FAT32 volume (BPB_BkBootSec)\n");
	snprintf(output_buffer, sizeof(output_buffer),
		 "\tis %u which is not the recommended value of %u\n", BackupBootSector, BPB_BKBOOTSEC_RECOMMENDED);
	CONSOLE_PUTS(output_buffer);
      }
  } else {
    if(BS_DEBUG)
      CONSOLE_PUTS("No copy of boot record is stored (BPB_BkBootSec == 0)\n");
  }

  int i, allZero;
  allZero = 1;
  for(i = 0; i <12; i++) {
    if(boot_sector_p->BPB_Reserved[i] != 0) {
      allZero = 0;
      break;
    }
  }
  if(allZero) {
    if(BS_DEBUG)
      CONSOLE_PUTS("The BPB_Reserved field is all zero as required\n");
  } else {
    if(BS_INFORMATIVE_PRINTF)
      CONSOLE_PUTS("*****The BPB_Reserved field is not all zeros, this is incorrect*****\n");
  }

  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Interrupt 0x13 drive number (BS_DrvNum): 0x%02X\n",
	     boot_sector_p->BS_DrvNum);
    CONSOLE_PUTS(output_buffer);
  }

  if(boot_sector_p->BS_Reserved1 == 0) {
    if(BS_DEBUG)
      CONSOLE_PUTS("The BS_Reserved1 field is zero as required\n");
  } else {
    if(BS_INFORMATIVE_PRINTF)
      CONSOLE_PUTS("The BS_Reserved1 field is not zero, this is incorrect\n");
  }

  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Extended boot signature (BS_BootSig): 0x%02X\n",
	     boot_sector_p->BS_BootSig);
    CONSOLE_PUTS(output_buffer);
  }
  if(boot_sector_p->BS_BootSig == 0x29) {
    uint32 = LITTLE_ENDIAN_4_BYTES_TO_UINT32(boot_sector_p->BS_VolID);
    if(BS_DEBUG) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "Volume serial number (BS_VolID): %lu (0x%08lX)\n", uint32,
	       uint32);
      CONSOLE_PUTS(output_buffer);
      snprintf(output_buffer, sizeof(output_buffer),
	       "Volume label (BS_VolLab): %.11s\n", boot_sector_p->BS_VolLab);
      CONSOLE_PUTS(output_buffer);
    }
  } else {
    allZero = 1;
    for(i = 0; i <4; i++) {
      if(boot_sector_p->BS_VolID[i] != 0) {
	allZero = 0;
	break;
      }
    }
    if(allZero) {
      if(BS_DEBUG)
	CONSOLE_PUTS("The BS_VolID field is all zero as required\n");
    } else {
      if(BS_INFORMATIVE_PRINTF)
	CONSOLE_PUTS("*****The BS_VolID field is not all zeros, this is incorrect*****\n");
      acceptableFormat = 0;
    }

    allZero = 1;
    for(i = 0; i <11; i++) {
      if(boot_sector_p->BS_VolLab[i] != 0) {
	allZero = 0;
	break;
      }
    }
    if(allZero) {
      if(BS_DEBUG)
	CONSOLE_PUTS("The BS_VolLab field is all zero as required\n");
    } else {
      if(BS_INFORMATIVE_PRINTF)
	CONSOLE_PUTS("*****The BS_VolLab field is not all zeros, this is incorrect*****\n");
      acceptableFormat = 0;
    }
  }

  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "File system type (BS_FilSysType): %.8s\n",
	     boot_sector_p->BS_FilSysType);
    CONSOLE_PUTS(output_buffer);
  }

  allZero = 1;
  for(i = 0; i <11; i++) {
    if(boot_sector_p->BS_Reserved2[i] != 0) {
      allZero = 0;
      break;
    }
  }
  if(allZero) {
    if(BS_DEBUG)
      CONSOLE_PUTS("The BS_Reserved2 field is all zero as suggested\n");
  } else {
    if(BS_DEBUG)
      CONSOLE_PUTS("The BS_Reserved2 field is not all zeros, this is not suggested\n");
  }

  DiskSize = (uint64_t)TotSec32 * (uint64_t)bytes_per_sector;
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Disk size (in bytes) computed from TotSec32*BytsPerSec: 0x%08lX%08lX\n",
	     (uint32_t)(DiskSize>>32), (uint32_t)(DiskSize&0xffffffff));
    CONSOLE_PUTS(output_buffer);
  }

  if(BS_DEBUG)
    CONSOLE_PUTS("\nComputations based on the boot sector\n\n");

  /* The following formulae are from https://wiki.osdev.org/FAT */

  /* FAT size in sectors: */
  /* fat_size = (fat_boot->table_size_16 == 0)? fat_boot_ext_32->table_size_16 : fat_boot->table_size_16; */

  /* The size of the root directory (unless you have FAT32, in which
     case the size will be 0): */
  /* root_dir_sectors = ((fat_boot->root_entry_count * 32) + (fat_boot->bytes_per_sector - 1)) / fat_boot->bytes_per_sector; */

  uint32_t total_data_sectors;
  enum fat_type_t {FAT12, FAT16, FAT32, ExFAT} fat_type;
  char *FAT_name[] = {"FAT12", "FAT16", "FAT32", "ExFAT"};
  
  /* The first data sector (that is, the first sector in which
     directories and files may be stored): */
  first_data_sector = RsvdSecCnt + (number_of_FATs * sectors_per_FAT);
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "First data sector: %lu\n", first_data_sector);
    CONSOLE_PUTS(output_buffer);
  }

  /* The first sector in the File Allocation Table: */
  first_FAT_sector = RsvdSecCnt;
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "First fat sector: %lu\n", first_FAT_sector);
    CONSOLE_PUTS(output_buffer);
  }

  /* The total number of data sectors: */
  total_data_sectors = TotSec32 - (RsvdSecCnt + (number_of_FATs * sectors_per_FAT));
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Total number of data sectors: %lu\n", total_data_sectors);
    CONSOLE_PUTS(output_buffer);
  }

  /* The total number of clusters: */
  total_data_clusters = total_data_sectors / sectors_per_cluster;
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Total number of data clusters: %lu\n", total_data_clusters);
    CONSOLE_PUTS(output_buffer);
  }

  /* The FAT type of this file system: */
  if(total_data_clusters < 4085)
    fat_type = FAT12;
  else if(total_data_clusters < 65525)
    fat_type = FAT16;
  else if (total_data_clusters < 268435445)
    fat_type = FAT32;
  else
    fat_type = ExFAT;
  if(BS_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "FAT type: %s\n", FAT_name[fat_type]);
    CONSOLE_PUTS(output_buffer);
  }

  /* In FAT32, root directory appears in data area on given cluster
     and can be a cluster chain: */
  root_directory_cluster = BPB_RootClus;
  if(BS_INFORMATIVE_PRINTF) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Cluster of root directory: %lu\n", root_directory_cluster);
    CONSOLE_PUTS(output_buffer);
  }

  if(fat_type != FAT32) {
    if(BS_INFORMATIVE_PRINTF)
      CONSOLE_PUTS("*****FAT type of this file system is *not* FAT32*****\n");
    acceptableFormat = 0;
  }

  if(BackupBootSector) {
    if(BS_DEBUG)
      CONSOLE_PUTS("About to read the backup boot sector\n");
    
    /* read the backup boot sector */
    if(SDHC_SUCCESS != sdhc_read_single_block(rca,
					      BackupBootSector,
					      &card_status, copyOfBootSector)) {
      __BKPT();
    }

    int exactCopy = 1;
    for(i = 0; i < bytes_per_sector; i++) {
      if(data[i] != copyOfBootSector[i]) {
	exactCopy = 0;
	break;
      }
    }

    if(!exactCopy) {
      if(BS_INFORMATIVE_PRINTF)
	CONSOLE_PUTS("The backup boot sector is *not* the same as the primary boot sector\n");
    }
  }
  
  if(!acceptableFormat) {
    if(BS_INFORMATIVE_PRINTF) {
      CONSOLE_PUTS("*****Unacceptable FAT32 format; terminating*****\n");
      exit(1);	/* exit with an error condition (i.e., non-zero) code */
    } else {
      __BKPT();
    }
  }
}

uint32_t first_sector_of_cluster(uint32_t cluster){
  return ((cluster - 2) * sectors_per_cluster) + first_data_sector;
}

int boot_record_copy_verify(uint32_t rca) {
  char output_buffer[BS_OUTPUT_BUFFER_SIZE];

  uint32_t main_boot_record_sector_number, copy_boot_record_sector_number, i;
  struct sdhc_card_status card_status;
  uint8_t main_boot_record_data[512], copy_boot_record_data[512];
  int compare_equal;

  compare_equal = 1;
  main_boot_record_sector_number = 0;
  copy_boot_record_sector_number = BackupBootSector;
  for(i = 0; i < 3; i++) {
    if(SDHC_SUCCESS !=
       sdhc_read_single_block(rca, main_boot_record_sector_number, &card_status,
			      main_boot_record_data)) {
      __BKPT();
    }

    if(SDHC_SUCCESS !=
       sdhc_read_single_block(rca, copy_boot_record_sector_number, &card_status,
			      copy_boot_record_data)) {
      __BKPT();
    }

    if(memcmp(main_boot_record_data, copy_boot_record_data, bytes_per_sector)
       != 0) {
      compare_equal = 0;
      if(BS_INFORMATIVE_PRINTF) {
	snprintf(output_buffer, sizeof(output_buffer),
		 "Sector #%lu of main boot record and sector #%lu of copy boot_record are not the same\n", main_boot_record_sector_number, copy_boot_record_sector_number);
	CONSOLE_PUTS(output_buffer);
      }
    }
    main_boot_record_sector_number++;
    copy_boot_record_sector_number++;
  }

  return compare_equal;
}

static int chs_to_cylinder(struct chs_field *CHS_field_p) {
  return (CHS_field_p->cylinderMSBs << 8) | CHS_field_p->cylinderLSBs;
}

static int chs_to_head(struct chs_field *CHS_field_p) {
  return CHS_field_p->head;
}

static int chs_to_sector(struct chs_field *CHS_field_p) {
  return CHS_field_p->sector;
}
