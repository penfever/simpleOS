#ifndef MYERROR_H
#define MYERROR_H
#define NUMCODES (int)(sizeof(errordesc)/sizeof(errordesc[0]))

typedef enum _os_error error_t;

extern enum _os_error
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
};

extern struct _errordesc errordesc[];

int error_checker(int return_value);

#endif