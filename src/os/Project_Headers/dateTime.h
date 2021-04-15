/*Holds current system time in ms since the MS-DOS epoch, 00:00, Jan 1 1980, as given by user*/
extern long long curTime;

/*System routine sets current time to time requested by the user, enables flexTimer0 at 1ms intervals
to allow the system to keep track of time as it changes*/
int set_time(long long newTime);

/*Subroutine called by flexTimer 0 ISR. increments current time by 1ms*/
void date_time_incr();
