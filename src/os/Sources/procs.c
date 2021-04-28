#include "devices.h"
#include "svc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct pcb* currentPCB = NULL;

/*temporary*/
struct pcb op_sys;
struct pcb op_sys = 
{
		"OS", //NAME
		0, //PID
		{ //NULL INIT STRUCT "STREAM" ARRAY
				{0},
				{0},
				{0},
				{0},
				{0},
				{0},
				{0},
				{0}
		}
};

pid_t maxPid = 0;

void* temp_sched(void* sp){
     return sp;
}

void* rr_sched(void* sp){
     disable_interrupts();
     /*during first quantum interrupt, do not save state.*/
     pcb* schedPCB = currentPCB; //create a pointer to the global for us to work with
     if (g_systick_count != 1){
          schedPCB->stackPointer = sp;
          schedPCB.runTimeInSysticks ++; //TODO: fix this timer, it's not going to be accurate if a process yields etc
          schedPCB.status = ready;
          schedPCB = schedPCB->next;
     }
     //loop 1: terminate all kill_pending processes -- this loop should us back to where we started, the process whose quantum just elapsed
     while (currentPid != schedPCB.pid){
          if (schedPCB.killPending == TRUE){
               pcb_destructor(schedPCB);
          }
     }
     schedPCB = schedPCB->next;
     while (schedPCB.status != ready){
          if (currentPid == schedPCB.pid){
               return E_PID; //we have gone all the way around and not found a ready process. All blocked?
          }
          schedPCB = schedPCB->next;
     }
     //convert next process scheduled to run to running state
     schedPCB.status = running;
     sp = schedPCB.stackPointer; //sp gets saved version of stack pointer
     currentPid = schedPCB.pid;
     pcb* schedPCB = schedPCB;
     enable_interrupts();
     return sp;
}

char* get_proc_name(int main(int argc, char* argv[])){
     for (int i = 0; i < NUMCOMMANDS; i++){
          if (commands[i].functionp == &main){
          return commands[i].name;
          }
     }
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
     returnPCB.pid = thisSpawn->spawnedPidPtr;
     returnPCB.state = ready;
     returnPCB.stackSize = thisSpawn->stackSize;
     returnPCB->procStackBase = (uint32_t*)pcb_malloc(returnPCB.stackSize); //pointer to base of memory. This will never change. We use this address to free the memory later.
     returnPCB->procStackBase += (returnPCB.stackSize/sizeof(uint32_t*)) - 1; //now points to TOP of memory, since stack grows down. TODO: check this math
     returnPCB->procStackCur = procStackBase;
     returnPCB.killPending = FALSE;
     returnPCB.runTimeInSysticks = 0;
     returnPCB->nextPCB = nextPCB;

     /*malloc space for argc and argv, copy them, assign streams*/
     returnPCB->openFiles[0] = the uart //TODO: stdin etc
     returnPCB->malArgc = myMalloc(sizeof(uint32_t));
     returnPCB->malArgc = argc; //TODO: syntax?
     if (argv == NULL){
          returnPCB->malArgv == NULL;
     }
     else{
          returnPCB->malArgv = myMalloc(sizeof(argv[]));
          memcpy(argv, returnPCB->malArgv, sizeof(argv));
     }

     /*Manipulate stack to resemble systick interrupt*/

     procStackCur -= 23; //Assuming procStackCur points to a valid word, and we don't need the reserved word, we jump down 23 words in memory and build up from there
     *(procStackCur++) = 0; //Word with SVCALLACT & SVCALLPENDED bits
     *(procStackCur++) = 4; //R4
     *(procStackCur++) = 5; //R5
     *(procStackCur++) = 6; //R6
     *(procStackCur++) = &(topOfMemory - 4); //R7 in stack contents pushed must get the address in memory of WORD WITH SVCCALLACTIVE AND PENDED (sp).
     *(procStackCur++) = 8; //R8
     *(procStackCur++) = 9; //R9
     *(procStackCur++) = 10; //R10
     *(procStackCur++) = 11; //R11
     *(procStackCur++) = 0; //empty word begins Stack Contents Pushed by Entry Code to SysTick Handler
     *(procStackCur++) = 0; //copyofsp will be overwritten when scheduler returns
     *(procStackCur++) = 0; //empty word
     *(procStackCur++) = 4; //value of R4 for new process (arbitrary)
     *(procStackCur++) = 7; //value of R7 for new process (arbitrary)
     *(procStackCur++) = 0xFFFFFFF9; //PER AN10, Thread mode, main stack. It might be that LR gets magic number automatically.
     *(procStackCur++) = &malArgc; //R0
     *(procStackCur++) = &malArgv; //R1
     *(procStackCur++) = 2; //R2
     *(procStackCur++) = 3; //R3
     *(procStackCur++) = 12; //R12
     *(procStackCur++) = pcb_destructor; //LR (R14) is where you will go if the program you invoke returns. This code sends to the function pcb_destructor
     *(procStackCur++) = returnPCB->procname; //PC gets main I think? Jamie says: you just take whatever the name of the command is, and you stick it into this word.
     *(procStackCur++) =  ? // xPSR (see below), "bottom" of stack
     //Now that memory is allocated, we link our new PCB into the chain, EI and return.
     if (currentPCB == NULL){
          currentPCB = &spawnedPCB;
     }
     else if(currentPCB->next == currentPCB){
          currentPCB->next = &returnPCB;
     }
     else{
          struct pcb* lastPCB = currentPCB->next;
          while (lastPCB->next != currentPCB){
               lastPCB = lastPCB->next;
          }
          returnPCB->next = &currentPCB;
          lastPCB->next = &returnPCB;
     }
     enable_interrupts();
     return 0;
}

int kill(pid_t targetPid){
     //if they try to kill the shell, return E_NOINPUT
     //if there's no targetPid in struct, return E_PID
     //else
          thisPCB.killPending = TRUE;
}


void pcb_destructor(struct pcb* thisPCB){
     /*
     when a process ends (naturally or when
     killed), any open streams need to be closed and the storage used for its PCB and for its stack must be reclaimed.  In addition, all dynamically-allocated (malloc'ed) storage owned by the process that is ending needs to be freed. How do we ensure this? Maybe by calling our pid_less malloc and freeing everything associated with that process's pid ...
     */
}

pid_t pid(void){
     disable_interrupts();
    for item in pidtable{
        if (currentPCB.state == ready){
             return currentPCB.pid;
        }
     }
     enable_interrupts();
}

pcb_struct* pid_struct(pid_t pid){
     disable_interrupts();
    for item in pidtable{
        if (currentPCB.pid == pid){
             return &currentPCB;
        }
     }
     enable_interrupts();
}

pid_t get_next_free_pid(void){
     maxPid ++;
     walk_pid_table_pid(maxPid);
     return maxPid;
}

void walk_pid_table_pid(maxPid){
     disable_interrupts();
     struct pcb* walkPCB = currentPCB;
     if (walkPCB == NULL){
          return;
     }
     pid_t curPid = walkPCB.pid;
     if (maxPid == curPid){
          maxPid ++;
     }
     walkPCB = walkPCB->next;
     while(curPid != walkPCB.pid){
          if (maxPid == walkPCB.pid){
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
     ;
}

void wait(pid_t targetPid){
     ;
}