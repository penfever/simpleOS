/*
 * directory.h
 * FAT32 Directory Display Header File
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021 James L. Frankel.  All rights reserved.
 */

#ifndef _DIRECTORY_H
#define _DIRECTORY_H

#include <stdint.h>

/* struct for a short directory entry */

/* Note: Using uint16_t and/or uint32_t types to map little endian
   hardware fields relies on the computer being little endian as well.
   This code will not work correctly on a big endian computer. */

struct dir_entry_8_3 {
  uint8_t DIR_Name[11];			/* Offset 0 */
  uint8_t DIR_Attr;			/* Offset 11 */
  uint8_t DIR_NTRes;			/* Offset 12 */
  uint8_t DIR_CrtTimeHundth;		/* Offset 13 */
  uint16_t DIR_CrtTime;			/* Offset 14 */
  uint16_t DIR_CrtDate;			/* Offset 16 */
  uint16_t DIR_LstAccDate;		/* Offset 18 */
  uint16_t DIR_FstClusHI;		/* Offset 20 */
  uint16_t DIR_WrtTime;			/* Offset 22 */
  uint16_t DIR_WrtDate;			/* Offset 24 */
  uint16_t DIR_FstClusLO;		/* Offset 26 */
  uint32_t DIR_FileSize;		/* Offset 28 */
};

#define MS_DOS_EPOCH 1980

/* struct for a long directory entry */

/* Note: Using uint16_t and/or uint32_t types to map little endian
   hardware fields relies on the computer being little endian as well.
   This code will not work correctly on a big endian computer. */

struct dir_entry_long {
  uint8_t LDIR_Ord;			/* Offset 0 */
  uint16_t LDIR_Name1[5];		/* Offset 1 */
  uint8_t LDIR_Attr;			/* Offset 11 */
  uint8_t LDIR_Type;			/* Offset 12 */
  uint8_t LDIR_Chksum;			/* Offset 13 */
  uint16_t LDIR_Name2[6];		/* Offset 14 */
  uint16_t LDIR_FstClusLO;		/* Offset 26 */
  uint16_t LDIR_Name3[2];		/* Offset 28 */
};

/* Number of UNICODE characters in a long directory entry */
#define DIR_ENTRY_LONG_FILE_NAME_CHARS_PER_ENTRY 13

/* Bit value that indicates a long directory's entry name is the rightmost
   (end) portion of that long name */
#define LDIR_ORD_NAME_END 0x40
/* Mask for the order number (1-origin) of a long directory's entry name */
#define LDIR_ORD_ORDER_MASK 0x3F

/* #define's for first byte (DIR_Name[0]/LDIR_Ord) in a directory entry */
#define DIR_ENTRY_LAST_AND_UNUSED 0x0
#define DIR_ENTRY_UNUSED 0xE5

/* #define's for the attribute byte (DIR_Attr/LDIR_Attr) */
#define DIR_ENTRY_ATTR_READ_ONLY 0x01
#define DIR_ENTRY_ATTR_HIDDEN 0x02
#define DIR_ENTRY_ATTR_SYSTEM 0x04
#define DIR_ENTRY_ATTR_VOLUME_ID 0x08
#define DIR_ENTRY_ATTR_DIRECTORY 0x10
#define DIR_ENTRY_ATTR_ARCHIVE 0x20

#define DIR_ENTRY_ATTR_LONG_NAME (DIR_ENTRY_ATTR_READ_ONLY |	\
DIR_ENTRY_ATTR_HIDDEN | \
DIR_ENTRY_ATTR_SYSTEM | \
DIR_ENTRY_ATTR_VOLUME_ID)

#define DIR_ENTRY_ATTR_LONG_NAME_MASK (DIR_ENTRY_ATTR_READ_ONLY |	\
DIR_ENTRY_ATTR_HIDDEN | \
DIR_ENTRY_ATTR_SYSTEM | \
DIR_ENTRY_ATTR_VOLUME_ID | \
DIR_ENTRY_ATTR_DIRECTORY | \
DIR_ENTRY_ATTR_ARCHIVE)

#endif /* ifndef _DIRECTORY_H */
