/*
 * fsInfo.c
 * FAT32 FSInfo Implementation
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021 James L. Frankel.  All rights reserved.
 *
 * Last updated: 9:32 PM 8-Mar-2021
 */

#include <stdio.h>

#include "microSD.h"
#include "bootSector.h"
#include "FAT.h"
#include "fsInfo.h"
#include "breakpoint.h"

uint32_t FSI_Free_Count;
uint32_t FSI_Nxt_Free;

void FSInfo_sector_read(uint32_t rca, uint32_t block_address) {
  char output_buffer[FSI_OUTPUT_BUFFER_SIZE];

  struct sdhc_card_status card_status;
  uint8_t data[512];

  struct FSInfo_sector *FSInfo_sector_p;

  uint32_t uint32;

  if(SDHC_SUCCESS !=
     sdhc_read_single_block(rca, block_address, &card_status, data)) {
    __BKPT();
  }

  FSInfo_sector_p = (struct FSInfo_sector *)data;

  uint32 = LITTLE_ENDIAN_4_BYTES_TO_UINT32(FSInfo_sector_p->FSI_LeadSig);
  if(FSI_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Lead Signature (FSI_LeadSig): 0x%08lX\n", uint32);
    CONSOLE_PUTS(output_buffer);
  }
  if(uint32 != FSI_LEADSIG_REQDVAL) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "*****The Lead Signature is not valid, should be 0x%08X*****\n",
	     FSI_LEADSIG_REQDVAL);
    CONSOLE_PUTS(output_buffer);
  }

  int i, allZero;
  allZero = 1;
  for(i = 0; i <480; i++) {
    if(FSInfo_sector_p->FSI_Reserved1[i] != 0) {
      allZero = 0;
      break;
    }
  }
  if(allZero) {
    if(FSI_DEBUG)
      CONSOLE_PUTS("The FSI_Reserved1 field is all zero as required\n");
  } else {
    CONSOLE_PUTS("*****The FSI_Reserved1 field is not all zeros, this is incorrect*****\n");
  }

  uint32 = LITTLE_ENDIAN_4_BYTES_TO_UINT32(FSInfo_sector_p->FSI_StrucSig);
  if(FSI_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Struc Signature (FSI_StrucSig): 0x%08lX\n", uint32);
    CONSOLE_PUTS(output_buffer);
  }
  if(uint32 != FSI_STRUCSIG_REQDVAL) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "*****The Struc Signature is not valid, should be 0x%08X*****\n",
	     FSI_STRUCSIG_REQDVAL);
    CONSOLE_PUTS(output_buffer);
  }

  FSI_Free_Count =
    LITTLE_ENDIAN_4_BYTES_TO_UINT32(FSInfo_sector_p->FSI_Free_Count);
  if(FSI_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Last known free cluster count on the volume (FSI_Free_Count): %lu (0x%08lX)\n",
	     FSI_Free_Count, FSI_Free_Count);
    CONSOLE_PUTS(output_buffer);
  }
  if(FSI_Free_Count == FSI_FREE_COUNT_UNKNOWN) {
    if(FSI_DEBUG)
      CONSOLE_PUTS("The Last known free cluster count on the volume is unknown\n");
  } else if(FSI_Free_Count > total_data_clusters) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "*****Last known free cluster count on the volume (FSI_Free_Count): %lu (0x%08lX)\n",
	     total_data_clusters, total_data_clusters);
    CONSOLE_PUTS(output_buffer);
    snprintf(output_buffer, sizeof(output_buffer),
	     "\texceeds the total number of data clusters on the volume: %lu (0x%08lX)*****\n",
	     total_data_clusters, total_data_clusters);
    CONSOLE_PUTS(output_buffer);
  }

  FSI_Nxt_Free = LITTLE_ENDIAN_4_BYTES_TO_UINT32(FSInfo_sector_p->FSI_Nxt_Free);
  if(FSI_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Next free cluster on the volume (FSI_Nxt_Free): %lu (0x%08lX)\n",
	     FSI_Nxt_Free, FSI_Nxt_Free);
    CONSOLE_PUTS(output_buffer);
  }
  if(FSI_Nxt_Free == FSI_NXT_FREE_UNKNOWN) {
    if(FSI_DEBUG)
      CONSOLE_PUTS("The next free cluster is unknown\n");
  }

  allZero = 1;
  for(i = 0; i <12; i++) {
    if(FSInfo_sector_p->FSI_Reserved2[i] != 0) {
      allZero = 0;
      break;
    }
  }
  if(allZero) {
    if(FSI_DEBUG)
      CONSOLE_PUTS("The FSI_Reserved2 field is all zero as required\n");
  } else {
    CONSOLE_PUTS("*****The FSI_Reserved2 field is not all zeros, this is incorrect*****\n");
  }

  uint32 = LITTLE_ENDIAN_4_BYTES_TO_UINT32(FSInfo_sector_p->FSI_TrailSig);
  if(FSI_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Trail Signature (FSI_TrailSig): 0x%08lX\n", uint32);
    CONSOLE_PUTS(output_buffer);
  }
  if(uint32 != FSI_TRAILSIG_REQDVAL) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "*****The Trail Signature is not valid, should be 0x%08X*****\n",
	     FSI_TRAILSIG_REQDVAL);
    CONSOLE_PUTS(output_buffer);
  }
}

void FSInfo_validate(uint32_t rca) {
  char output_buffer[FSI_OUTPUT_BUFFER_SIZE];

  uint32_t lowestFreeCluster = 0xffffffff;
  uint32_t countOfFreeClusters = 0;
  uint32_t cluster;

  for(cluster = root_directory_cluster;
      cluster < (root_directory_cluster+total_data_clusters);
      cluster++) {
    if(read_FAT_entry(rca, cluster) == FAT_ENTRY_FREE) {
      if(lowestFreeCluster == 0xffffffff) {
	lowestFreeCluster = cluster;
      }
      countOfFreeClusters++;
    }
  }

  if(FSI_Free_Count == FSI_FREE_COUNT_UNKNOWN)
    CONSOLE_PUTS("Count of free clusters from the FSInfo sector: [Unknown]\n");
  else {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Count of free clusters from the FSInfo sector: %lu\n",
	     FSI_Free_Count);
    CONSOLE_PUTS(output_buffer);
  }

  snprintf(output_buffer, sizeof(output_buffer),
	   "Count of free clusters from examining the entire FAT: %lu\n",
	   countOfFreeClusters);
  CONSOLE_PUTS(output_buffer);

  if(FSI_Nxt_Free == FSI_NXT_FREE_UNKNOWN)
    CONSOLE_PUTS("Hint from FSInfo sector of first cluster for free cluster search: [Unknown]\n");
  else {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Hint from FSInfo sector of first cluster for free cluster search: %lu\n",
	     FSI_Nxt_Free);
    CONSOLE_PUTS(output_buffer);
  }

  if(lowestFreeCluster == 0xffffffff)
    CONSOLE_PUTS("No free clusters as determined by examining the FAT\n");
  else {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Lowest number free cluster from examining the FAT: %lu\n",
	     lowestFreeCluster);
    CONSOLE_PUTS(output_buffer);
  }
}
