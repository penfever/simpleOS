#include "devices.h"

#ifndef _PROCS_H
#define _PROCS_H
#define NEWPROC_DEF 100000

typedef uint32_t pid_t;

struct spawnData {
     char* procName;
     uint32_t stackSize;
     pid_t *spawnedPidPtr;
};

enum procState{
     running = 0,
     ready = 1,
     blocked = 2
 };

struct pcb {
    char* procName;
    pid_t pid;
    enum procState state;
     /*Each PCB must have a pointer to the memory allocated for that process' stack.  That value will be the address returned when you allocate a new stack for the process.  It is the lowest address in the memory allocated for the stack.  It will never change during the lifetime of that process.  Let's call that location in the PCB the stackBaseAddress.*/
    uint32_t* procStackBase;
    /*Each PCB must have a stored SP which is the value of the SP when a process is preempted by the SysTick interrupt.  This stored SP must be initialized correctly when a new process is created.*/
    uint32_t* procStackCur;
    uint32_t stackSize;
    uint32_t runTimeInSysticks;
    uint8_t killPending;
     struct pcb* nextPCB;
     /*data owned by this process*/
     struct stream openFiles[MAXOPEN];
     uint32_t* malArgc;
     char** malArgv;
};

extern struct pcb* currentPCB;

/*Given an sp, and a pointer to pcb, scheduler saves copy of sp to an appropriate field inside the pcb. This function only saves the sp of the quantum-expired process. it saves it to the sp field in the pcb. Currently running PCB must always be the first item pointed to by the global in order for this to work.*/
void* rr_sched(void* sp);

char* get_proc_name(int main(int argc, char* argv[]));

/*spawn spawns a new process*/
int spawn(int main(int argc, char *argv[]), int argc, char *argv[], struct spawnData* thisSpawn);

/*kill sets the state of PCB with targetPid to terminate*/
int kill(pid_t targetPid);

/*kill calls this, but it can also be called when a process terminates. This function breaks down a PCB and frees its memory.
when a process ends (naturally or when killed), any open streams need to be closed 
and the storage used for its PCB and for its stack must be reclaimed.  In addition, 
all dynamically-allocated (malloc'ed) storage owned by the process that is ending 
needs to be freed. How do we ensure this? Maybe by calling our pid_less malloc and 
freeing everything associated with that process's pid ...
*/
int pcb_destructor(struct pcb* thisPCB);

/*Returns pid of current process*/
pid_t getCurrentPid(void);

/*determines next free pid number returns it as pid_t*/
pid_t get_next_free_pid(void);

void walk_pid_table_pid(pid_t maxPid);

void yield(void);

void block(void);

/* sets the targetPid process to ready state */
int wake(pid_t targetPid);

/*Process A waits for process B to complete*/
void wait(pid_t targetPid);
#endif
