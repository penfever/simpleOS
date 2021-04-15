#define SECYEAR 31536000
#define SECDAY 86400
#define SECHOUR 3600
#define SECMIN 60
#define TIMESTAMP "%02d:%02d:%02d.%06.ld"

/*Holds current system time in ms since the MS-DOS epoch, 00:00, Jan 1 1980, as given by user*/
struct date_time {
  char* month;
  uint32_t day;
  uint32_t year;
  uint32_t hour;
  uint32_t minute;
  uint32_t second;
  uint32_t msec;
  char* clock;
};

struct months {
  char *month;
  int order;
  int offset;
};

extern struct months allMonths[];

struct date_time get_time();

extern long long curTime;

/*System routine sets current time to time requested by the user, enables flexTimer0 at 1ms intervals
to allow the system to keep track of time as it changes*/
int set_time(long long newTime);

/*Subroutine called by flexTimer 0 ISR. increments current time by 1ms*/
void date_time_incr();

int isleapyear(int inyear);

/*formats and prints a date and time from get_time
  (4) "date" will output to stdout the current date and time in the format "January
   23, 2014 15:57:07.123456".  "date" will call the POSIX system call "gettimeofday" to determine the time and date.  "gettimeofday"
   returns the number of seconds and microseconds since midnight (zero
   hours) on January 1, 1980 -- this time is referred to as the MSDOS Epoch. */
void print_time(struct date_time curr_date);
