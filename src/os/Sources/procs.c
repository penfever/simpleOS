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
#include "delay.h"
#include "MK70F12.h"

char* null_array = NULL;

struct pcb* currentPCB = NULL;

struct pcb* spawnPCB = NULL;

struct stateString stateStr[] = { {"running", 0},
                                  {"ready", 1},
                                  {"blocked", 2},
                                  {"kill pending", 3}
};

pid_t maxPid = 0;

uint32_t g_curPCBCount = 0;

int spawn(int main(int argc, char *argv[]), int argc, char *argv[], struct spawnData* thisSpawnData){
     disable_interrupts();
     g_curPCBCount ++; //increment current PCB count
     struct pcb* returnPCB = myMalloc(sizeof(struct pcb));
     spawnPCB = returnPCB;
     returnPCB->pid = get_next_free_pid();
     *(thisSpawnData->spawnedPidPtr) = returnPCB->pid; //pass copy of new pid back to caller
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
          returnPCB->nextPCB = currentPCB;
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
    	 enable_interrupts();
         return E_CMD_NOT_FOUND;
     }
     //TODO: size of stack should be rounded up to mult of 8
     returnPCB->state = ready;
     returnPCB->stackSize = thisSpawnData->stackSize;
     returnPCB->killPending = FALSE;
     returnPCB->runTimeInSysticks = 0;
     returnPCB->procStackBase = (uint32_t*)myMalloc(returnPCB->stackSize); //TODO: switch to pcb_malloc. pointer to base of memory. This will never change. We use this address to free the memory later.
     uint32_t currAddr = (uint32_t)(returnPCB->procStackBase); //Copy pointer address to avoid changing procStackBase when procStackCur changes
     currAddr += returnPCB->stackSize - 4; //Moves "pointer" stacksize - 4 bytes
     returnPCB->procStackCur = (uint32_t*)currAddr; //procStackCur now addresses TOP of memory, since stack grows down 
     /*malloc space for argc and argv, copy them, assign streams*/
     myfopen(&io_dev, "dev_UART2", 'w'); //open stdin/stdout device
     returnPCB->malArgc = argc;
     if (argv == NULL){
    	 returnPCB->malArgv = NULL;
     }
     else {
          returnPCB->malArgv = (char **)myMalloc((argc + 1) * sizeof(char *)); 
          for (int i = 0; i < argc; i++){
               int argSize = strlen(argv[i]);
               returnPCB->malArgv[i] = (char*)myMalloc(argSize+1);
               strcpy(returnPCB->malArgv[i], argv[i]);
          }
     }
     /*Manipulate stack to resemble systick interrupt.*/
     returnPCB->procStackCur -= 22; //TODO: WAS 23
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
     
     *(returnPCB->procStackCur++) = 0xFFFFFFF9; //PER AN10, Thread mode, main stack.
     
     *(returnPCB->procStackCur++) = (uint32_t)returnPCB->malArgc; //R0
     
     *(returnPCB->procStackCur++) = (uint32_t)returnPCB->malArgv; //R1
     
     *(returnPCB->procStackCur++) = 2; //R2
     
     *(returnPCB->procStackCur++) = 3; //R3
     
     *(returnPCB->procStackCur++) = 12; //R12
     
     *(returnPCB->procStackCur++) = (uint32_t)set_kill; //LR (R14) is where you will go if the program you invoke returns. This code sends to the function pcb_destructor
//     if (returnPCB->pid == SHELLPID){
//         *(returnPCB->procStackCur++) = (uint32_t)shell; //if PID is 1, shell goes here
//     }
//     else{
     *(returnPCB->procStackCur++) = (uint32_t)main; //otherwise, cmd goes here
//     }
     *(returnPCB->procStackCur) = 0x01000000; /* xPSR , "bottom" of stack 
     IN BINARY
     00000001000000000000000000000000
     Could also be 1 << 24
     */
     returnPCB->procStackCur -= 22;
     spawnPCB = NULL;
     enable_interrupts();
     return 0;
}

void set_kill(void){
	kill(currentPCB->pid);
	while(TRUE){
		;
	}
}

int kill(pid_t targetPid){
     if (targetPid == 1 || currentPCB == NULL){
          return E_NOINPUT; //shell is 1, and shell cannot be killed
     }
     disable_interrupts();
     struct pcb* walkPCB = currentPCB;
     do{
    	  /*Wake the parent (shell)*/
          if (walkPCB->pid == SHELLPID && walkPCB->state == blocked){
               wake(SHELLPID); 
          }
          else if (walkPCB->pid == targetPid){
               walkPCB->killPending = TRUE;
               break;
          }
          walkPCB = walkPCB->nextPCB;
     }while (currentPCB->pid != walkPCB->pid); //TODO: as currently written, this does not send an error if kill cannot find targetPid
     blockPid(targetPid);
     enable_interrupts();
	return 0;
}

int pcb_destructor(struct pcb* thisPCB){
     int err = 0;
     /*disconnect PCB from the chain*/
     disable_interrupts();
     struct pcb* lastPCB = thisPCB->nextPCB;
     while (lastPCB->nextPCB != thisPCB){
          lastPCB = lastPCB->nextPCB;
     }
     lastPCB->nextPCB = thisPCB->nextPCB;
     enable_interrupts();
     /*close all streams*/
     for (int i = 0; i < MAXOPEN; i++){
          thisPCB->openFiles[i].deviceType = UNUSED;
     }

     //TODO: free any memory this process has malloced

     /*free process stack*/
     if ((err = myFreeErrorCode(thisPCB->procStackBase)) != 0){
  		if (MYFAT_DEBUG_LITE || MYFAT_DEBUG){
  			printf("pcb_destructor error #%d \n", err);
  		}
     }
     /*free all argv and argc*/
     for (int i = 0; i < thisPCB->malArgc; i++){
          if (thisPCB->malArgv[i] != NULL){
               if ((err = myFreeErrorCode(thisPCB->malArgv[i])) != 0){
					if (MYFAT_DEBUG_LITE || MYFAT_DEBUG){
						printf("pcb_destructor error #%d \n", err);
					}
               }
          }
     }
     if ((err = myFreeErrorCode(thisPCB->malArgv)) != 0){
  		if (MYFAT_DEBUG_LITE || MYFAT_DEBUG){
  			printf("pcb_destructor error #%d \n", err);
  		}
     }
     /*free the pcb itself*/
     if ((err = myFreeErrorCode(thisPCB)) != 0){
  		if (MYFAT_DEBUG_LITE || MYFAT_DEBUG){
  			printf("pcb_destructor error #%d \n", err);
  		}
     }
     return 0;
}

pid_t getCurrentPid(void){
	if (spawnPCB != NULL){
		return spawnPCB->pid;
	}
	else if (spawnPCB == NULL && currentPCB == NULL){
		return SHELLPID;
	}
	return currentPCB->pid;
}

pid_t get_next_free_pid(void){
     maxPid ++;
     walk_pid_table_pid(maxPid);
     return maxPid;
}

void walk_pid_table_pid(pid_t maxPid){
     struct pcb* walkPCB = currentPCB;
     if (walkPCB == NULL){
         return;
     }
     pid_t curPid = walkPCB->pid;
     if (maxPid == curPid){
          maxPid ++;
     }
     disable_interrupts();
     walkPCB = walkPCB->nextPCB;
     enable_interrupts();
          if (maxPid == walkPCB->pid){
               maxPid ++;
               walk_pid_table_pid(maxPid);
          }
     return;
}

void yield(void){
     SCB_ICSR |= SCB_ICSR_PENDSVSET_MASK;
}

void block(void){
     currentPCB->state = blocked;
}

int blockPid(pid_t targetPid){
     struct pcb* walkPCB = currentPCB;
     disable_interrupts();
     do{
          if (walkPCB->pid == targetPid){
               walkPCB->state = blocked;
               break;
          }
          walkPCB = walkPCB->nextPCB;
     }while (currentPCB->pid != walkPCB->pid); //TODO: as currently written, this does not send an error if wake cannot find targetPid
     enable_interrupts();
	return 0;
}

int wake(pid_t targetPid){
     struct pcb* walkPCB = currentPCB;
     disable_interrupts();
     do{
          if (walkPCB->pid == targetPid){
               walkPCB->state = ready;
               break;
          }
          walkPCB = walkPCB->nextPCB;
     }while (currentPCB->pid != walkPCB->pid); //TODO: as currently written, this does not send an error if wake cannot find targetPid
     enable_interrupts();
	return 0;
}

/*Sets the currently running process to wait.*/
void wait(pid_t targetPid){
	/*Create a new pcb struct for the purposes of walking the linked list.*/
     struct pcb* walkPCB = currentPCB;
     uint8_t found = FALSE;
     do{
          if (walkPCB->pid == targetPid){
               found = TRUE;
          }
          else{
              walkPCB = walkPCB->nextPCB;
              /*If we have come full circle, breakpoint*/
              if (currentPCB->pid == walkPCB->pid){
            	  __BKPT();
              }
          }
     }while (!found);
     while(walkPCB->killPending == FALSE){
         block();
     }
     return;
}

void* rr_sched(void* sp){
  	 int err = 0;
  	 
     /*Check whether one process, current or next, is waiting to be killed -- if so, destroys it.*/
     struct pcb *killPCB = NULL;
     if (currentPCB->killPending == TRUE){
    	 killPCB = currentPCB;
         currentPCB = currentPCB->nextPCB; //Avoids leaving currentPCB pointing to a "stranded" PCB
     }
     else if (currentPCB->nextPCB->killPending == TRUE){
    	 killPCB = currentPCB->nextPCB;
     }
     if (killPCB != NULL){
	     	disable_interrupts();
	    	g_firstrun_flag = 0; //we should not save state of a process we are killing
	     	g_curPCBCount --; //Decrement length of PCB chain
	     	if ((err = pcb_destructor(killPCB)) != 0){
	     		if (MYFAT_DEBUG_LITE || MYFAT_DEBUG){
	     			printf("pcb_destructor error #%d \n", err);
	     		}
	     	}
	     	killPCB = NULL;
	     	enable_interrupts();
     }
     //create a pointer to the current process, and a copy of its pid
     struct pcb *schedPCB = currentPCB; 
     uint32_t currentPid = currentPCB->pid;
     
     /*g_firstrun_flag checks if it is the first quantum interrupt, does not save state if flag is true.*/
     if (g_firstrun_flag != 0){
          schedPCB->procStackCur = (uint32_t *)sp; //this saves process state
          schedPCB->runTimeInSysticks ++; //TODO: fix this timer
          if (schedPCB->state != blocked){
              schedPCB->state = ready;
          }
          schedPCB = schedPCB->nextPCB;
     }
     else{
    	 g_firstrun_flag = 1;
     }
     
     /*Check whether process is blocked. If so, skip it.*/
     while (schedPCB->state == blocked){
//          if (currentPid == schedPCB->pid){
//        	  wake(SHELLPID); //if all processes are blocked, wake the shell
//          }
          schedPCB = schedPCB->nextPCB;
     }
     
     //convert next process scheduled to run to running state
     schedPCB->state = running;
     currentPCB = schedPCB;
     sp = (void *)schedPCB->procStackCur; //sp gets saved version of stack pointer
     currentPid = schedPCB->pid;
     return sp;
}
