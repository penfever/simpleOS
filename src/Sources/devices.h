/*
 * devices.h
 *
 *  Created on: Mar 11, 2021
 *
 */

#include <stdint.h>
#include "univio.h"

/*General device handling*/
#ifndef DEVICES_H_
#define DEVICES_H_
#define DEV 17
#define MAXOPEN 32
#define PUSHB_MIN 0x00FF0000
#define PUSHB_MAX 0x00FF0001
#define LED_MIN 0x00EF0000
#define LED_MAX 0x00EF0003
#define ADC_MIN 0x02EF0000
#define ADC_MAX 0x02EF0001
#define TSI_MIN 0x03EF0000
#define TSI_MAX 0x03EF0003
#define DAC_MIN 0x04EF0000
#define DAC_MAX 0x04EF0001
#ifndef CONSOLEIO
#define CONSOLEIO 0
#endif
#ifndef UARTIO
#define UARTIO TRUE
#endif

/*ADC*/
#define ADC_CHANNEL_POTENTIOMETER   	0x14
#define ADC_CHANNEL_TEMPERATURE_SENSOR  0x1A

#define ADC_CFG1_MODE_8_9_BIT       0x0
#define ADC_CFG1_MODE_12_13_BIT     0x1
#define ADC_CFG1_MODE_10_11_BIT     0x2
#define ADC_CFG1_MODE_16_BIT        0x3
#define ADC_SC3_AVGS_32_SAMPLES     0x3

#define PORT_PCR_MUX_ANALOG 0
#define PORT_PCR_MUX_GPIO 1

#define ELECTRODE_COUNT 4
#define THRESHOLD_OFFSET 0x200

/*Systick*/
#define CSRINIT 0x6F
#define QUANTUM 6000000 //50ms default
#define QUANTUM_INTERRUPT_PRIORITY 14

extern uint32_t g_firstrun_flag;

extern struct console console;

struct stream { //Abstraction: what device is this, and how do I talk to it?
    enum device_type { // Abstraction: Major IDs
        UNUSED = 0,
        FAT32 = 1,
        PUSHBUTTON = 2,
        LED = 3,
        IO = 4,
        ADC = 5,
        TSI = 6,
        DAC = 7
    } deviceType; //Major ID
    int minorId;
    int mode;  //rw mode
    enum dev_status
    {
        off = 0,
        on = 1
    } devStatus;
    //FAT32 specific entries
    uint32_t cursor; // current position in file (shared read/write)
    uint32_t clusterAddr;
    char *fileName;
    uint32_t fileSize;
    //LED-specific entries
    enum led_color
    {
        red = 1,
        yellow = 2,
        green = 3,
        blue = 4
    } ledcolor;
};

struct dev_id {
    uint32_t minorId;
    char *dev_name;
};

enum minor_id {
    dev_null = 0,
    dev_sdhc = 0x99FF0000,
    dev_sw1 = 0x00FF0000,
    dev_sw2 = 0x00FF0001,
    dev_E1 = 0x00EF0000, //orange
    dev_E2 = 0x00EF0001, //blue
    dev_E3 = 0x00EF0002, //green
    dev_E4 = 0x00EF0003, //yellow
    dev_UART2 = 0x01EF0000,
    dev_pot = 0x02EF0000,
    dev_temp = 0x02EF0001,
    dev_TSI1 = 0x03EF0000,
    dev_TSI2 = 0x03EF0001,
    dev_TSI3 = 0x03EF0002,
    dev_TSI4 = 0x03EF0003,
    dev_DAC0 = 0x04EF0000,
    dev_DAC1 = 0x04EF0001
};

typedef struct dev_id dev_id_t;
extern dev_id_t devTable[DEV];

int uart_init(int baud);

int init_sys(void);

void adc_init(void);

unsigned int adc_read(uint8_t channel);

int electrode_in(int electrodeNumber);

void TSI_Calibrate(void);

void TSI_Init(void);

void SysTickHandler(void);

void systick_init(void);

void systick_pause(void);

void systick_resume(void);

void dacInit(void);

#endif
