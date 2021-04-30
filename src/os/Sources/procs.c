#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "procs.h"
#include "svc.h"
#include "SDHC_FAT32_Files.h"
#include "breakpoint.h"
#include "mymalloc.h"
#include "myerror.h"
#include "devices.h"
#include "intSerialIO.h"
#include "dateTime.h"
#include "simpleshell.h"

char* null_array = NULL;

struct pcb* currentPCB = NULL;

pid_t maxPid = 0;

int spawn(int main(int argc, char *argv[]), int argc, char *argv[], struct spawnData* thisSpawnData){
     disable_interrupts();

     struct pcb* returnPCB = myMalloc(sizeof(struct pcb));
     /*Null out all openFiles memory (this avoids junk data causing file errors)*/
     for (int i = 0; i < MAXOPEN; i++){
    	 returnPCB->openFiles[i].clusterAddr = 0;
    	 returnPCB->openFiles[i].cursor = 0;
    	 returnPCB->openFiles[i].deviceType = UNUSED;
    	 returnPCB->openFiles[i].fileName = NULL;
    	 returnPCB->openFiles[i].fileSize = 0;
    	 returnPCB->openFiles[i].minorId = 0;
    	 returnPCB->openFiles[i].mode = 0;
     }
     //Now that memory is allocated, we link our new PCB into the chain.
     if (currentPCB == NULL){
          currentPCB = returnPCB;
          currentPCB->nextPCB = returnPCB;
     }
     else if(currentPCB->nextPCB == currentPCB){
          currentPCB->nextPCB = returnPCB;
     }
     else{
          struct pcb* lastPCB = currentPCB->nextPCB;
          while (lastPCB->nextPCB != currentPCB){
               lastPCB = lastPCB->nextPCB;
          }
          returnPCB->nextPCB = currentPCB;
          lastPCB->nextPCB = returnPCB;
     }
     /*Configure PCB struct*/

     returnPCB->procName = thisSpawnData->procName;
     if (returnPCB->procName == NULL){
          return E_CMD_NOT_FOUND;
     }
     returnPCB->pid = *(thisSpawnData->spawnedPidPtr);
     returnPCB->state = ready;
     returnPCB->stackSize = thisSpawnData->stackSize;
     returnPCB->killPending = FALSE;
     returnPCB->runTimeInSysticks = 0;
     returnPCB->procStackBase = (uint32_t*)myMalloc(returnPCB->stackSize); //TODO: switch to pcb_malloc. pointer to base of memory. This will never change. We use this address to free the memory later.
     uint32_t currAddr = (uint32_t)(returnPCB->procStackBase); //Copy pointer address to avoid changing procStackBase when procStackCur changes
     currAddr += returnPCB->stackSize - 4; //Moves "pointer" 99996 bytes
     returnPCB->procStackCur = (uint32_t*)currAddr; //procStackCur now addresses TOP of memory, since stack grows down 
     /*malloc space for argc and argv, copy them, assign streams*/
     myfopen(&io_dev, "dev_UART2", 'w'); //open stdin/stdout device
     returnPCB->malArgc = myMalloc(sizeof(uint32_t));
     *(returnPCB->malArgc) = argc;
     returnPCB->malArgv = (char **)myMalloc((argc + 1) * sizeof(char *)); 
     if (argv == NULL){
    	 returnPCB->malArgv = NULL;
     }
     else {
          for (int i = 0; i < argc; i++){
               int argSize = strlen(argv[i]);
               returnPCB->malArgv[i] = (char*)myMalloc(argSize);
               memcpy(argv[i], returnPCB->malArgv[i], argSize);
          }
     }
     /*Manipulate stack to resemble systick interrupt.*/
     returnPCB->procStackCur -= 23;
     //Assuming procStackCur points to a valid word, and we don't need the reserved word, we jump down 23 words in memory and build up from there
     //currAddr = (uint32_t)returnPCB->procStackCur;
     *(returnPCB->procStackCur++) = 0; //Word with SVCALLACT & SVCALLPENDED bits
     
     *(returnPCB->procStackCur++) = 4; //R4
     
     *(returnPCB->procStackCur++) = 5; //R5
     
     *(returnPCB->procStackCur++) = 6; //R6

     *(returnPCB->procStackCur) = (uint32_t)(returnPCB->procStackCur+5); //R7 must point to the EMPTY WORD just "above" R11, per lecture 12 at 2:50:14
     
     returnPCB->procStackCur++;
     
     *(returnPCB->procStackCur++) = 8; //R8
     
     *(returnPCB->procStackCur++) = 9; //R9
     
     *(returnPCB->procStackCur++) = 10; //R10
     
     *(returnPCB->procStackCur++) = 11; //R11
     
     *(returnPCB->procStackCur++) = 0; //empty word begins Stack Contents Pushed by Entry Code to SysTick Handler
     
     *(returnPCB->procStackCur++) = 0; //copyofsp will be overwritten when scheduler returns
     
     *(returnPCB->procStackCur++) = 0; //empty word
     
     *(returnPCB->procStackCur++) = 4; //value of R4 for new process (arbitrary)
     
     *(returnPCB->procStackCur++) = 7; //value of R7 for new process (arbitrary)
     
     *(returnPCB->procStackCur++) = 0xFFFFFFF9; //PER AN10, Thread mode, main stack. It might be that LR gets magic number automatically.
     
     *(returnPCB->procStackCur++) = returnPCB->malArgc; //R0
     
     *(returnPCB->procStackCur++) = returnPCB->malArgv; //R1
     
     *(returnPCB->procStackCur++) = 2; //R2
     
     *(returnPCB->procStackCur++) = 3; //R3
     
     *(returnPCB->procStackCur++) = 12; //R12
     
     *(returnPCB->procStackCur++) = (uint32_t)set_kill; //LR (R14) is where you will go if the program you invoke returns. This code sends to the function pcb_destructor
     if (returnPCB->pid == SHELLPID){
         *(returnPCB->procStackCur++) = (uint32_t)shell; //if PID is 1, shell goes here
     }
     else{
         *(returnPCB->procStackCur++) = (uint32_t)main; //otherwise, cmd goes here
     }
     *(returnPCB->procStackCur) = 0x01000000; /* xPSR , "bottom" of stack 
     IN BINARY
     00000001000000000000000000000000
     Could also be 1 << 24
     */
     returnPCB->procStackCur -= 22;
     
     enable_interrupts();
     return 0;
}

void set_kill(void){
	return kill(currentPCB->pid);
}

int kill(pid_t targetPid){
     if (targetPid == 1 || currentPCB == NULL){
          return E_NOINPUT; //shell is 1, and shell cannot be killed
     }
     disable_interrupts();
     struct pcb* walkPCB = currentPCB;
     do{
          if (walkPCB->pid == targetPid){
               walkPCB->killPending = TRUE;
               break;
          }
          walkPCB = walkPCB->nextPCB;
     }while (currentPCB->pid != walkPCB->pid); //TODO: as currently written, this does not send an error if kill cannot find targetPid
     yield();
     while(TRUE){
    	 ;
     }
     enable_interrupts();
	return 0;
}

int pcb_destructor(struct pcb* thisPCB){
     disable_interrupts();
     int err = 0;
     /*disconnect PCB from the chain*/
     struct pcb* lastPCB = thisPCB->nextPCB;
     while (lastPCB->nextPCB != currentPCB){
          lastPCB = lastPCB->nextPCB;
     }
     lastPCB->nextPCB = thisPCB->nextPCB;
     /*close all streams*/
     for (int i = 0; i < MAXOPEN; i++){
          thisPCB->openFiles[i].deviceType = UNUSED;
     }

     //TODO: free any memory this process has malloced

     /*free process stack*/
     if ((err = myFreeErrorCode(thisPCB->procStackBase)) != 0){
          return err;
     }
     /*free all argv and argc*/
     for (int i = 0; i <= thisPCB->malArgc; i ++){
          if (thisPCB->malArgv[i] != NULL){
               if ((err = myFreeErrorCode(thisPCB->malArgv[i])) != 0){
                    return err;
               }
          }
     }
     if ((err = myFreeErrorCode(thisPCB->malArgv)) != 0){
          return err;
     }
     if ((err = myFreeErrorCode(thisPCB->malArgc)) != 0){
          return err;
     }
     /*free the pcb itself*/
     if ((err = myFreeErrorCode(thisPCB)) != 0){
          return err;
     }
     enable_interrupts();
     return;
}

pid_t getCurrentPid(void){
	return currentPCB->pid;
}

pid_t get_next_free_pid(void){
     maxPid ++;
     walk_pid_table_pid(maxPid);
     return maxPid;
}

void walk_pid_table_pid(pid_t maxPid){
     disable_interrupts();
     struct pcb* walkPCB = currentPCB;
     if (walkPCB == NULL){
         enable_interrupts();
         return;
     }
     pid_t curPid = walkPCB->pid;
     if (maxPid == curPid){
          maxPid ++;
     }
     walkPCB = walkPCB->nextPCB;
     while(curPid != walkPCB->pid){
          if (maxPid == walkPCB->pid){
               maxPid ++;
               walk_pid_table_pid(maxPid);
          }
     }
     enable_interrupts();
     return;
}

void yield(void){
     ;
}

void block(void){
     ;
}

int wake(pid_t targetPid){
	return 0;
}

void wait(pid_t targetPid){
     ;
}

void* rr_sched(void* sp){
     //disable_interrupts();
     struct pcb *schedPCB = currentPCB; //create a pointer to the global for us to work with
     uint32_t currentPid = currentPCB->pid;
     /*during first quantum interrupt, do not save state.*/
     if (g_firstrun_flag != 0){
          schedPCB->procStackCur = (uint32_t *)sp; //this saves process state
          schedPCB->runTimeInSysticks ++; //TODO: fix this timer
          schedPCB->state = ready;
          schedPCB = schedPCB->nextPCB;
     }
     else{
    	 g_firstrun_flag = 1;
     }
     while (schedPCB->state != ready){
          if (currentPid == schedPCB->pid){
               error_checker(E_FREE_PERM); //we have gone all the way around and not found a ready process. All blocked?
          }
          schedPCB = schedPCB->nextPCB;
     }
     //convert next process scheduled to run to running state
     schedPCB->state = running;
     sp = (void *)schedPCB->procStackCur; //sp gets saved version of stack pointer
     currentPid = schedPCB->pid;
     //loop 1: terminate all kill_pending processes -- this loop should us back to where we started, the process whose quantum just elapsed
     while (currentPid != schedPCB->pid){
          if (schedPCB->killPending == TRUE){
               int err = 0;
               if ((err = pcb_destructor(schedPCB)) != 0){
                    if (MYFAT_DEBUG){
                         printf("pcb_destructor error #%d \n", err);
                    }
               }
          }
     }
     //enable_interrupts();
     return sp;
}
