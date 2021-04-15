#include "flexTimer.h"
#include "devices.h"

//MS-DOS epoch is 00:00 Jan 1 1980

long long curTime;

int set_time(long long newTime){
     /* Disable interrupts (PRIMASK is set) */
     __asm("cpsid i");
     curTime = newTime;
     /* Allows interrupts (PRIMASK is cleared) */
     __asm("cpsie i");
     flexTimer0Init(1875);
     flexTimer0Start();
     return 0;
}

void date_time_incr(){
     /* Disable interrupts (PRIMASK is set) */
     __asm("cpsid i");
     curTime += 1;
     /* Allows interrupts (PRIMASK is cleared) */
     __asm("cpsie i");
}