/*
 * microSD.h
 * NXP/Freescale K70 microSD Controller Header File
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu) & Daniel Willenson
 *
 * Copyright (c) 2021 James L. Frankel.  All rights reserved.
 */

#ifndef _MICRO_SD_H
#define _MICRO_SD_H

#include <stdint.h>

#include "SDCardReader.h"

/* These #define's enable debugging output */
#define MICRO_SD_DEBUG 0
#define MICRO_SD_INFORMATIVE_PRINTF 0

/* Routine to configure the microSD Card Detect switch & pull-down resistor in
   the ARM */
/* This must be called before calling microSDCardDetectedUsingSwitch
   or microSDCardDetectedUsingResistor */
void microSDCardDetectConfig(void);

/* Routine to read the state of the microSD Card Detect switch */
/* Returns true if the card is detected and false otherwise */
int microSDCardDetectedUsingSwitch(void);
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
/* Returns true if the card is detected and false otherwise */
int microSDCardDetectedUsingResistor(void);

/* Routine to disable the microSD Card Detect ARM internal pull-down
   resistor */
/* This must be called after conducting SD card detection
   using the SD card internal resistor and before initializing the SD
   card interface */
void microSDCardDisableCardDetectARMPullDownResistor(void);

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
uint32_t sdhc_initialize(void);

enum sdhc_status {
    SDHC_SUCCESS,
    SDHC_COMMAND_ERROR_DATA_END_BIT,
    SDHC_COMMAND_ERROR_DATA_CRC,
    SDHC_COMMAND_ERROR_DATA_TIMEOUT,
    SDHC_COMMAND_ERROR_COMMAND_INDEX,
    SDHC_COMMAND_ERROR_COMMAND_END_BIT,
    SDHC_COMMAND_ERROR_COMMAND_CRC,
    SDHC_COMMAND_ERROR_COMMAND_TIMEOUT,
    SDHC_VOLTAGE_MISMATCH,
    SDHC_CHECK_PATTERN_MISMATCH,
    SDHC_REJECTED_APP_CMD,
    SDHC_CARD_ERROR
};

/* Note: Using bit fields to map hardware bit field format relies on
   unspecified and implementation-defined behavior.  This code will
   probably not work correctly on a big endian computer. */

struct sdhc_card_status {
    uint32_t: 2;
    uint32_t: 1;
    unsigned int ake_seq_err: 1;
    uint32_t: 1;
    unsigned int app_cmd: 1;
    unsigned int fx_event: 1;
    uint32_t: 1;
    unsigned int ready_for_data: 1;

    enum {
        CARD_STATE_IDLE = 0,
        CARD_STATE_READY = 1,
        CARD_STATE_IDENT = 2,
        CARD_STATE_STBY = 3,
        CARD_STATE_TRAN = 4,
        CARD_STATE_DATA = 5,
        CARD_STATE_RCV = 6,
        CARD_STATE_PRG = 7,
        CARD_STATE_DIS = 8,

        CARD_STATE_RESERVED_09 = 9,
        CARD_STATE_RESERVED_10 = 10,
        CARD_STATE_RESERVED_11 = 11,
        CARD_STATE_RESERVED_12 = 12,
        CARD_STATE_RESERVED_13 = 13,
        CARD_STATE_RESERVED_14 = 14,

        CARD_STATE_RESERVED_IO = 15
    } card_state: 4;
    unsigned int erase_reset: 1;
    unsigned int card_ecc_disabled: 1;
    unsigned int wp_erase_skip: 1;
    unsigned int csd_overwrite: 1;
    unsigned int deferred_response: 1;
    uint32_t: 1;
    unsigned int error: 1;
    unsigned int cc_error: 1;
    unsigned int card_ecc_failed: 1;
    unsigned int illegal_command: 1;
    unsigned int com_crc_error: 1;
    unsigned int lock_unlock_failed: 1;
    unsigned int card_is_locked: 1;
    unsigned int wp_violation: 1;
    unsigned int erase_param: 1;
    unsigned int erase_seq_error: 1;
    unsigned int block_len_error: 1;
    unsigned int address_error: 1;
    unsigned int out_of_range: 1;
};

/* Reads a block from the SDHC card */
/*   rca is the Relative Card Address returned from sdhc_initialize */
/*   block_address is the number of the sector to be read */
/*   card_status is a pointer to an existing struct sdhc_card_status in which
     additional error information will be returned in the event of a
     non-successful call */
/*   data is a pointer to existing memory in which the sector will be read */
/* Returns status for the result of the operation; If the return value is
   equal to SDHC_SUCCESS, all is well; otherwise, the operation failed */
enum sdhc_status sdhc_read_single_block(uint32_t rca, uint32_t block_address,
                                        struct sdhc_card_status *card_status,
                                        uint8_t data[512]);

/* Writes a block to the SDHC card */
/*   rca is the Relative Card Address returned from sdhc_initialize */
/*   block_address is the number of the sector to be written */
/*   card_status is a pointer to an existing struct sdhc_card_status in which
     additional error information will be returned in the event of a
     non-successful call */
/*   data is a pointer to the sector contents that will be written */
/* Returns status for the result of the operation; If the return value is
   equal to SDHC_SUCCESS, all is well; otherwise, the operation failed */
enum sdhc_status sdhc_write_single_block(uint32_t rca, uint32_t block_address,
        struct sdhc_card_status *card_status,
        uint8_t data[512]);

/* Routine to connect the 50k Ohm (nominal value, specified range is 10k Ohm to
   90k Ohm) pull-up resistor inside the SD card; This should be called when
   unmounting (i.e., finished using) the SD card; The function can be invoked
   only after calling sdhc_initialize */
enum sdhc_status sdhc_command_send_set_clr_card_detect_connect(uint32_t rca);

/* #define's and functions below this comment are for internal use only */
/* -------------------------------------------------------------------- */

/* First sector of the FAT32 file structure */
extern uint32_t file_structure_first_sector;

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
#define MICRO_SD_OUTPUT_BUFFER_SIZE 129

#define MICRO_SD_CARD_DETECT_SWITCH_PORTE_BIT 28
#define MICRO_SD_CARD_DETECT_RESISTOR_PORTE_BIT 4

#ifndef PORT_PCR_MUX_ANALOG
#define PORT_PCR_MUX_ANALOG 0
#endif
#ifndef PORT_PCR_MUX_GPIO
#define PORT_PCR_MUX_GPIO 1
#endif

#define SD_COMMAND_GO_IDLE_STATE_CMD0 0
#define SD_COMMAND_ALL_SEND_CID_CMD2 2
#define SD_COMMAND_SEND_RELATIVE_ADDR_CMD3 3
#define SD_COMMAND_SET_DSR_CMD4 4
#define SD_COMMAND_SELECT_OR_DESELECT_CARD_CMD7 7
#define SD_COMMAND_SEND_IF_COND_CMD8 8
#define SD_COMMAND_SEND_CSD_CMD9 9
#define SD_COMMAND_SEND_CID_CMD10 10
#define SD_COMMAND_VOLTAGE_SWITCH_CMD11 11
#define SD_COMMAND_STOP_TRANSMISSION_CMD12 12
#define SD_COMMAND_SEND_STATUS_OR_SEND_TASK_STATUS_CMD13 13
#define SD_COMMAND_GO_INACTIVE_STATE_CMD15 15
#define SD_COMMAND_SET_BLOCKLEN_CMD16 16
#define SD_COMMAND_READ_SINGLE_BLOCK_CMD17 17
#define SD_COMMAND_READ_MULTIPLE_BLOCK_CMD18 18
#define SD_COMMAND_SET_TUNING_BLOCK_CMD19 19
#define SD_COMMAND_SPEED_CLASS_CONTROL_CMD20 20
#define SD_COMMAND_ADDRESS_EXTENSION_CMD22 22
#define SD_COMMAND_SET_BLOCK_COUNT_CMD23 23
#define SD_COMMAND_WRITE_BLOCK_CMD24 24
#define SD_COMMAND_WRITE_MULTIPLE_BLOCK_CMD25 25
#define SD_COMMAND_PROGRAM_CSD_CMD27 27
#define SD_COMMAND_SET_WRITE_PROT_CMD28 28
#define SD_COMMAND_CLR_WRITE_PROT_CMD29 29
#define SD_COMMAND_SEND_WRITE_PROT_CMD30 30
#define SD_COMMAND_ERASE_WR_BLK_START_CMD32 32
#define SD_COMMAND_ERASE_WR_BLK_END_CMD33 33
#define SD_COMMAND_ERASE_CMD38 38
#define SD_COMMAND_LOCK_UNLOCK_CMD42 42
#define SD_COMMAND_APP_CMD_CMD55 55
#define SD_COMMAND_GEN_CMD_CMD56 56

#define SD_APP_COMMAND_SD_SEND_OP_COND_ACMD41 41
#define SD_APP_COMMAND_SET_CLR_CARD_DETECT_ACMD42 42

#define CARD_STATUS_OUT_OF_RANGE_BIT 31
#define CARD_STATUS_ADDRESS_ERROR_BIT 30
#define CARD_STATUS_BLOCK_LEN_ERROR_BIT 29
#define CARD_STATUS_ERASE_SEQ_ERROR_BIT 28
#define CARD_STATUS_ERASE_PARAM_BIT 27
#define CARD_STATUS_WP_VIOLATION_BIT 26
#define CARD_STATUS_CARD_IS_LOCKED_BIT 25
#define CARD_STATUS_LOCK_UNLOCK_FAILED_BIT 24
#define CARD_STATUS_COM_CRC_ERROR_BIT 23
#define CARD_STATUS_ILLEGAL_COMMAND_BIT 22
#define CARD_STATUS_CARD_ECC_FAILED_BIT 21
#define CARD_STATUS_CC_ERROR_BIT 20
#define CARD_STATUS_ERROR_BIT 19
#define CARD_STATUS_RESERVED18_BIT 18
#define CARD_STATUS_DEFERRED_RESPONSE_BIT 17
#define CARD_STATUS_CSD_OVERWRITE_BIT 16
#define CARD_STATUS_WP_ERASE_SKIP_BIT 15
#define CARD_STATUS_CARD_ECC_DISABLED_BIT 14
#define CARD_STATUS_ERASE_RESET_BIT 13

#define CARD_STATUS_CURRENT_STATE_MASK 0x1e00

#define CARD_STATUS_READY_FOR_DATA_BIT 8
#define CARD_STATUS_RESERVED7_BIT 7
#define CARD_STATUS_FX_EVENT_BIT 6
#define CARD_STATUS_APP_CMD_BIT 5
#define CARD_STATUS_RESERVED4_BIT 4
#define CARD_STATUS_AKE_SEQ_ERROR_BIT 3
#define CARD_STATUS_RESERVED2_BIT 2
#define CARD_STATUS_RESERVED1_BIT 1
#define CARD_STATUS_RESERVED0_BIT 0

/* Voltage supplied between 2.7V and 3.6V */
#define SD_CMD8_IF_COND_ARG_VHS_MASK (0xf << 8)
#define SD_CMD8_IF_COND_ARG_VHS_2_7_TO_3_6 (1 << 8)

/* Check pattern */
#define SD_CMD8_IF_COND_ARG_CHECK_PATTERN_MASK 0xff
#define SD_CMD8_IF_COND_ARG_CHECK_PATTERN 0xbe

/* HCS (bit 30) set to 1 and HO2T (bit 27) set to 0 mean that
   SDHC/SDXC is supported by the host */
#define SD_ACMD41_OP_COND_ARG_HCS (1 << 30)
/* XPC (bit 28) set to 1 means that 0.54W (150mA at 3.6V on VDD1) max. and
   speed class is supported */
#define SD_ACMD41_OP_COND_ARG_XPC (1 << 28)
/* VDD_VOLTAGE_WINDOW is a 24 bit field set to 1 means:
   Bit position		Definition
   7			Reserved for Low Voltage Range
   15			2.7-2.8
   16			2.8-2.9

   17			2.9-3.0
   18			3.0-3.1
   19			3.1-3.2

   20			3.2-3.3
   21			3.3-3.4
   22			3.4-3.5
   23			3.5-3.6

   Value of 0x300000 means 3.2-3.4V
   */
#define SD_ACMD41_OP_COND_ARG_OCR_VDD_VOLTAGE_WINDOW_3_2_TO_3_4 0x300000

/* Note: Using bit fields to map hardware bit field format relies on
   unspecified and implementation-defined behavior.  This code will
   probably not work correctly on a big endian computer. */

struct sdhc_card_id {
    uint8_t manufacture_month: 4;
    uint16_t manufacture_year: 8;
    uint8_t: 4;
    uint16_t product_serial_low: 16;
    uint16_t product_serial_high: 16;
    uint8_t product_revision_minor: 4;
    uint8_t product_revision_major: 4;
    char product_name[5];
    char oem_id[2];
    uint8_t manufacturer_id;
    uint8_t: 8;
};

#define CSD_FILE_FORMAT_MBR 0
#define CSD_FILE_FORMAT_BOOT_SECTOR 1
#define CSD_FILE_FORMAT_UNIVERSAL 2
#define CSD_FILE_FORMAT_OTHERS 3

/* Note: Using bit fields to map hardware bit field format relies on
   unspecified and implementation-defined behavior.  This code will
   probably not work correctly on a big endian computer. */

struct sdhc_card_specific_data {
    uint32_t: 2;
    uint32_t file_format: 2;
    unsigned int tmp_write_protect: 1;
    unsigned int perm_write_protect: 1;
    unsigned int copy: 1;
    unsigned int file_format_grp: 1;

    uint32_t: 5;
    unsigned int write_bl_partial: 1;
    uint32_t write_bl_len: 4;
    uint32_t r2w_factor: 3;
    uint32_t: 2;
    unsigned int wp_grp_enable: 1;

    uint32_t wp_grp_size: 7;
    uint32_t sector_size_low: 1;


    union {
        struct {
            uint32_t sector_size_high: 6;
            unsigned int erase_blk_en: 1;
            uint32_t c_size_mult_low: 1;

            uint32_t c_size_mult_high: 2;
            uint32_t vdd_w_curr_max: 3;
            uint32_t vdd_w_curr_min: 3;


            uint32_t vdd_r_curr_max: 3;
            uint32_t vdd_r_curr_min: 3;
            uint32_t c_size_low: 10;


            uint32_t c_size_high: 2;
            uint32_t: 2;
            unsigned int dsr_imp: 1;
            unsigned int read_blk_misalign: 1;
            unsigned int write_blk_misalign: 1;
            unsigned int read_bl_partial: 1;
            uint32_t read_bl_len: 4;
            uint32_t ccc: 12;
            uint32_t tran_speed: 8;
        } v1_0;
        struct {
            uint32_t sector_size_high: 6;
            unsigned int erase_blk_en: 1;
            uint32_t: 1;

            uint32_t c_size: 22;
            uint32_t: 2;


            uint32_t: 4;
            unsigned int dsr_imp: 1;
            unsigned int read_blk_misalign: 1;
            unsigned int write_blk_misalign: 1;
            unsigned int read_bl_partial: 1;
            uint32_t read_bl_len: 4;
            uint32_t ccc: 12;
            uint32_t tran_speed: 8;
        } v2_0;
    };

    uint32_t nsac: 8;
    uint32_t taac: 8;
    uint32_t: 6;
    uint32_t csd_structure: 2;
};

/* Converts the file_format and file_format_grp fields from the SD card
   "card specific data" into a printable string */
char *sdhc_file_format(unsigned file_format, unsigned file_format_grp);

#endif /* ifndef _MICRO_SD_H */
