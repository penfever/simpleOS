/*
 * FAT.c
 * FAT32 FAT (File Allocation Table) Implementation
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021 James L. Frankel.  All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "microSD.h"
#include "FAT.h"
#include "bootSector.h"
#include "breakpoint.h"

uint32_t FAT_sector_valid;
uint32_t FAT_in_mem_0_origin_sector;
uint32_t FAT_sector[512/4];

static int FAT_endiannessDetermined = 0;
/* FAT_endian will indicate if the *computer* is little or big endian;
   The format of data in FAT32 file structures is always little endian */
static enum endianness {
  endian_little,
  endian_big
} FAT_endian;

static void determineEndianness(void);

static void read_FAT(uint32_t rca, uint8_t *data, uint32_t sector);

static void write_FAT(uint32_t rca, uint8_t *data, uint32_t sector);

static void determineEndianness(void) {
  char output_buffer[FAT_OUTPUT_BUFFER_SIZE];

  union union_32 {
    uint32_t uint32;
    uint8_t uint8[4];
  } union_32;

  union_32.uint8[0] = 0;
  union_32.uint8[1] = 1;
  union_32.uint8[2] = 2;
  union_32.uint8[3] = 3;

  if(union_32.uint32 == 0x03020100) {
    FAT_endian = endian_little;
  } else {
    FAT_endian = endian_big;
  }

  if(FAT_DEBUG) {
    snprintf(output_buffer, FAT_OUTPUT_BUFFER_SIZE,
	     "Computer is %s endian\n",
	     (FAT_endian == endian_little) ? "little" : "big");
    CONSOLE_PUTS(output_buffer);
  }
}

uint32_t read_FAT_entry(uint32_t rca, uint32_t cluster) {
  char output_buffer[FAT_OUTPUT_BUFFER_SIZE];

  uint32_t FAT_0_origin_sector_for_cluster_num = cluster/(bytes_per_sector/4);
  uint32_t FAT_entry_offset_in_sector = cluster%(bytes_per_sector/4);
  uint32_t FAT_sector_to_access =
    FAT_0_origin_sector_for_cluster_num+first_FAT_sector;
  
  if(FAT_DEBUG) {
    snprintf(output_buffer, FAT_OUTPUT_BUFFER_SIZE,
	     "Reading FAT entry for cluster %lu\n", cluster);
    CONSOLE_PUTS(output_buffer);
  }
  if(!FAT_sector_valid) {
    if(FAT_DEBUG)
      CONSOLE_PUTS("No FAT sector is currently in memory\n");
    /* no FAT sector is currently in memory; read the needed sector */
    read_FAT(rca, (uint8_t *)FAT_sector, FAT_sector_to_access);
    FAT_in_mem_0_origin_sector = FAT_0_origin_sector_for_cluster_num;
    FAT_sector_valid = 1;
  } else if(FAT_0_origin_sector_for_cluster_num == FAT_in_mem_0_origin_sector) {
    /* FAT sector we need is currently in memory; just return needed entry */
    if(FAT_DEBUG)
      CONSOLE_PUTS("Correct FAT sector is currently in memory\n");
  } else {
    /* A FAT sector is in memory, but not the one we need; read the needed
       sector */
    if(FAT_DEBUG)
      CONSOLE_PUTS("Incorrect FAT sector is currently in memory\n");
    read_FAT(rca, (uint8_t *)FAT_sector, FAT_sector_to_access);
    FAT_in_mem_0_origin_sector = FAT_0_origin_sector_for_cluster_num;
  }

  if(FAT_DEBUG) {
    snprintf(output_buffer, FAT_OUTPUT_BUFFER_SIZE,
	     "The FAT entry for cluster %lu is %lu (0x%08lX)\n", cluster,
	     FAT_sector[FAT_entry_offset_in_sector],
	     FAT_sector[FAT_entry_offset_in_sector]);
    CONSOLE_PUTS(output_buffer);
  }
  return FAT_sector[FAT_entry_offset_in_sector] & FAT_ENTRY_MASK;
}

void write_FAT_entry(uint32_t rca, uint32_t cluster, uint32_t nextCluster) {
  char output_buffer[FAT_OUTPUT_BUFFER_SIZE];

  uint32_t FAT_0_origin_sector_for_cluster_num = cluster/(bytes_per_sector/4);
  uint32_t FAT_entry_offset_in_sector = cluster%(bytes_per_sector/4);
  uint32_t FAT_sector_to_access =
    FAT_0_origin_sector_for_cluster_num+first_FAT_sector;
  
  if(FAT_DEBUG) {
    snprintf(output_buffer, FAT_OUTPUT_BUFFER_SIZE,
	     "Writing FAT entry for cluster %lu to be %lu\n",
	     cluster, nextCluster);
    CONSOLE_PUTS(output_buffer);
  }
  if(!FAT_sector_valid) {
    if(FAT_DEBUG)
      CONSOLE_PUTS("No FAT sector is currently in memory\n");
    /* no FAT sector is currently in memory; read the needed sector */
    read_FAT(rca, (uint8_t *)FAT_sector, FAT_sector_to_access);
    FAT_in_mem_0_origin_sector = FAT_0_origin_sector_for_cluster_num;
    FAT_sector_valid = 1;
  } else if(FAT_0_origin_sector_for_cluster_num == FAT_in_mem_0_origin_sector) {
    /* FAT sector we need is currently in memory; just return needed entry */
    if(FAT_DEBUG)
      CONSOLE_PUTS("Correct FAT sector is currently in memory\n");
  } else {
    /* A FAT sector is in memory, but not the one we need; read the needed
       sector */
    if(FAT_DEBUG)
      CONSOLE_PUTS("Incorrect FAT sector is currently in memory\n");
    read_FAT(rca, (uint8_t *)FAT_sector, FAT_sector_to_access);
    FAT_in_mem_0_origin_sector = FAT_0_origin_sector_for_cluster_num;
  }

  if(FAT_DEBUG) {
    snprintf(output_buffer, FAT_OUTPUT_BUFFER_SIZE,
	     "The previous FAT entry for cluster %lu is %lu (0x%08lX)\n", cluster,
	     FAT_sector[FAT_entry_offset_in_sector],
	     FAT_sector[FAT_entry_offset_in_sector]);
    CONSOLE_PUTS(output_buffer);
  }

  /* change the selected FAT entry to be nextCluster */
  /* we're allowed to change only the low-order 28 bits; the
     high-order 4 bits must maintain their previous value */
  FAT_sector[FAT_entry_offset_in_sector] =
    (FAT_sector[FAT_entry_offset_in_sector] & ~FAT_ENTRY_MASK) |
    (nextCluster & FAT_ENTRY_MASK);
  /* write the modified FAT sector to the main FAT in the file system */
  write_FAT(rca, (uint8_t *)FAT_sector, FAT_sector_to_access);
  /* write the modified FAT sector to the copy FAT in the file system */
  write_FAT(rca, (uint8_t *)FAT_sector+sectors_per_FAT, FAT_sector_to_access);
  return;
}

void invalidate_FAT_by_sector(uint32_t sector_address) {
  if(sector_address == (FAT_in_mem_0_origin_sector+first_FAT_sector)) {
    FAT_sector_valid = 0;
  }
}

static void read_FAT(uint32_t rca, uint8_t *data, uint32_t sector) {
  char output_buffer[FAT_OUTPUT_BUFFER_SIZE];

  struct sdhc_card_status card_status;

  if(FAT_DEBUG) {
    snprintf(output_buffer, FAT_OUTPUT_BUFFER_SIZE,
	     "Reading FAT sector %lu\n", sector);
    CONSOLE_PUTS(output_buffer);
  }

  if(!FAT_endiannessDetermined) {
    determineEndianness();
    FAT_endiannessDetermined = 1;
  }

  /* computer is little endian, we can read directly into memory */
  if(SDHC_SUCCESS !=
     sdhc_read_single_block(rca, sector, &card_status, data)) {
    __BKPT();
  }
  if(FAT_endian == endian_big) {
    /* if computer is big endian, we need to reverse the bytes in each
       32-bit word */
    int i;
    uint8_t MSByte, NextToMSByte;

    for(i = 0; i < 512; i += 4) {
      MSByte = data[i];
      NextToMSByte = data[i+1];
      data[i] = data[i+3];
      data[i+1] = data[i+2];
      data[i+2] = NextToMSByte;
      data[i+3] = MSByte;
    }
  }
}

static void write_FAT(uint32_t rca, uint8_t *data, uint32_t sector) {
  char output_buffer[FAT_OUTPUT_BUFFER_SIZE];

  struct sdhc_card_status card_status;

  if(FAT_DEBUG) {
    snprintf(output_buffer, FAT_OUTPUT_BUFFER_SIZE,
	     "Writing FAT sector %lu\n", sector);
    CONSOLE_PUTS(output_buffer);
  }

  if(!FAT_endiannessDetermined) {
    determineEndianness();
    FAT_endiannessDetermined = 1;
  }

  if(FAT_endian == endian_big) {
    /* if computer is big endian, we need to reverse the bytes in each
       32-bit word before writing it to the FAT file structure */
    int i;
    uint8_t MSByte, NextToMSByte;

    for(i = 0; i < 512; i += 4) {
      MSByte = data[i];
      NextToMSByte = data[i+1];
      data[i] = data[i+3];
      data[i+1] = data[i+2];
      data[i+2] = NextToMSByte;
      data[i+3] = MSByte;
    }
  }

  /* now that the FAT sector is in little endian byte order, we can
     write directly from memory */
  if(SDHC_SUCCESS !=
     sdhc_write_single_block(rca, sector, &card_status, data)) {
    __BKPT();
  }

  if(FAT_endian == endian_big) {
    /* unfortunately, if computer is big endian, we now need to
       re-reverse the bytes in each 32-bit word */
    int i;
    uint8_t MSByte, NextToMSByte;

    for(i = 0; i < 512; i += 4) {
      MSByte = data[i];
      NextToMSByte = data[i+1];
      data[i] = data[i+3];
      data[i+1] = data[i+2];
      data[i+2] = NextToMSByte;
      data[i+3] = MSByte;
    }
  }
}

int FAT_copy_verify(uint32_t rca) {
  char output_buffer[FAT_OUTPUT_BUFFER_SIZE];

  uint32_t main_FAT_sector_number, copy_FAT_sector_number, i;
  struct sdhc_card_status card_status;
  uint8_t main_FAT_data[512], copy_FAT_data[512];
  int compare_equal;

  compare_equal = 1;
  main_FAT_sector_number = first_FAT_sector;
  copy_FAT_sector_number = first_FAT_sector+sectors_per_FAT;
  for(i = 0; i < sectors_per_FAT; i++) {
    if(SDHC_SUCCESS !=
       sdhc_read_single_block(rca, main_FAT_sector_number,
			      &card_status,
			      main_FAT_data)) {
      __BKPT();
    }

    if(SDHC_SUCCESS !=
       sdhc_read_single_block(rca, copy_FAT_sector_number,
			      &card_status,
			      copy_FAT_data)) {
      __BKPT();
    }

    if(memcmp(main_FAT_data, copy_FAT_data, bytes_per_sector) != 0) {
      compare_equal = 0;
      if(FAT_INFORMATIVE_PRINTF) {
	snprintf(output_buffer, FAT_OUTPUT_BUFFER_SIZE,
		 "Sector #%lu of main FAT and sector #%lu of copy FAT are not the same\n", main_FAT_sector_number, copy_FAT_sector_number);
	CONSOLE_PUTS(output_buffer);
      }
    }
    main_FAT_sector_number++;
    copy_FAT_sector_number++;
  }

  return compare_equal;
}
