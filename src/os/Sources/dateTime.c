#include "flexTimer.h"
#include "devices.h"
#include "simpleshell.h"
#include "dateTime.h"

//MS-DOS epoch is 00:00 Jan 1 1980

long long curTime;

struct months allMonths[] = {{"January", 0, 31}, 
              {"February", 1, 28}, 
              {"March", 2, 31},  
              {"April", 3, 30},  
              {"May", 4, 31},  
              {"June", 5, 31},  
              {"July", 6, 31}, 
              {"August", 7, 31}, 
              {"September", 8, 30}, 
              {"October", 9, 31}, 
              {"November", 10, 30}, 
              {"December", 11, 31}, 
              {"February", 12, 29}};

int set_time(long long *newTime){
     /* Disable interrupts (PRIMASK is set) */
     __asm("cpsid i");
     curTime = *newTime;
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

struct date_time get_time(){
	struct date_time curr_date;
	long long thisTime = curTime; //avoid further ticks
	long long sec_now = thisTime/1000;
	curr_date.msec = thisTime - sec_now*1000;
  //converts an integer value of seconds into a populated date_time struct
  int year_count = 0;
  while (sec_now > SECYEAR){
    year_count += 1;
    sec_now -= SECYEAR;
  }
  //check leap years
  for (int i = 1980; i < 1980 + year_count; i++){
    if (isleapyear(i)){
      sec_now -= SECDAY;
      if (sec_now < 0){
        sec_now += SECYEAR;
        year_count -= 1;
      }
    }
  }
  curr_date.year = 1980 + year_count;
  curr_date.day = 1;
  curr_date.hour = 0;
  curr_date.minute = 0;
  curr_date.clock = "";

  while (sec_now > SECDAY){
    sec_now -= SECDAY;
    curr_date.day += 1;
  }
  while (sec_now > SECHOUR){
    sec_now -= SECHOUR;
    curr_date.hour += 1;
  }
  while (sec_now > SECMIN){
    sec_now -= SECMIN;
    curr_date.minute += 1;
  }
  curr_date.second = sec_now;
  for (int i = 0; i < 11; i ++){
    if (curr_date.day > allMonths[i].offset){
      curr_date.day -= allMonths[i].offset;
      if (i == 1 && isleapyear(curr_date.year)){
        curr_date.day -= 1;
        if (curr_date.day <= 0) {
          curr_date.day += allMonths[i].offset;
          i++;
          curr_date.month = allMonths[i].month;
          break;
        }
      }
    }
    else {
      curr_date.month = allMonths[i].month;
      break;
    }
  }
  return curr_date;
}

void print_time(struct date_time curr_date){
     char output[128] = {'\0'};
     sprintf(output, "%s %d, %d " TIMESTAMP "\r\n", curr_date.month, curr_date.day, curr_date.year, curr_date.hour, curr_date.minute, curr_date.second, curr_date.msec, io_dev);
     SVC_fputs(io_dev, output, strlen(output));
}

//checks if a given integer, assumed to be a year, is a leap year
int isleapyear(int inyear){
    if(inyear % 400 == 0){
        return TRUE;
    }
    else if(inyear % 4 == 0 && inyear % 100 != 0){
        return TRUE;
    }
    else{
        return FALSE;
    }
}
