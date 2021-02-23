#ifndef _MYERROR_H
#define _MYERROR_H
#define NUMCODES (int)(sizeof(errordesc)/sizeof(errordesc[0]))

typedef enum _os_error error_t;

enum _os_error
{
    E_SUCCESS = 0,
    E_CONSTRUCTION = -1,
    E_CMD_NOT_FOUND = -2,
    E_SYSCALL = -3,
    E_NUMARGS = -4,
    E_NOINPUT = -5,
    E_TOO_LONG = -6,
    E_MALLOC = -7,
    E_INF = -8,
    E_FREE = -9,
    E_EMPTYMEM = -10,
    E_FREE_PERM = -11,
    E_MEMCHK = -12
};

struct _errordesc {
    int  code;
    char *message;
} errordesc[] = {
    { E_SUCCESS, "No error \n" },
    { E_CONSTRUCTION, "Under construction \n" },
    { E_CMD_NOT_FOUND, "Invalid command -- type help for a list \n" },
    { E_SYSCALL, "System call failed \n" },
    { E_NUMARGS, "Wrong number of arguments \n" },
    { E_NOINPUT, "No input or invalid input -- type help for more \n"},
    { E_TOO_LONG, "Input longer than the maximum allowable characters (256) \n" },
    { E_MALLOC, "Fatal: failed to allocate memory \n" },
    { E_INF, "Fatal: end of infinite loop reached \n" },
    { E_FREE, "Free: Invalid pointer \n" },
    { E_EMPTYMEM, "There is no memory allocated. Freeing block. \n"},
    { E_FREE_PERM, "Your PID does not have permission to free this block \n"},
    { E_MEMCHK, "memchk failed \n"}
};

int error_checker(int return_value);

#endif