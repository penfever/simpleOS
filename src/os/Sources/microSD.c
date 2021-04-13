/*
 * microSD.c
 * NXP/Freescale K70 microSD Controller Implementation
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu) & Daniel Willenson
 *
 * Copyright (c) 2021 James L. Frankel.  All rights reserved.
 *
 * Last updated: 9:29 PM 8-Mar-2021
 */

#include <stdio.h>
#include <stdint.h>

#include "derivative.h"
#include "microSD.h"
#include "bootSector.h"
#include "fsInfo.h"
#include "FAT.h"
#include "breakpoint.h"

uint32_t file_structure_first_sector;

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

static enum sdhc_status sdhc_command(uint32_t xfertyp, uint32_t cmdarg);

static enum sdhc_status sdhc_no_response_command(uint8_t index,
						 uint32_t cmdarg);

static enum sdhc_status sdhc_response_48_command(uint8_t index,
                                          unsigned int check_response_index,
                                          unsigned int check_response_crc,
                                          uint32_t cmdarg,
                                          uint32_t *response);

static enum sdhc_status sdhc_response_136_command(uint8_t index,
					   unsigned int check_response_index,
                                           unsigned int check_response_crc,
                                           uint32_t cmdarg,
                                           uint32_t response[4]);

static enum sdhc_status sdhc_command_go_idle_state(void);

static enum sdhc_status sdhc_command_send_if_cond(void);

static enum sdhc_status sdhc_command_app_cmd(uint32_t rca);

static enum sdhc_status sdhc_command_send_op_cond(void);

static enum sdhc_status sdhc_command_send_set_clr_card_detect_disconnect(uint32_t rca);

static enum sdhc_status sdhc_init(void);

static enum sdhc_status sdhc_card_init(void);

static enum sdhc_status sdhc_command_all_send_cid(uint32_t cid[4]);

static enum sdhc_status sdhc_command_send_relative_address(uint32_t *rca);

static enum sdhc_status sdhc_command_send_csd(uint32_t rca,
					      uint32_t csd[4]);

static enum sdhc_status sdhc_command_select_card(uint32_t rca,
						 uint32_t *card_status);

static enum sdhc_status sdhc_command_send_status(uint32_t rca,
						 uint32_t *card_status);

/**
 * Routine to configure the microSD Card Detect switch & pull-down resistor in
 * the ARM.
 * This must be called before calling microSDCardDetectedUsingSwitch
 * or microSDCardDetectedUsingResistor.
 * 
 */
void microSDCardDetectConfig(void) {
  /* Enable the clock for PORTE using the SIM_SCGC5 register (System
   * Clock Gating Control Register 5) (See 12.2.13 on labeled page 347
   * (PDF page 354) of the K70 Sub-Family Reference Manual, Rev. 4, Oct
   * 2015; 12.2.13 on page 342 of the K70 Sub-Family Reference Manual,
   * Rev. 2, Dec 2011) */
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;

  /* According to the K70 Sub-Family Data Sheet, Document Number
     K70P256M120SF3, Rev. 7, 02/2018, Table 4 on page 15 (PDF page
     15), the internal pullup resistors, R(PU) = min. 20k ohms &
     max. 50k ohms and the internal pulldown resistors, R(PD) =
     min. 20k ohms & max. 50k ohms. */
  
  /* Configure bit MICRO_SD_CARD_DETECT_SWITCH_PORTE_BIT of PORTE
   * using the Pin Control Register (PORTE_PCR) to be a GPIO pin.  This
   * sets the MUX field (Pin Mux Control) to GPIO mode (Alternative 1).
   * Also, by setting the PE bit, enable the internal pull-up or
   * pull-down resistor.  And, by setting the PS bit, enable the
   * internal pull-up resistor -- not the pull-down resistor.  (See
   * 11.5.1 on labeled page 316 (PDF page 323) of the K70 Sub-Family
   * Reference Manual, Rev. 4, Oct 2015; See 11.4.1 on page 309 of the
   * K70 Sub-Family Reference Manual, Rev. 2, Dec 2011) */
  PORT_PCR_REG(PORTE_BASE_PTR, MICRO_SD_CARD_DETECT_SWITCH_PORTE_BIT) =
    PORT_PCR_MUX(PORT_PCR_MUX_GPIO) |
    PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

  /* Configure bit MICRO_SD_CARD_DETECT_RESISTOR_PORTE_BIT of PORTE
   * using the Pin Control Register (PORTE_PCR) to be a GPIO pin.  This
   * sets the MUX field (Pin Mux Control) to GPIO mode (Alternative 1).
   * Also, by setting the PE bit, enable the internal pull-up or
   * pull-down resistor.  And, by not setting the PS bit, enable the
   * internal pull-down resistor -- not the pull-up resistor.  (See
   * 11.5.1 on labeled page 316 (PDF page 323) of the K70 Sub-Family
   * Reference Manual, Rev. 4, Oct 2015; See 11.4.1 on page 309 of the
   * K70 Sub-Family Reference Manual, Rev. 2, Dec 2011) */
  PORT_PCR_REG(PORTE_BASE_PTR, MICRO_SD_CARD_DETECT_RESISTOR_PORTE_BIT) =
    PORT_PCR_MUX(PORT_PCR_MUX_GPIO) | PORT_PCR_PE_MASK;

  /* By not setting the Port Data Direction Register (GPIOx_PDDR) to
   * be a GPIO output, we have configured bit
   * MICRO_SD_CARD_DETECT_SWITCH_PORTE_BIT of PORTE to be a GPIO input.
   * (See 59.2.6 on labeled page 2150 (PDF page 2157) of the K70
   * Sub-Family Reference Manual, Rev. 4, Oct 2015; 60.2.6 on page 2155
   * of the K70 Sub-Family Reference Manual, Rev. 2, Dec 2011) */
}

/**
 * Routine to disable the micro SD Card Detect ARM pull-down resistor
 *
 */
/* Routine to disable the microSD Card Detect ARM internal pull-down
   resistor */
/* This must be called after conducting SD card detection
   using the SD card internal resistor and before initializing the SD
   card interface */
void microSDCardDisableCardDetectARMPullDownResistor(void) {
  /* Configure bit MICRO_SD_CARD_DETECT_RESISTOR_PORTE_BIT of PORTE
   * using the Pin Control Register (PORTE_PCR) to be a GPIO pin.  This
   * sets the MUX field (Pin Mux Control) to GPIO mode (Alternative 1).
   * Also, by not setting the PE bit, disable the internal pull-up or
   * pull-down resistor.  (See 11.5.1 on labeled page 316 (PDF page 323)
   * of the K70 Sub-Family Reference Manual, Rev. 4, Oct 2015; See
   * 11.4.1 on page 309 of the K70 Sub-Family Reference Manual, Rev. 2,
   * Dec 2011) */
  PORT_PCR_REG(PORTE_BASE_PTR, MICRO_SD_CARD_DETECT_RESISTOR_PORTE_BIT) =
    PORT_PCR_MUX(PORT_PCR_MUX_GPIO);
}

/* There are two ways to detect presence of a micro SD card: (1)
   there is a mechanical switch in the card holder which is open when
   a card is not present and closed to ground when a card is present,
   and (2) there is a pull-up resistor integrated into the SD card
   that is connected to the DAT3/CD pin.  (See NXP Application Note
   AN10911: SD(HC)-memory card and MMC interface conditioning).)  Note
   that the MMC specification does not specify any internal pull-up
   resistor for an electrical card detection mechanism.  So, if MMC
   cards are to be used, the second approach cannot be used.

   After power-up DAT3/CD is connected to a 50k Ohm (nominal value,
   specified range is 10k Ohm to 90k Ohm) pull-up resistor inside the
   card.  In case DAT3/CD is connected to a high-ohmic pull-down
   resistor, the connected host can detect a logic level change from
   low to high level.  It is recommended that pull-down resistor is
   >270k Ohm to fulfill the logic voltage level requirements.  The
   exact value depends on the logic level requirements.  The card
   internal pull-up resistor should be disconnected during regular
   data transmission with SET_CLR_CARD_DETECT (ACMD42) command. */

/* Routine to read the state of the micro SD Card Detect switch  */
/* Return true if the card is detected and false otherwise */
int microSDCardDetectedUsingSwitch(void) {
  /* Returns 1 when a micro SD card is detected and 0 otherwise */
  int microSDCardDetectState;
	
  /* Read the state of bit MICRO_SD_CARD_DETECT_SWITCH_PORTE_BIT of
   * PORTE */
  microSDCardDetectState = PTE_BASE_PTR->PDIR &
    (1 << MICRO_SD_CARD_DETECT_SWITCH_PORTE_BIT);
  return !microSDCardDetectState;
}

/* Routine to read the state of the micro SD Card Detect using the 50k
   Ohm (nominal value, specified range is 10k Ohm to 90k Ohm) pull-up
   resistor inside the card.  The 50k Ohm resistor is connected
   between the DAT3/CD pin and the Vcc/Vsd pin on power-up.  The
   card's internal pull-up resistor should be disconnected during
   regular data transmission with SET_CLR_CARD_DETECT (ACMD42)
   command.  It is recommended that pull-down resistor is >270k Ohm to
   fulfill the logic voltage level requirements. */
/* The internal resistor is enabled when a card is inserted into the microSD
   slot, when new code is loaded into the K70, and when explicitly enabled by
   calling sdhc_command_send_set_clr_card_detect_connect */
/* Return true if the card is detected and false otherwise */
int microSDCardDetectedUsingResistor(void) {
  /* Returns 1 when a micro SD card is detected and 0 otherwise */
  int microSDCardDetectState;
	
  /* Read the state of bit MICRO_SD_CARD_DETECT_RESISTOR_PORTE_BIT of
   * PORTE */
  microSDCardDetectState = PTE_BASE_PTR->PDIR &
    (1 << MICRO_SD_CARD_DETECT_RESISTOR_PORTE_BIT);
  return !!microSDCardDetectState;
}

/* Initializes the SDHC interface */
/* A microSDHC card must be present in the TWR-K70F120M microSD card slot
   before calling this function */
/* This must be called before calling any of the supplied functions except for
   microSDCardDetectConfig, microSDCardDetectedUsingSwitch,
   microSDCardDetectedUsingResistor, and
   microSDCardDisableCardDetectARMPullDownResistor */
/* This function also disconnects the 50k Ohm pull-up resistor inside the SD
   card */
/* Returns the rca (Relative Card Address) */
/* After calling sdhc_initialize, all variables declared in the furnished
   header files are appropriately initialized */
uint32_t sdhc_initialize(void) {
  enum sdhc_status status;
  struct sdhc_card_specific_data csd;
  struct sdhc_card_status card_status;
  struct sdhc_card_id cid;
  uint32_t rca;
  char output_buffer[MICRO_SD_OUTPUT_BUFFER_SIZE];

  /* cause all sector reads and writes to initially assume that this disk
     is not partitioned by setting the file_structure_first_sector to be 0 */
  /* if the disk is partitioned, then set the first sector of the disk's FAT32
     file structure to be the first sector LBA (Logical Block Address) from
     the partition table -- this happens in boot_sector_read */

  file_structure_first_sector = 0;

  /* SD Memory Card State: Idle State */

  if(SDHC_SUCCESS != (status = sdhc_init())) {
    __BKPT();
  }

  if(SDHC_SUCCESS != (status = sdhc_card_init())) {
    __BKPT();
  }

  /* SD Memory Card State: Ready State */

  if(SDHC_SUCCESS != (status = sdhc_command_all_send_cid((void *)&cid))) {
    __BKPT();
  }

  /* SD Memory Card State: Identification State */

  if(MICRO_SD_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "OEM ID: %c%c\n", cid.oem_id[1], cid.oem_id[0]);
    CONSOLE_PUTS(output_buffer);
    snprintf(output_buffer, sizeof(output_buffer),
	     "Product Name: %c%c%c%c%c\n", cid.product_name[4],
	     cid.product_name[3], cid.product_name[2], cid.product_name[1],
	     cid.product_name[0]);
    CONSOLE_PUTS(output_buffer);
    snprintf(output_buffer, sizeof(output_buffer),
	     "Product Revision: %d.%d\n", cid.product_revision_major,
	     cid.product_revision_minor);
    CONSOLE_PUTS(output_buffer);
    snprintf(output_buffer, sizeof(output_buffer),
	     "Product Serial Number: 0x%04X%04X\n", cid.product_serial_high,
	     cid.product_serial_low);
    CONSOLE_PUTS(output_buffer);
    snprintf(output_buffer, sizeof(output_buffer),
	     "Manufacturing Date: %04d-%02d\n", 2000 + cid.manufacture_year,
	     cid.manufacture_month);
    CONSOLE_PUTS(output_buffer);
  }

  if(SDHC_SUCCESS != (status = sdhc_command_send_relative_address(&rca))) {
    __BKPT();
  }

  /* SD Memory Card State: Stand-by State */

  if(MICRO_SD_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "Relative Card Address (rca): 0x%08lX\n", rca);
    CONSOLE_PUTS(output_buffer);
  }

  if(SDHC_SUCCESS != (status = sdhc_command_send_csd(rca, (void *)&csd))) {
    __BKPT();
  }

  if(MICRO_SD_DEBUG) {
    snprintf(output_buffer, sizeof(output_buffer),
	     "CSD Version: %u.0\n", csd.csd_structure+1);
    CONSOLE_PUTS(output_buffer);
  }
  if(csd.csd_structure+1 == 2) {
    /* This is CSD (Card-Specific Data) Version 2.0 */
    /* Version 2.0 supports High Capacity (SDHC) and Extended Capacity (SDXC) */
    if(MICRO_SD_DEBUG) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "Erasable block enable: %s\n", csd.v2_0.erase_blk_en ? "true" :
	       "false");
      CONSOLE_PUTS(output_buffer);
      snprintf(output_buffer, sizeof(output_buffer),
	       "Number of erasable write blocks: %u blocks\n",
	       ((csd.v2_0.sector_size_high << 1) | csd.sector_size_low) + 1);
      CONSOLE_PUTS(output_buffer);
      snprintf(output_buffer, sizeof(output_buffer),
	       "Maximum write block length: %u bytes\n",
	       1 << csd.write_bl_len);
      CONSOLE_PUTS(output_buffer);
      snprintf(output_buffer, sizeof(output_buffer),
	       "Maximum read block length: %u bytes\n",
	       1 << csd.v2_0.read_bl_len);
      CONSOLE_PUTS(output_buffer);
      snprintf(output_buffer, sizeof(output_buffer),
	       "Card size: %uKi (512 byte) blocks = %uKiB\n", csd.v2_0.c_size+1,
	       (csd.v2_0.c_size+1) << 9);
      CONSOLE_PUTS(output_buffer);
      snprintf(output_buffer, sizeof(output_buffer),
	       "Card contents copied: %s\n", csd.copy ? "true" : "false");
      CONSOLE_PUTS(output_buffer);
      snprintf(output_buffer, sizeof(output_buffer),
	       "Permanent write protect: %s\n", csd.perm_write_protect ?
	       "true" : "false");
      CONSOLE_PUTS(output_buffer);
      snprintf(output_buffer, sizeof(output_buffer),
	       "Temporary write protect: %s\n", csd.tmp_write_protect ?
	       "true" : "false");
      CONSOLE_PUTS(output_buffer);
    }
  }

  if(SDHC_SUCCESS !=
     (status = sdhc_command_select_card(rca, (void *)&card_status))) {
    __BKPT();
  }

  if(SDHC_SUCCESS !=
     (status = sdhc_command_send_status(rca, (void *)&card_status))) {
    __BKPT();
  }

  if(SDHC_SUCCESS !=
     (status = sdhc_command_send_set_clr_card_detect_disconnect(rca))) {
    __BKPT();
  }

  boot_sector_read(rca);

  FSInfo_sector_read(rca, FSInfo_sector_number);

  uint32_t LSBbyteMask = 0xff;

  /* the low-order byte of FAT[0] must be the same as the BPB_Media in the
     boot sector */
  if((read_FAT_entry(rca, 0) & LSBbyteMask) != BPB_Media) {
    if(MICRO_SD_INFORMATIVE_PRINTF) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "*****The LSB of FAT[0] is not equal to BPB_Media from the boot sector*****\n");
      CONSOLE_PUTS(output_buffer);
    }
  }
  /* the high-order three bytes of FAT[0] must be 0xfffff */
  if(((read_FAT_entry(rca, 0) & ~LSBbyteMask) >> 8) != 0xfffff) {
    if(MICRO_SD_INFORMATIVE_PRINTF) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "*****The high-order three bytes of FAT[0] are not equal to 0xFFFFF*****\n");
      CONSOLE_PUTS(output_buffer);
    }
  }

  /* the low-order 28 bits of FAT[1] must be set to be the same as an
     EOC (End Of Clusterchain) mark (the EOC is anything >=
     FAT_ENTRY_RESERVED_TO_END) */
  if(read_FAT_entry(rca, 1) < FAT_ENTRY_RESERVED_TO_END) {
    if(MICRO_SD_INFORMATIVE_PRINTF) {
      snprintf(output_buffer, sizeof(output_buffer),
	       "*****The low-order 28 bits of FAT[1] are not equal to the EOC*****\n");
      CONSOLE_PUTS(output_buffer);
    }
  }

  return rca;
}

static enum sdhc_status sdhc_command(uint32_t xfertyp, uint32_t cmdarg) {
  /* Clear the command complete flag */
  SDHC_IRQSTAT = SDHC_IRQSTAT_CC_MASK;

  SDHC_CMDARG = cmdarg;

  uint32_t prsstat;
  while(((prsstat = SDHC_PRSSTAT) & SDHC_PRSSTAT_RTA_MASK) ||
	(prsstat & SDHC_PRSSTAT_DLA_MASK)) {
  }

  /* Check the Command Inhibit (CMD) status bit */
  if((prsstat = SDHC_PRSSTAT) & SDHC_PRSSTAT_CIHB_MASK) {
    __BKPT();
  }

  /* Check the Command Inhibit (DAT) status bit */
  if((prsstat = SDHC_PRSSTAT) & SDHC_PRSSTAT_CDIHB_MASK) {
    __BKPT();
  }

  SDHC_XFERTYP = xfertyp;

  while(0 == (SDHC_IRQSTAT & SDHC_IRQSTAT_CTOE_MASK) &&
	0 == (SDHC_IRQSTAT & SDHC_IRQSTAT_CC_MASK)) {
    /* Wait for command complete */
  }

  /* Save and clear IRQ status register */
  uint32_t status = SDHC_IRQSTAT;
  /* For the SDHC_IRQSTAT register, writing a 1 to a bit clears that
     bit; writing 0 to a bit has no effect */
  /* Clear the Command Index Error (CIE), Command End Bit Error
     (CEBE), Command Timeout Error (CTOE), Command CRC Error (CCE),
     and Command Complete (CC) bits */
  SDHC_IRQSTAT = SDHC_IRQSTAT_CIE_MASK |
    SDHC_IRQSTAT_CEBE_MASK |
    SDHC_IRQSTAT_CCE_MASK |
    SDHC_IRQSTAT_CTOE_MASK |
    SDHC_IRQSTAT_CC_MASK;

  /* Decode and return status */
  if(0 != (status & SDHC_IRQSTAT_DEBE_MASK)) {
    return SDHC_COMMAND_ERROR_DATA_END_BIT;

  } else if(0 != (status & SDHC_IRQSTAT_DCE_MASK)) {
    return SDHC_COMMAND_ERROR_DATA_CRC;

  } else if(0 != (status & SDHC_IRQSTAT_DTOE_MASK)) {
    return SDHC_COMMAND_ERROR_DATA_TIMEOUT;

  } else if(0 != (status & SDHC_IRQSTAT_CIE_MASK)) {
    return SDHC_COMMAND_ERROR_COMMAND_INDEX;

  } else if(0 != (status & SDHC_IRQSTAT_CEBE_MASK)) {
    return SDHC_COMMAND_ERROR_COMMAND_END_BIT;

  } else if(0 != (status & SDHC_IRQSTAT_CCE_MASK)) {
    return SDHC_COMMAND_ERROR_COMMAND_CRC;

  } else if(0 != (status & SDHC_IRQSTAT_CTOE_MASK)) {
    return SDHC_COMMAND_ERROR_COMMAND_TIMEOUT;

  } else {
    return SDHC_SUCCESS;
  }
}

static enum sdhc_status sdhc_no_response_command(uint8_t index,
						 uint32_t cmdarg) {
  return sdhc_command(SDHC_XFERTYP_CMDINX(index), cmdarg);
}

static enum sdhc_status sdhc_response_48_command(uint8_t index,
                                          unsigned int check_response_index,
                                          unsigned int check_response_crc,
                                          uint32_t cmdarg,
                                          uint32_t *response) {
  uint32_t xfertyp = SDHC_XFERTYP_CMDINX(index) |
    (check_response_index ? SDHC_XFERTYP_CICEN_MASK : 0) |
    (check_response_crc ? SDHC_XFERTYP_CCCEN_MASK : 0) |
    SDHC_XFERTYP_RSPTYP(2);

  enum sdhc_status status = sdhc_command(xfertyp, cmdarg);
  /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-13 on printed
     page #2011, PDF page 2018) shows that for response types R1, R1b
     (normal response), R3, R4, R5, R5b, and R6, that bits 39:8 of the
     response field are stored in response register CMDRSP0. */
  if(SDHC_SUCCESS == status) {
    *response = SDHC_CMDRSP0;
  }
  return status;
}

static enum sdhc_status sdhc_response_136_command(uint8_t index,
					   unsigned int check_response_index,
                                           unsigned int check_response_crc,
                                           uint32_t cmdarg,
                                           uint32_t response[4]) {
  uint32_t xfertyp = SDHC_XFERTYP_CMDINX(index) |
    (check_response_index ? SDHC_XFERTYP_CICEN_MASK : 0) |
    (check_response_crc ? SDHC_XFERTYP_CCCEN_MASK : 0) |
    SDHC_XFERTYP_RSPTYP(1);

  enum sdhc_status status = sdhc_command(xfertyp, cmdarg);
  /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-13 on printed
     page #2011, PDF page 2018) shows that for response type R2, that
     bits 127:8 of the response field are stored in response registers
     CMDRSP3[23:0], CMDRSP2, CMDRSP1, and CMDRSP0. */
  if(SDHC_SUCCESS == status) {
    response[0] = SDHC_CMDRSP0;
    response[1] = SDHC_CMDRSP1;
    response[2] = SDHC_CMDRSP2;
    response[3] = SDHC_CMDRSP3;
  }
  return status;
}

static enum sdhc_status sdhc_command_go_idle_state(void) {
  return sdhc_no_response_command(SD_COMMAND_GO_IDLE_STATE_CMD0, 0);
}

static enum sdhc_status sdhc_command_send_if_cond(void) {
  enum sdhc_status status;
  uint32_t response;

  /* SD Physical Specification says that CMD8 has an R7 response.  See
     Part1 Physical Layer Simplified Specification, V8.00 (Tables 4-23
     through 4-30, printed pages 97-102, PDF pages 117-122). */
  /* The K70 manual does not have an entry for R7 responses in Table
     57-8 */
  /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-8 on printed
     page #2006, PDF page 2013) says that R6 requires Response type
     (RSPTYP) 2 Index check enable (CICEN) 1 and CRC check enable
     (CCCEN) 1; All sdhc_response_48_commands use RSPTYP(2).  The two
     true arguments set CICEN and CCCEN. */
  /* As shown in the schematics for the TWR-K70F120M, we supply 3.3V to
     the microSD card */
  status = sdhc_response_48_command(SD_COMMAND_SEND_IF_COND_CMD8,
				    true, true,
				    SD_CMD8_IF_COND_ARG_VHS_2_7_TO_3_6 |
				    SD_CMD8_IF_COND_ARG_CHECK_PATTERN,
				    &response);

 if(SDHC_SUCCESS == status) {
   if((response & SD_CMD8_IF_COND_ARG_VHS_MASK) !=
      SD_CMD8_IF_COND_ARG_VHS_2_7_TO_3_6) {
     return SDHC_VOLTAGE_MISMATCH;
   } else if((response & SD_CMD8_IF_COND_ARG_CHECK_PATTERN_MASK) !=
	     SD_CMD8_IF_COND_ARG_CHECK_PATTERN) {
     return SDHC_CHECK_PATTERN_MISMATCH;
   }
 }
 return status;
}

static enum sdhc_status sdhc_command_app_cmd(uint32_t rca) {
  const uint32_t CARD_STATUS_READY_FOR_DATA = (0x1 << 8);
  const uint32_t CARD_STATUS_APP_CMD = (0x1 << 5);
  uint32_t response;
  enum sdhc_status status;

  /* SD Physical Specification says that CMD55 has an R1 response.
     See Part1 Physical Layer Simplified Specification, V8.00 (Tables
     4-23 through 4-30, printed pages 97-102, PDF pages 117-122). */
  /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-8 on printed
     page #2006, PDF page 2013) says that R1 requires Response type
     (RSPTYP) 2 Index check enable (CICEN) 1 and CRC check enable
     (CCCEN) 1; All sdhc_response_48_commands use RSPTYP(2).  The two
     true arguments set CICEN and CCCEN. */
  status = sdhc_response_48_command(SD_COMMAND_APP_CMD_CMD55, true, true, rca,
				    &response);

  if(SDHC_SUCCESS == status && ((response & ~CARD_STATUS_CURRENT_STATE_MASK) !=
	      (CARD_STATUS_READY_FOR_DATA | CARD_STATUS_APP_CMD))) {
    return SDHC_REJECTED_APP_CMD;

  } else {
    return status;
  }
}

static enum sdhc_status sdhc_command_send_op_cond(void) {
  enum sdhc_status status;
  uint32_t response;
  do {
    status = sdhc_command_app_cmd(0);
    if(SDHC_SUCCESS == status) {

  /* SD Physical Specification says that ACMD41 has an R3 response.
     See Part1 Physical Layer Simplified Specification, V8.00 (Table
     4-31, printed pages 102-104, PDF pages 122-124). */
  /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-8 on printed
     page #2006, PDF page 2013) says that R3 requires Response type
     (RSPTYP) 2 Index check enable (CICEN) 0 and CRC check enable
     (CCCEN) 0; All sdhc_response_48_commands use RSPTYP(2).  The
     two false arguments set CICEN and CCCEN to 0. */
      status = sdhc_response_48_command(SD_APP_COMMAND_SD_SEND_OP_COND_ACMD41,
			false, false,
			SD_ACMD41_OP_COND_ARG_HCS |
			SD_ACMD41_OP_COND_ARG_XPC |
			SD_ACMD41_OP_COND_ARG_OCR_VDD_VOLTAGE_WINDOW_3_2_TO_3_4,
			&response);
    }
  /* SD Physical Specification says that ACMD41 has an R3 response.
     See Part1 Physical Layer Simplified Specification, V8.00 (Table
     4-4, printed page 43, PDF pages 63). */
#define ACMD41_BUSY 0x80000000
  } while(SDHC_SUCCESS == status && 0 == (response & ACMD41_BUSY));
  return status;
}

/* Routine to disconnect the 50k Ohm (nominal value, specified range
   is 10k Ohm to 90k Ohm) pull-up resistor inside the SD card */
enum sdhc_status sdhc_command_send_set_clr_card_detect_disconnect(uint32_t rca) {
  enum sdhc_status status;
  uint32_t response;

  status = sdhc_command_app_cmd(rca);
  if(SDHC_SUCCESS == status) {
    status =
  /* SD Physical Specification says that ACMD42 has an R1 response.
     See Part1 Physical Layer Simplified Specification, V8.00 (Table
     4-31, printed pages 102-104, PDF pages 122-124). */
  /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-8 on printed
     page #2006, PDF page 2013) says that R1 requires Response type
     (RSPTYP) 2 Index check enable (CICEN) 1 and CRC check enable
     (CCCEN) 1; All sdhc_response_48_commands use RSPTYP(2).  The two
     true arguments set CICEN and CCCEN. */
    sdhc_response_48_command(SD_APP_COMMAND_SET_CLR_CARD_DETECT_ACMD42,
			     true, true, 0, &response);
      /*
	sdhc_response_48_command(SD_APP_COMMAND_SET_CLR_CARD_DETECT_ACMD42,
	false, false, 0, &response);
      */
  }
  return status;
}

/* Routine to connect the 50k Ohm (nominal value, specified range is
   10k Ohm to 90k Ohm) pull-up resistor inside the SD card */
enum sdhc_status sdhc_command_send_set_clr_card_detect_connect(uint32_t rca) {
  enum sdhc_status status;
  uint32_t response;

  status = sdhc_command_app_cmd(rca);
  if(SDHC_SUCCESS == status) {
    status =
  /* SD Physical Specification says that ACMD42 has an R1 response.
     See Part1 Physical Layer Simplified Specification, V8.00 (Table
     4-31, printed pages 102-104, PDF pages 122-124). */
  /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-8 on printed
     page #2006, PDF page 2013) says that R1 requires Response type
     (RSPTYP) 2 Index check enable (CICEN) 1 and CRC check enable
     (CCCEN) 1; All sdhc_response_48_commands use RSPTYP(2).  The two
     true arguments set CICEN and CCCEN. */
    sdhc_response_48_command(SD_APP_COMMAND_SET_CLR_CARD_DETECT_ACMD42,
			     true, true, 1, &response);
      /*
	sdhc_response_48_command(SD_APP_COMMAND_SET_CLR_CARD_DETECT_ACMD42,
	false, false, 0, &response);
      */
  }
  return status;
}

static enum sdhc_status sdhc_init(void) {
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;

  /* SDHC0_D1: ALT4, high drive strength, pull up enabled */
  PORTE_PCR0 = PORT_PCR_MUX(4) | PORT_PCR_DSE_MASK | PORT_PCR_PE_MASK |
    PORT_PCR_PS_MASK;

  /* SDHC0_D0: ALT4, high drive strength, pull up enabled */
  PORTE_PCR1 = PORT_PCR_MUX(4) | PORT_PCR_DSE_MASK | PORT_PCR_PE_MASK |
    PORT_PCR_PS_MASK;

  /* SDHC0_DCLK: ALT4, high drive strength */
  PORTE_PCR2 = PORT_PCR_MUX(4) | PORT_PCR_DSE_MASK;

  /* SDHC0_CMD: ALT4, high drive strength */
  PORTE_PCR3 = PORT_PCR_MUX(4) | PORT_PCR_DSE_MASK;

  /* SDHC0_D3: ALT4, high drive strength, pull up enabled */
  PORTE_PCR4 = PORT_PCR_MUX(4) | PORT_PCR_DSE_MASK | PORT_PCR_PE_MASK |
    PORT_PCR_PS_MASK;;

  /* SDHC0_D2: ALT4, high drive strength, pull up enabled */
  PORTE_PCR5 = PORT_PCR_MUX(4) | PORT_PCR_DSE_MASK | PORT_PCR_PE_MASK |
    PORT_PCR_PS_MASK;

  SIM_SCGC3 |= SIM_SCGC3_ESDHC_MASK;

  /* SDHC clock rate: 60 MHz / 128 = 468.75 kHz */
  SDHC_SYSCTL = SDHC_SYSCTL_SDCLKFS(128 >> 1);

  /* Reset SDHC */
  SDHC_SYSCTL |= SDHC_SYSCTL_RSTA_MASK;
  while(0 != (SDHC_SYSCTL & SDHC_SYSCTL_RSTA_MASK)) {
    /* Wait for reset complete */
  }

  while(0 != (SDHC_PRSSTAT & SDHC_PRSSTAT_CIHB_MASK)
	|| 0 != (SDHC_PRSSTAT & SDHC_PRSSTAT_CDIHB_MASK)) {
    /* Wait for command inhibit and command data inhibit to be clear */
  }

  /* Send 80 clock ticks to initialize card */
  SDHC_SYSCTL |= SDHC_SYSCTL_INITA_MASK;
  while(0 != (SDHC_SYSCTL & SDHC_SYSCTL_INITA_MASK)) {
    /* Wait for card initialization to complete */
  }

  /* Always transfer one 512-byte block */
  SDHC_BLKATTR = SDHC_BLKATTR_BLKCNT(1) | SDHC_BLKATTR_BLKSIZE(512);

  /* Set read and write watermarks to 1 word */
  SDHC_WML = SDHC_WML_RDWML(1) | SDHC_WML_WRWML(1);

  return sdhc_command_go_idle_state();
}

static enum sdhc_status sdhc_card_init(void) {
  enum sdhc_status status = sdhc_command_send_if_cond();
  if(SDHC_SUCCESS != status) {
    return status;

  } else {
    status = sdhc_command_send_op_cond();
    return status;
  }
}

static enum sdhc_status sdhc_command_all_send_cid(uint32_t cid[4]) {
  /* SD Physical Specification says that CMD2 has an R2 response.  See
     Part1 Physical Layer Simplified Specification, V8.00 (Tables 4-23
     through 4-30, printed pages 97-102, PDF pages 117-122). */
  /* The K70 manual does not have an entry for R2 responses in Table
     57-8 */
  /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-8 on printed
     page #2006, PDF page 2013) says that R2 requires Response type
     (RSPTYP) 2 Index check enable (CICEN) 1 and CRC check enable
     (CCCEN) 1; All sdhc_response_48_commands use RSPTYP(2).  The two
     true arguments set CICEN and CCCEN. */
  return sdhc_response_136_command(SD_COMMAND_ALL_SEND_CID_CMD2,
				   false, true, 0, cid);
}

static enum sdhc_status sdhc_command_send_relative_address(uint32_t *rca) {
  enum sdhc_status status;
  /* SD Physical Specification says that CMD3 has an R6 response.  See
     Part1 Physical Layer Simplified Specification, V8.00 (Tables 4-23
     through 4-30, printed pages 97-102, PDF pages 117-122). */
  /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-8 on printed
     page #2006, PDF page 2013) says that R6 requires Response type
     (RSPTYP) 2 Index check enable (CICEN) 1 and CRC check enable
     (CCCEN) 1; All sdhc_response_48_commands use RSPTYP(2).  The two
     true arguments set CICEN and CCCEN. */
  status = sdhc_response_48_command(SD_COMMAND_SEND_RELATIVE_ADDR_CMD3,
				    true, true, 0, rca);

  if(SDHC_SUCCESS == status) {
    if(0 != (*rca & 0x8000)) {
      return SDHC_COMMAND_ERROR_COMMAND_CRC;
    } else if(0 != (*rca & 0x4000)) {
      return SDHC_COMMAND_ERROR_COMMAND_INDEX;
    } else if(0 != (*rca & 0x2000)) {
      return SDHC_CARD_ERROR;
    } else {
      *rca &= 0xFFFF0000;
      return status;
    }
  } else {
    return status;
  }
}

static enum sdhc_status sdhc_command_send_csd(uint32_t rca, uint32_t csd[4]) {
  /* SD Physical Specification says that CMD9 has an R2 response.  See
     Part1 Physical Layer Simplified Specification, V8.00 (Tables 4-23
     through 4-30, printed pages 97-102, PDF pages 117-122). */
  /* The K70 manual does not have an entry for R2 responses in Table
     57-8 */
  /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-8 on printed
     page #2006, PDF page 2013) says that R2 requires Response type
     (RSPTYP) 2 Index check enable (CICEN) 1 and CRC check enable
     (CCCEN) 1; All sdhc_response_48_commands use RSPTYP(2).  The two
     true arguments set CICEN and CCCEN. */
  return sdhc_response_136_command(SD_COMMAND_SEND_CSD_CMD9,
				   false, true, rca, csd);
}

static enum sdhc_status sdhc_command_select_card(uint32_t rca, uint32_t *card_status) {
  /* SD Physical Specification says that CMD7 has an R1b response.
     See Part1 Physical Layer Simplified Specification, V8.00 (Tables
     4-23 through 4-30, printed pages 97-102, PDF pages 117-122). */
  /* Comment below cannot be correctly implemented with
     sdhc_response_48_command! */
  /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-8 on printed
     page #2006, PDF page 2013) says that R1b requires Response type
     (RSPTYP) 3 Index check enable (CICEN) 1 and CRC check enable
     (CCCEN) 1; All sdhc_response_48_commands use RSPTYP(2).  The two
     true arguments set CICEN and CCCEN. */
  return sdhc_response_48_command(SD_COMMAND_SELECT_OR_DESELECT_CARD_CMD7,
				  true, true, rca, card_status);
}

static enum sdhc_status sdhc_command_send_status(uint32_t rca, uint32_t *card_status) {
  /* SD Physical Specification says that CMD13 has an R1 response.
     See Part1 Physical Layer Simplified Specification, V8.00 (Tables
     4-23 through 4-30, printed pages 97-102, PDF pages 117-122). */
  /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-8 on printed
     page #2006, PDF page 2013) says that R1 requires Response type
     (RSPTYP) 2 Index check enable (CICEN) 1 and CRC check enable
     (CCCEN) 1; All sdhc_response_48_commands use RSPTYP(2).  The two
     true arguments set CICEN and CCCEN. */
  return sdhc_response_48_command(SD_COMMAND_SEND_STATUS_OR_SEND_TASK_STATUS_CMD13,
				  true, true, rca, card_status);
}

enum sdhc_status sdhc_read_single_block(uint32_t rca, uint32_t block_address,
                                        struct sdhc_card_status *card_status,
                                        uint8_t data[512]) {
  enum sdhc_status status;

  /* SD Physical Specification says that CMD17 has an R1 response.  See
     Part1 Physical Layer Simplified Specification, V8.00 (Tables 4-23
     through 4-30, printed pages 97-102, PDF pages 117-122). */
  /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-8 on printed
     page #2006, PDF page 2013) says that R1 requires Response type
     (RSPTYP) 2 Index check enable (CICEN) 1 and CRC check enable
     (CCCEN) 1. */
  uint32_t xfertyp = SDHC_XFERTYP_CMDINX(SD_COMMAND_READ_SINGLE_BLOCK_CMD17) |
    SDHC_XFERTYP_CICEN_MASK |
    SDHC_XFERTYP_CCCEN_MASK |
    SDHC_XFERTYP_DPSEL_MASK |
    SDHC_XFERTYP_DTDSEL_MASK |
    SDHC_XFERTYP_RSPTYP(2);

  status = sdhc_command(xfertyp, file_structure_first_sector + block_address);
  if(SDHC_SUCCESS == status) {
    int count = 0;
    uint32_t *dwords = (uint32_t *)data;
    /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-13 on printed
       page #2011, PDF page 2018) shows that for response types R1,
       R1b (normal response), R3, R4, R5, R5b, and R6, that bits 39:8
       of the response field are stored in response register
       CMDRSP0. */
    *(uint32_t *)card_status = SDHC_CMDRSP0;
    while(count * sizeof(dwords[0]) < 512) {
      /* The BREN flag indicates that valid data greater than the
	 watermark level exist in the buffer (i.e., that valid data
	 exists in the host side buffer) */
      if(0 != (SDHC_PRSSTAT & SDHC_PRSSTAT_BREN_MASK)) {
	dwords[count++] = SDHC_DATPORT;
      }
    }
  }
  return status;
}

enum sdhc_status sdhc_write_single_block(uint32_t rca, uint32_t block_address,
                                         struct sdhc_card_status *card_status,
                                         uint8_t data[512]) {
  enum sdhc_status status;

  /* SD Physical Specification says that CMD24 has an R1 response.  See
     Part1 Physical Layer Simplified Specification, V8.00 (Tables 4-23
     through 4-30, printed pages 97-102, PDF pages 117-122). */
  /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-8 on printed
     page #2006, PDF page 2013) says that R1 requires Response type
     (RSPTYP) 2 Index check enable (CICEN) 1 and CRC check enable
     (CCCEN) 1. */
  uint32_t xfertyp = SDHC_XFERTYP_CMDINX(SD_COMMAND_WRITE_BLOCK_CMD24) |
    SDHC_XFERTYP_CICEN_MASK |
    SDHC_XFERTYP_CCCEN_MASK |
    SDHC_XFERTYP_DPSEL_MASK |
    SDHC_XFERTYP_RSPTYP(2);
  status = sdhc_command(xfertyp, file_structure_first_sector + block_address);
  if(SDHC_SUCCESS == status) {
    int count = 0;
    uint32_t *dwords = (uint32_t *)data;
    /* K70 Sub-Family Reference Manual, Rev. 4 (Table 57-13 on printed
       page #2011, PDF page 2018) shows that for response types R1,
       R1b (normal response), R3, R4, R5, R5b, and R6, that bits 39:8
       of the response field are stored in response register
       CMDRSP0. */
    *(uint32_t *)card_status = SDHC_CMDRSP0;
    while(count * sizeof(dwords[0]) < 512) {
      /* The BWEN flag indicates that the buffer can hold valid data
	 greater than the write watermark level (i.e., that space is
	 available for write data) */
      if(0 != (SDHC_PRSSTAT & SDHC_PRSSTAT_BWEN_MASK)) {
	SDHC_DATPORT = dwords[count++];
      }
    }
  }
  return status;
}

char *sdhc_file_format(unsigned file_format, unsigned file_format_grp) {
  if(file_format_grp == 0) {
    if(file_format == CSD_FILE_FORMAT_MBR)
      return "MBR";
    else if(file_format == CSD_FILE_FORMAT_BOOT_SECTOR)
      return "Boot Sector";
    else if(file_format == CSD_FILE_FORMAT_UNIVERSAL)
      return "Universal";
    else if(file_format == CSD_FILE_FORMAT_OTHERS)
      return "Others";
  }
  return "Reserved";
}
