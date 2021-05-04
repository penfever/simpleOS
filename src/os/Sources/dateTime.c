#include "flexTimer.h"
#include "devices.h"
#include "simpleshell.h"
#include "dateTime.h"
#include "myerror.h"
#include "svc.h"
#include <stdio.h>
#include <string.h>

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
     disable_interrupts();
     curTime = *newTime;
     /* Allows interrupts (PRIMASK is cleared) */
     enable_interrupts();
     flexTimer0Init(1875);
     flexTimer0Start();
     return 0;
}

//NOTE: requires DI/EI because 64-bit integer
void date_time_incr(){
    disable_interrupts();
     curTime += 1; //TODO: make unsigned
     enable_interrupts();
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

uint8_t month_to_int(char* month){
  for (int i = 0; i < 12; ++i){
    if (strcmp(allMonths[i].month, month) != 0){
      continue;
    }
    return i;
  }
  return 12;
}

uint16_t date_format_FAT(){
  struct date_time curr_date = get_time();
  uint8_t myMonth = month_to_int(curr_date.month);
  if (myMonth == 12 && MYFAT_DEBUG){
    printf("Month returned invalid date. Please check for errors. \n");
  }
  return ((curr_date.year-1980) << 9) | ((myMonth + 1) << 5) | (curr_date.day << 0);
}

uint16_t time_format_FAT(){
  struct date_time curr_date = get_time();
  return ((curr_date.hour) << 11) | ((curr_date.minute) << 5) | (curr_date.second << 0);
}

void print_time(struct date_time curr_date){
     char output[128] = {'\0'};
     sprintf(output, "%s %u, %u " TIMESTAMP "\r\n", curr_date.month, curr_date.day, curr_date.year, curr_date.hour, curr_date.minute, curr_date.second, curr_date.msec, io_dev);
     SVC_fputs(io_dev, output, strlen(output));
}

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

int count_leap_years(int inputYear){
	int count = 0;
	while (inputYear > 1979){
	    if(inputYear % 400 == 0){
	        count ++;
	    }
	    else if(inputYear % 4 == 0 && inputYear % 100 != 0){
	        count ++;
	    }
	inputYear --;
	}
	return count;
}

/*validates date setting*/
void validate_date(char* string){
	  if (check_digit(string[4]) != TRUE){
		 string[4] = '0';
	  }  
}

unsigned long long timestamp_to_ms(){
  char* comDate = __DATE__;
  char* comTime = __TIME__;
  validate_date(comDate);
  unsigned long long returnTimeInSeconds = 0;
  unsigned int thisYear = ((comDate[7]-'0') * 1000 + (comDate[8]-'0') * 100 + (comDate[9]-'0') * 10 + (comDate[10]-'0'));
  returnTimeInSeconds += ((thisYear-1980)*SECYEAR); //years
  unsigned int leapDiff = count_leap_years(thisYear);
  returnTimeInSeconds = returnTimeInSeconds + (leapDiff*SECDAY);//adjust for leap years
  unsigned int thisMonth = (comDate[0] == 'J') ? ((comDate[1] == 'a') ? 0 : ((comDate[2] == 'n') ? 150 : 180))    // Jan, Jun or Jul
                                : (comDate[0] == 'F') ? 30                                                              // Feb
                                : (comDate[0] == 'M') ? ((comDate[2] == 'r') ? 59 : 119)                                 // Mar or May
                                : (comDate[0] == 'A') ? ((comDate[1] == 'p') ? 90 : 211)                                 // Apr or Aug
                                : (comDate[0] == 'S') ? 242                                                              // Sep
                                : (comDate[0] == 'O') ? 272                                                             // Oct
                                : (comDate[0] == 'N') ? 303                                                             // Nov
                                : (comDate[0] == 'D') ? 333                                                             // Dec
                                : 0;
  returnTimeInSeconds += thisMonth * SECDAY; //months
  returnTimeInSeconds += ((10*(comDate[4]-'0')+(comDate[5]-'0'))-1)*SECDAY; //days, -1 because it starts on the first
  returnTimeInSeconds += (10*(comTime[6]-'0')+(comTime[7]-'0')); //seconds
  returnTimeInSeconds += (10*(comTime[3]-'0')+(comTime[4]-'0'))*60; //minutes
  returnTimeInSeconds += (10*(comTime[0]-'0')+(comTime[1]-'0'))*SECHOUR; //hours
  return returnTimeInSeconds*1000;
}

unsigned long long ymdhm_to_ms(int inputYear, int inputMonth, int inputDay, int inputHour, int inputMinute){
  unsigned long long returnTimeInSeconds = 0;
  returnTimeInSeconds += ((inputYear-1980)*SECYEAR); //years
  returnTimeInSeconds += count_leap_years(inputYear)*SECDAY;//adjust for leap years
  for (int i = 1; i < inputMonth; i ++){
    returnTimeInSeconds += (allMonths[i].offset*SECDAY);
  }
  returnTimeInSeconds += (inputDay-1)*SECDAY; //days, -1 because it starts on the first
  returnTimeInSeconds += inputHour*SECHOUR; //hours
  returnTimeInSeconds += inputMinute*60; //minutes
  return returnTimeInSeconds*1000;
}
