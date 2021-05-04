/**
 * svc.c
 * Routines for supervisor calls
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021, 2014 James L. Frankel.  All rights reserved.
 */

/*
 * Size of the supervisor call stack frame (no FP extension):
 *   No alignment => 32 (0x20) bytes
 *   With alignment => 36 (0x24) bytes
 *   
 * Format of the supervisor call stack frame (no FP extension):
 *   SP Offset   Contents
 *   +0			 R0
 *   +4			 R1
 *   +8			 R2
 *   +12		 R3
 *   +16		 R12
 *   +20		 LR (R14)
 *   +24		 Return Address
 *   +28		 xPSR (bit 9 indicates the presence of a reserved alignment
 *   				   word at offset +32)
 *   +32		 Possible Reserved Word for Alignment on 8 Byte Boundary
 *   
 * Size of the supervisor call stack frame (with FP extension):
 *   No alignment => 104 (0x68) bytes
 *   With alignment => 108 (0x6C) bytes
 *   
 * Format of the supervisor call stack frame (with FP extension):
 *   SP Offset   Contents
 *   +0			 R0
 *   +4			 R1
 *   +8			 R2
 *   +12		 R3
 *   +16		 R12
 *   +20		 LR (R14)
 *   +24		 Return Address
 *   +28		 xPSR (bit 9 indicates the presence of a reserved alignment
 *   				   word at offset +104)
 *   +32		 S0
 *   +36		 S1
 *   +40		 S2
 *   +44		 S3
 *   +48		 S4
 *   +52		 S5
 *   +56		 S6
 *   +60		 S7
 *   +64		 S8
 *   +68		 S9
 *   +72		 S10
 *   +76		 S11
 *   +80		 S12
 *   +84		 S13
 *   +88		 S14
 *   +92		 S15
 *   +96		 FPSCR
 *   +100		 Reserved Word for 8 Byte Boundary of Extended Frame
 *   +104		 Possible Reserved Word for Alignment on 8 Byte Boundary
 */

#include <derivative.h>
#include <stdio.h>
#include "svc.h"
#include "univio.h"
#include "SDHC_FAT32_Files.h"
#include "myerror.h"
#include "simpleshell.h"
#include "mymalloc.h"
#include "devices.h"
#include "datetime.h"
#include "uart.h"
#include "pdb.h"
#include "procs.h"

#define XPSR_FRAME_ALIGNED_BIT 9
#define XPSR_FRAME_ALIGNED_MASK (1<<XPSR_FRAME_ALIGNED_BIT)

struct frame {
	union {
		int r0;
		int arg0;
		int returnVal;
		file_descriptor* descr;
		file_descriptor descrf;
		unsigned int unsInt;
	};
	union {
		int r1;
		int arg1;
		char* filename;
		char bufp;
	};
	union {
		int r2;
		int arg2;
		char mode;
	};
	union {
		int r3;
		int arg3;
	};
	int r12;
	union {
		int r14;
		int lr;
	};
	int returnAddr;
	int xPSR;
};

uint32_t g_interrupt_count;

/* Issue the SVC (Supervisor Call) instruction (See A7.7.175 on page A7-503 of the
 * ARM�v7-M Architecture Reference Manual, ARM DDI 0403Derrata 2010_Q3 (ID100710)) */
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVC_fopen(file_descriptor* descr, char* filename, char mode) {
	__asm("svc %0" : : "I" (SVC_FOPEN));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#else
int __attribute__((never_inline)) SVC_fopen(file_descriptor* descr, char* filename, char mode) {
	__asm("svc %0" : : "I" (SVC_FOPEN));
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVC_fclose(file_descriptor descrf) {
	__asm("svc %0" : : "I" (SVC_FCLOSE));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#else
int __attribute__((never_inline)) SVC_fclose(file_descriptor descrf) {
	__asm("svc %0" : : "I" (SVC_FCLOSE));
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVC_create(char* filename) {
	__asm("svc %0" : : "I" (SVC_CREATE));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#else
int __attribute__((never_inline)) SVC_create(char* filename) {
	__asm("svc %0" : : "I" (SVC_CREATE));
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVC_delete(char* filename) {
	__asm("svc %0" : : "I" (SVC_DELETE));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#else
int __attribute__((never_inline)) SVC_delete(char* filename) {
	__asm("svc %0" : : "I" (SVC_DELETE));
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVC_fgetc(file_descriptor descrf, char* bufp) {
	__asm("svc %0" : : "I" (SVC_FGETC));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#else
int __attribute__((never_inline)) SVC_fgetc(file_descriptor descrf, char* bufp) {
	__asm("svc %0" : : "I" (SVC_FGETC));
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVC_fputc(file_descriptor descrf, char bufp) {
	__asm("svc %0" : : "I" (SVC_FPUTC));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#else
int __attribute__((never_inline)) SVC_fputc(file_descriptor descrf, char* bufp, int buflen) {
	__asm("svc %0" : : "I" (SVC_FPUTC));
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVC_fputs(file_descriptor descrf, char* bufp, int buflen) {
	__asm("svc %0" : : "I" (SVC_FPUTS));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#else
int __attribute__((never_inline)) SVC_fputs(file_descriptor descrf, char* bufp, int buflen) {
	__asm("svc %0" : : "I" (SVC_FPUTS));
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVC_fgets(file_descriptor descrf, char* bufp, int buflen) {
	__asm("svc %0" : : "I" (SVC_FGETS));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#else
int __attribute__((never_inline)) SVC_fgets(file_descriptor descrf, char* bufp, int buflen) {
	__asm("svc %0" : : "I" (SVC_FGETS));
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
void* __attribute__((naked)) __attribute__((noinline)) SVC_malloc(unsigned int size) {
	__asm("svc %0" : : "I" (SVC_MALLOC));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#else
void* __attribute__((never_inline)) SVC_malloc(unsigned int size) {
	__asm("svc %0" : : "I" (SVC_MALLOC));
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVC_free(void* ptr) {
	__asm("svc %0" : : "I" (SVC_FREE));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#else
int __attribute__((never_inline)) SVC_free(void* ptr) {
	__asm("svc %0" : : "I" (SVC_FREE));
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVC_dir_ls(int full) {
	__asm("svc %0" : : "I" (SVC_DIR_LS));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#else
int __attribute__((never_inline)) SVC_dir_ls(int full) {
	__asm("svc %0" : : "I" (SVC_DIR_LS));
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVC_settime(long long* newTime) {
	__asm("svc %0" : : "I" (SVC_SETTIME));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#else
int __attribute__((never_inline)) SVC_settime(long long newTime) {
	__asm("svc %0" : : "I" (SVC_SETTIME));
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVC_pdb0oneshottimer(uint16_t* delayCount) {
	__asm("svc %0" : : "I" (SVC_PDBONESHOT));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#else
int __attribute__((never_inline)) SVC_pdb0oneshottimer(uint16_t* delayCount) {
	__asm("svc %0" : : "I" (SVC_PDBONESHOT));
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVC_spawn(int main(int argc, char *argv[]), int argc, char *argv[], struct spawnData* thisSpawn) {
	__asm("svc %0" : : "I" (SVC_SPAWN));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#else
int __attribute__((never_inline)) SVC_spawn(int main(int argc, char *argv[]), int argc, char *argv[], struct spawnData* thisSpawn) {
	__asm("svc %0" : : "I" (SVC_SPAWN));
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
void __attribute__((naked)) __attribute__((noinline)) SVC_yield(void) {
	__asm("svc %0" : : "I" (SVC_YIELD));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#else
void __attribute__((never_inline)) SVC_yield(void) {
	__asm("svc %0" : : "I" (SVC_YIELD));
}
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
void __attribute__((naked)) __attribute__((noinline)) SVC_block(void) {
	__asm("svc %0" : : "I" (SVC_BLOCK));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVC_wake(pid_t targetPid) {
	__asm("svc %0" : : "I" (SVC_WAKE));
	__asm("bx lr");
}
#endif
#pragma GCC diagnostic pop
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVC_kill(pid_t targetPid) {
	__asm("svc %0" : : "I" (SVC_KILL));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
void __attribute__((naked)) __attribute__((noinline)) SVC_wait(pid_t targetPid) {
	__asm("svc %0" : : "I" (SVC_WAIT));
	__asm("bx lr");
}
#pragma GCC diagnostic pop
#endif

/* This function sets the priority at which the SVCall handler runs (See
 * B3.2.11, System Handler Priority Register 2, SHPR2 on page B3-723 of
 * the ARM�v7-M Architecture Reference Manual, ARM DDI 0403Derrata
 * 2010_Q3 (ID100710)).
 * 
 * If priority parameter is invalid, this function performs no action.
 * 
 * The ARMv7-M Architecture Reference Manual in section "B1.5.4 Exception
 * priorities and preemption" on page B1-635 states, "The number of
 * supported priority values is an IMPLEMENTATION DEFINED power of two in
 * the range 8 to 256, and the minimum supported priority value is always 0.
 * All priority value fields are 8-bits, and if an implementation supports
 * fewer than 256 priority levels then low-order bits of these fields are RAZ."
 * 
 * In the K70 Sub-Family Reference Manual in section "3.2.2.1 Interrupt
 * priority levels" on page 85, it states, "This device supports 16 priority
 * levels for interrupts.  Therefore, in the NVIC each source in the IPR
 * registers contains 4 bits."  The diagram that follows goes on to confirm
 * that only the high-order 4 bits of each 8 bit field is used.  It doesn't
 * explicitly mention the System Handler (like the SVC handler) priorities,
 * but they should be handled consistently with the interrupt priorities.
 * 
 * It is important to set the SVCall priority appropriately to allow
 * or disallow interrupts while the SVCall handler is running. */

void svcInit_SetSVCPriority(unsigned char priority) {
	if(priority > SVC_MaxPriority)
		return;

	SCB_SHPR2 = (SCB_SHPR2 & ~SCB_SHPR2_PRI_11_MASK) |
			SCB_SHPR2_PRI_11(priority << SVC_PriorityShift);
}

void pendSVInit_SetpendSVPriority(unsigned char priority) {
	if(priority > SVC_MaxPriority)
		return;

	SCB_SHPR3 = (SCB_SHPR3 & ~SCB_SHPR3_PRI_14_MASK) |
			SCB_SHPR3_PRI_14(priority << SVC_PriorityShift);
}

void svcHandlerInC(struct frame *framePtr);

/* Exception return behavior is detailed in B1.5.8 on page B1-652 of the
 * ARM�v7-M Architecture Reference Manual, ARM DDI 0403Derrata 2010_Q3
 * (ID100710) */

/* When an SVC instruction is executed, the following steps take place:
 * (1) A stack frame is pushed on the current stack (either the main or
 *     the process stack, depending on how the system is configured)
 *     containing the current values of R0-R3, R12, LR, the return
 *     address for the SVC instruction, xPSR, and, if appropriate, the
 *     floating point registers,
 * (2) An appropriate value is loaded into the LR register indicating
 *     that a stack frame is present on the stack (this will cause a
 *     special return sequence later when this value is loaded into
 *     the PC),
 * (3) Values of R0-R3 and R12 are no longer valid,
 * (4) The PC is set to the address in the interrupt vector table for
 * 	   the interrupt service routine for the SVC instruction,
 * (5) The processor switches to Handler mode (code execution in
 *     Handler mode is always privileged),
 * (6) The xPSR is set to indicate appropriate SVC state,
 * (7) The processor switches to the main stack by now using the main
 * 	   stack pointer.
 *     
 * These steps are discussed in detail in the pseudo-code given for
 * processor action ExceptionEntry() on page B1-643 of the ARM�v7-M
 * Architecture Reference Manual, ARM DDI 0403Derrata 2010_Q3
 * (ID100710).  ExceptionEntry() invokes PushStack() and
 * ExceptionTaken() on page B1-643. */

/* The following assembler function makes R0 point to the top-of-stack
 * for the stack that was current (either the main or the process stack)
 * before the SVC interrupt service routine began running.  This is
 * where the stack frame was stored by the SVC instruction.  Then, this
 * function calls the svcHandlerInC function.  Note that when a C
 * function is called, R0 contains the first parameter.  Therefore, the
 * first parameter passed to svcHandlerInC is a pointer to the
 * top-of-stack of the stack containing the stack frame. */

#ifdef __GNUC__
void __attribute__((naked)) svcHandler(void) {
	__asm("\n\
            tst		lr, #4\n\
			ite		eq\n\
			mrseq	r0, msp\n\
			mrsne	r0, psp\n\
			push	{lr}\n\
			bl		svcHandlerInC\n\
			pop		{pc}\n\
			");
}
#else
__asm void svcHandler(void) {
	tst		lr, #4
	mrseq	r0, msp
	mrsne	r0, psp
	push	lr
	bl		svcHandlerInC
	pop		pc
}
#endif

void svcHandlerInC(struct frame *framePtr) {
	if (MYFAT_DEBUG){
		printf("Entering svcHandlerInC\n");
		printf("framePtr = 0x%08x\n", (unsigned int)framePtr);
		/* framePtr->returnAddr is the return address for the SVC interrupt
		 * service routine.  ((unsigned char *)framePtr->returnAddr)[-2]
		 * is the operand specified for the SVC instruction. */
		printf("SVC operand = %d\n",
				((unsigned char *)framePtr->returnAddr)[-2]);
	}
	switch(((unsigned char *)framePtr->returnAddr)[-2]) {
		case SVC_FOPEN:
			if (MYFAT_DEBUG){
				printf("FOPEN\n");
			}
			framePtr->returnVal = myfopen(framePtr->descr,
					framePtr->filename, framePtr->mode);
			break;
		case SVC_FCLOSE:
			if (MYFAT_DEBUG){
			printf("FCLOSE\n");
			}
			framePtr->returnVal = myfclose(framePtr->descrf);
			break;
		case SVC_CREATE:
			if (MYFAT_DEBUG){
				printf("CREATE\n");
			}
			framePtr->returnVal = mycreate(framePtr->arg0);
			break;
		case SVC_DELETE:
			if (MYFAT_DEBUG){
				printf("DELETE\n");
			}
			framePtr->returnVal = mydelete(framePtr->arg0);
			break;
		case SVC_FGETC:
			if (MYFAT_DEBUG){
				printf("FGETC\n");
			}
			framePtr->returnVal = myfgetc(framePtr->descrf, framePtr->filename);
			break;
		case SVC_FPUTC:
			if (MYFAT_DEBUG){
				printf("FPUTC\n");
			}
			framePtr->returnVal = myfputc(framePtr->descrf, framePtr->bufp);
			break;
		case SVC_FPUTS:
			if (MYFAT_DEBUG){
				printf("FPUTS\n");
			}
			framePtr->returnVal = myfputs(framePtr->descrf, framePtr->filename, framePtr->arg2);
			break;
		case SVC_FGETS:
			if (MYFAT_DEBUG){
				printf("FGETS\n");
			}
			framePtr->returnVal = myfgets(framePtr->descrf, framePtr->filename, framePtr->arg2);
			break;
		case SVC_MALLOC:
			if (MYFAT_DEBUG){
				printf("MALLOC\n");
			}
			framePtr->returnVal = myMalloc(framePtr->unsInt);
			break;
		case SVC_FREE:
			if (MYFAT_DEBUG){
				printf("FREE\n");
			}
			framePtr->returnVal = myFreeErrorCode(framePtr->arg0);
			break;
		case SVC_DIR_LS:
			framePtr->returnVal = dir_ls(framePtr->arg0);
			break;
		case SVC_SETTIME:
			framePtr->returnVal = set_time(framePtr->arg0);
			break;
		case SVC_PDBONESHOT:
			framePtr->returnVal = pdb0_one_shot_timer(framePtr->arg0);
			break;
		case SVC_SPAWN:
			framePtr->returnVal = spawn(framePtr->arg0, framePtr->arg1, framePtr->arg2, framePtr->arg3);
			break;
		case SVC_YIELD:
			yield();
			framePtr->returnVal = NULL; //TODO: yield and block do not return a value. is this syntax OK?
			break;
		case SVC_BLOCK:
			block();
			framePtr->returnVal = NULL;
			break;
		case SVC_WAKE:
			framePtr->returnVal = wake(framePtr->arg0);
			break;
		case SVC_KILL:
			framePtr->returnVal = kill(framePtr->arg0);
			break;
		case SVC_WAIT:
			wait(framePtr->arg0);
			framePtr->returnVal = NULL;
			break;
		default:
			if (MYFAT_DEBUG){
				printf("Unknown SVC has been called\n");
			}
		}
	if (MYFAT_DEBUG){
		printf("Exiting svcHandlerInC\n");
	}
}

/*Routines for safely enabling and disabling interrupts across the OS.*/
void disable_interrupts(void){
	__asm("cpsid i");
	g_interrupt_count ++;
}

void enable_interrupts(void){
	g_interrupt_count --;
	if (g_interrupt_count == 0){
		g_interrupt_count = 0;
		__asm("cpsie i");
	}
}
