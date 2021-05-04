/*
 *    kinetis_sysinit.c - Default init routines for P3
 *                     		Kinetis ARM systems
 *    Copyright 2012 Freescale semiConductor Inc. All Rights Reserved.
 */
 
#include "kinetis_sysinit.h"
#include "derivative.h"

/**
 **===========================================================================
 **  External declarations
 **===========================================================================
 */
#if __cplusplus
extern "C" {
#endif
extern uint32_t __vector_table[];
extern unsigned long _estack;
extern void __thumb_startup(void);
extern void svcHandler(void); //handler for svc
extern void interruptSerialPort2(void); //handler for UART2
extern void flexTimer0Isr(void); //handler for flexTimer 0
extern void PDB0Isr(void); //handler for pdb0
extern void SysTickHandler(void); //handler for systick

#if __cplusplus
}
#endif

/**
 **===========================================================================
 **  Default interrupt handler
 **===========================================================================
 */
void Default_Handler() {
  __asm("bkpt");
}

/**
 **===========================================================================
 **  Reset handler
 **===========================================================================
 */
void __init_hardware() {
  SCB_VTOR = (uint32_t)__vector_table; /* Set the interrupt vector table position */
  /*
    Disable the Watchdog because it may reset the core before entering main().
    There are 2 unlock words which shall be provided in sequence before
    accessing the control register.
  */
  WDOG_UNLOCK = KINETIS_WDOG_UNLOCK_SEQ_1;
  WDOG_UNLOCK = KINETIS_WDOG_UNLOCK_SEQ_2;
  WDOG_STCTRLH = KINETIS_WDOG_DISABLED_CTRL;
}

/* Weak definitions of handlers point to Default_Handler if not implemented */
void NMI_Handler() __attribute__ ((weak, alias("Default_Handler")));
void HardFault_Handler() __attribute__ ((weak, alias("Default_Handler")));
void MemManage_Handler() __attribute__ ((weak, alias("Default_Handler")));
void BusFault_Handler() __attribute__ ((weak, alias("Default_Handler")));
void UsageFault_Handler() __attribute__ ((weak, alias("Default_Handler")));
void DebugMonitor_Handler() __attribute__ ((weak, alias("Default_Handler")));
void PendSV_Handler() __attribute__ ((weak, alias("Default_Handler")));
void SysTick_Handler() __attribute__ ((weak, alias("Default_Handler")));

/* The Interrupt Vector Table */
void (* const InterruptVector[])() __attribute__ ((section(".vectortable"))) = {

  /* ARM Core System Handler Vectors */
  (void(*)(void)) &_estack,	/* Vector 0: Initial Stack Pointer (SP) */
  __thumb_startup,		/* Vector 1: Initial Program Counter (PC) */
  NMI_Handler,			/* Vector 2: Non-Maskable Interrupt (NMI) */
  HardFault_Handler,		/* Vector 3: Hard Fault */
  MemManage_Handler,		/* Vector 4: MemManage Fault */
  BusFault_Handler,		/* Vector 5: Bus Fault */
  UsageFault_Handler,		/* Vector 6: Usage Fault */
  0,				/* Vector 7 */
  0,				/* Vector 8 */
  0,				/* Vector 9 */
  0,				/* Vector 10 */
  svcHandler,			/* Vector 11: SuperVisor Call (SVCall) */
  DebugMonitor_Handler,		/* Vector 12: Debug Monitor */
  0,				/* Vector 13 */
  SysTickHandler,		/* Vector 14: Pendable request for system
				   service (PendSV) */
  SysTickHandler,		/* Vector 15: System Tick Timer (SysTick) */

  /* Non-Core Vectors */
  Default_Handler,		/* Vector 16: DMA Chan 0, 16 Trnsfr Complete */
  Default_Handler,		/* Vector 17: DMA Chan 1, 17 Trnsfr Complete */
  Default_Handler,		/* Vector 18: DMA Chan 2, 18 Trnsfr Complete */
  Default_Handler,		/* Vector 19: DMA Chan 3, 19 Trnsfr Complete */
  Default_Handler,		/* Vector 20: DMA Chan 4, 20 Trnsfr Complete */
  Default_Handler,		/* Vector 21: DMA Chan 5, 21 Trnsfr Complete */
  Default_Handler,		/* Vector 22: DMA Chan 6, 22 Trnsfr Complete */
  Default_Handler,		/* Vector 23: DMA Chan 7, 23 Trnsfr Complete */
  Default_Handler,		/* Vector 24: DMA Chan 8, 24 Trnsfr Complete */
  Default_Handler,		/* Vector 25: DMA Chan 9, 25 Trnsfr Complete */
  Default_Handler,		/* Vector 26: DMA Chan 10, 26 Trnsfr Complete */
  Default_Handler,		/* Vector 27: DMA Chan 11, 27 Trnsfr Complete */
  Default_Handler,		/* Vector 28: DMA Chan 12, 28 Trnsfr Complete */
  Default_Handler,		/* Vector 29: DMA Chan 13, 29 Trnsfr Complete */
  Default_Handler,		/* Vector 30: DMA Chan 14, 30 Trnsfr Complete */
  Default_Handler,		/* Vector 31: DMA Chan 15, 31 Trnsfr Complete */
  Default_Handler,		/* Vector 32: DMA Error Channels 0-31 */
  Default_Handler,		/* Vector 33: MCM, Normal Interrupt */
  Default_Handler,		/* Vector 34: Flash Memory, Command Complete*/
  Default_Handler,		/* Vector 35: Flash Memory, Read Collision */
  Default_Handler,		/* Vector 36: Mode Controller, Low-voltage
				   Detect, low-voltage Warning */
  Default_Handler,		/* Vector 37: LLWU, Low Leakage Wakeup */
  Default_Handler,		/* Vector 38: WDOG: Watchdog Interrupt */
  Default_Handler,		/* Vector 39: RNG: Random Number Generator */
  Default_Handler,		/* Vector 40: I2C0 */
  Default_Handler,		/* Vector 41: I2C1 */
  Default_Handler,		/* Vector 42: SPI0 */
  Default_Handler,		/* Vector 43: SPI1 */
  Default_Handler,		/* Vector 44: SPI2 */
  Default_Handler,		/* Vector 45: CAN0, OR'ed Msg buffer (0-15) */
  Default_Handler,		/* Vector 46: CAN0, Bus Off */
  Default_Handler,		/* Vector 47: CAN0, Error */
  Default_Handler,		/* Vector 48: CAN0, Transmit Warning */
  Default_Handler,		/* Vector 49: CAN0, Receive Warning */
  Default_Handler,		/* Vector 50: CAN0, Wake Up */
  Default_Handler,		/* Vector 51: I2S0, Transmit */
  Default_Handler,		/* Vector 52: I2S0, Receive */
  Default_Handler,		/* Vector 53: CAN1, OR'ed Msg buffer (0-15) */
  Default_Handler,		/* Vector 54: CAN1, Bus Off */
  Default_Handler,		/* Vector 55: CAN1, Error */
  Default_Handler,		/* Vector 56: CAN1, Transmit Warning */
  Default_Handler,		/* Vector 57: CAN1, Receive Warning */
  Default_Handler,		/* Vector 58: CAN1, Wake Up */
  Default_Handler,		/* Vector 59 */
  Default_Handler,		/* Vector 60: UART0, LON Sources */
  Default_Handler,		/* Vector 61: UART0, Status Sources */
  Default_Handler,		/* Vector 62: UART0, Error Sources */
  Default_Handler,		/* Vector 63: UART1, Status Sources */
  Default_Handler,		/* Vector 64: UART1, Error Sources */
  interruptSerialPort2,		/* Vector 65: UART2, Status Sources */
  Default_Handler,		/* Vector 66: UART2, Error Sources */
  Default_Handler,		/* Vector 67: UART3, Status Sources */
  Default_Handler,		/* Vector 68: UART3, Error Sources */
  Default_Handler,		/* Vector 69: UART4, Status Sources */
  Default_Handler,		/* Vector 70: UART4, Error Sources */
  Default_Handler,		/* Vector 71: UART5, Status Sources */
  Default_Handler,		/* Vector 72: UART5, Error Sources */
  Default_Handler,		/* Vector 73: ADC0 */
  Default_Handler,		/* Vector 74: ADC1 */
  Default_Handler,		/* Vector 75: CMP0 */
  Default_Handler,		/* Vector 76: CMP1 */
  Default_Handler,		/* Vector 77: CMP2 */
  flexTimer0Isr,		/* Vector 78: FTM0 */
  Default_Handler,		/* Vector 79: FTM1 */
  Default_Handler,		/* Vector 80: FTM2 */
  Default_Handler,		/* Vector 81: CMT */
  Default_Handler,		/* Vector 82: RTC, Alarm Interrupt */
  Default_Handler,		/* Vector 83: RTC, Seconds Interrupt */
  Default_Handler,		/* Vector 84: PIT, Channel 0 */
  Default_Handler,		/* Vector 85: PIT, Channel 1 */
  Default_Handler,		/* Vector 86: PIT, Channel 2 */
  Default_Handler,		/* Vector 87: PIT, Channel 3 */
  PDB0Isr,		/* Vector 88: PDB */
  Default_Handler,		/* Vector 89: USB OTG */
  Default_Handler,		/* Vector 90: USB Charger Detect */
  Default_Handler,		/* Vector 91: Ethernet MAC, IEEE 1588 Timer */
  Default_Handler,		/* Vector 92: Ethernet MAC, Transmit */
  Default_Handler,		/* Vector 93: Ethernet MAC, Receive */
  Default_Handler,		/* Vector 94: Ethernet MAC, Error & Misc */
  Default_Handler,		/* Vector 95 */
  Default_Handler,		/* Vector 96: SDHC */
  Default_Handler,		/* Vector 97: DAC0 */
  Default_Handler,		/* Vector 98: DAC1 */
  Default_Handler,		/* Vector 99: TSI */
  Default_Handler,		/* Vector 100: MCG */
  Default_Handler,		/* Vector 101: Low Power Timer */
  Default_Handler,		/* Vector 102 */
  Default_Handler,		/* Vector 103: Port Cntl, Pin Detect (Port A) */
  Default_Handler,		/* Vector 104: Port Cntl, Pin Detect (Port B) */
  Default_Handler,		/* Vector 105: Port Cntl, Pin Detect (Port C) */
  Default_Handler,		/* Vector 106: Port Cntl, Pin Detect (Port D) */
  Default_Handler,		/* Vector 107: Port Cntl, Pin Detect (Port E) */
  Default_Handler,		/* Vector 108: Port Cntl, Pin Detect (Port F) */
  Default_Handler,		/* Vector 109: DDR Controller */
  Default_Handler,		/* Vector 110: Software Interrupt */
  Default_Handler,		/* Vector 111: NAND Flash Controller (NFC) */
  Default_Handler,		/* Vector 112: USB HS */
  Default_Handler,		/* Vector 113: Graphical LCD */
  Default_Handler,		/* Vector 114: CMP3 */
  Default_Handler,		/* Vector 115: Tamper, DryIce Tamper */
  Default_Handler,		/* Vector 116 */
  Default_Handler,		/* Vector 117: FTM3 */
  Default_Handler,		/* Vector 118: ADC2 */
  Default_Handler,		/* Vector 119: ADC3 */
  Default_Handler,		/* Vector 120: I2S1, Transmit */
  Default_Handler,		/* Vector 121: I2S1, Receive */
};
