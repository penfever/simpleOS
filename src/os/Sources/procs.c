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

/*temporary*/
struct pcb op_sys;

struct pcb* currentPCB = &op_sys;

pid_t maxPid = 0;

char* get_proc_name(int main(int argc, char* argv[])){ //TODO: NUMCOMMANDS won't compile here
//     for (int i = 0; i < NUMCOMMANDS; i++){
//          if (commands[i].functionp == &main){
//          return commands[i].name;
//          }
//     }
     return NULL;
}

int spawn(int main(int argc, char *argv[]), int argc, char *argv[], struct spawnData* thisSpawn){
     disable_interrupts();

     struct pcb* returnPCB = myMalloc(sizeof(struct pcb));

     /*Configure PCB struct*/

     returnPCB->procName = get_proc_name(main);
     if (returnPCB->procName == NULL){
          return E_CMD_NOT_FOUND;
     }
     returnPCB->pid = thisSpawn->spawnedPidPtr;
     returnPCB->state = ready;
     returnPCB->stackSize = thisSpawn->stackSize;
     returnPCB->procStackBase = (uint32_t*)myMalloc(returnPCB->stackSize); //TODO: switch to pcb_malloc. pointer to base of memory. This will never change. We use this address to free the memory later.
     returnPCB->procStackBase += (returnPCB->stackSize/sizeof(uint32_t*)) - 1; //now points to TOP of memory, since stack grows down. TODO: check this math
     returnPCB->procStackCur = returnPCB->procStackBase;
     returnPCB->killPending = FALSE;
     returnPCB->runTimeInSysticks = 0;
     //returnPCB->nextPCB = nextPCB; TODO: I don't know what this means

     /*malloc space for argc and argv, copy them, assign streams*/
     //returnPCB->openFiles[0] = ; //TODO: stdin etc the uart
     returnPCB->malArgc = myMalloc(sizeof(uint32_t));
     returnPCB->malArgc = argc; //TODO: syntax?
     if (argv == NULL){
          returnPCB->malArgv == NULL;
     }
     else{
          //returnPCB->malArgv = myMalloc(sizeof(argv)); //TODO: Find a way to get this syntax to work
          memcpy(argv, returnPCB->malArgv, sizeof(argv));
     }

     /*Manipulate stack to resemble systick interrupt. TODO: do I want to change the actual pointer here?*/
     returnPCB->procStackCur -= 23; //Assuming procStackCur points to a valid word, and we don't need the reserved word, we jump down 23 words in memory and build up from there
     *(returnPCB->procStackCur++) = 0; //Word with SVCALLACT & SVCALLPENDED bits
     *(returnPCB->procStackCur++) = 4; //R4
     *(returnPCB->procStackCur++) = 5; //R5
     *(returnPCB->procStackCur++) = 6; //R6
     *(returnPCB->procStackCur++) = (returnPCB->procStackCur + 5); //R7 must point to the EMPTY WORD just "above" R11, per lecture 12 at 2:50:14
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
     *(returnPCB->procStackCur++) = pcb_destructor; //LR (R14) is where you will go if the program you invoke returns. This code sends to the function pcb_destructor
     *(returnPCB->procStackCur++) = returnPCB->procName; //PC gets main I think? Jamie says: you just take whatever the name of the command is, and you stick it into this word.
     *(returnPCB->procStackCur++) = 0x01000100; /* xPSR , "bottom" of stack 
     IN BINARY
     00000001000000000000000100000000
     */
     //Now that memory is allocated, we link our new PCB into the chain, EI and return.
     if (currentPCB == NULL){
          currentPCB = &returnPCB;
     }
     else if(currentPCB->nextPCB == currentPCB){
          currentPCB->nextPCB = &returnPCB;
     }
     else{
          struct pcb* lastPCB = currentPCB->nextPCB;
          while (lastPCB->nextPCB != currentPCB){
               lastPCB = lastPCB->nextPCB;
          }
          returnPCB->nextPCB = &currentPCB;
          lastPCB->nextPCB = &returnPCB;
     }
     enable_interrupts();
     return 0;
}

int kill(pid_t targetPid){
     //if they try to kill the shell, return E_NOINPUT
     //if there's no targetPid in struct, return E_FREE_PERM
     //else
          //thisPCB.killPending = TRUE;
	return 0;
}


void pcb_destructor(struct pcb* thisPCB){
     /*
     when a process ends (naturally or when
     killed), any open streams need to be closed and the storage used for its PCB and for its stack must be reclaimed.  In addition, all dynamically-allocated (malloc'ed) storage owned by the process that is ending needs to be freed. How do we ensure this? Maybe by calling our pid_less malloc and freeing everything associated with that process's pid ...
     */
}

pid_t pid(void){
//     disable_interrupts();
//    for item in pidtable{
//        if (currentPCB.state == ready){
//             return currentPCB.pid;
//        }
//     }
//     enable_interrupts();
	return 0;
}

struct pcb* pid_struct(pid_t pid){
//     disable_interrupts();
//    for item in pidtable{
//        if (currentPCB.pid == pid){
//             return &currentPCB;
//        }
//     }
//     enable_interrupts();
	return 0;
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

void* temp_sched(void* sp){
     return sp;
}

void* rr_sched(void* sp){
     disable_interrupts();
     /*during first quantum interrupt, do not save state.*/
     struct pcb* schedPCB = currentPCB; //create a pointer to the global for us to work with
     uint32_t currentPid = currentPCB->pid;
     if (g_systick_count != 1){
          schedPCB->procStackCur = sp;
          schedPCB->runTimeInSysticks ++; //TODO: fix this timer, it's not going to be accurate if a process yields etc
          schedPCB->state = ready;
          schedPCB = schedPCB->nextPCB;
     }
     //loop 1: terminate all kill_pending processes -- this loop should us back to where we started, the process whose quantum just elapsed
     while (currentPid != schedPCB->pid){
          if (schedPCB->killPending == TRUE){
               pcb_destructor(schedPCB);
          }
     }
     schedPCB = schedPCB->nextPCB;
     while (schedPCB->state != ready){
          if (currentPid == schedPCB->pid){
               return E_FREE_PERM; //we have gone all the way around and not found a ready process. All blocked?
          }
          schedPCB = schedPCB->nextPCB;
     }
     //convert next process scheduled to run to running state
     schedPCB->state = running;
     sp = schedPCB->procStackCur; //sp gets saved version of stack pointer
     currentPid = schedPCB->pid;
     //pcb* schedPCB = schedPCB; TODO: what was this line trying to do?
     enable_interrupts();
     return sp;
}
