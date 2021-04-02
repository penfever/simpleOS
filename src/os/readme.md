# Simple Shell

## Purpose

Simple shell is the shell for an operating system I am developing as part of the Principles of Operating Systems course. 

## Use

Simple shell is currently able to run a variety of commands -- type 'help' for a comprehensive list of the commands which are available.

The shell supports most normal IO operations over the UART and/or CONSOLE IO.

Certain messages may be enabled for UART only or for CONSOLE only.
Backspace, delete, copy/paste, arrow keys and tab are currently NOT supported on the shell.

All shell commands are named according to the standards laid out in the problem sets for this course, EXCEPT for the following:

cat -> catfile

## Building the Shell
In order to build the shell and test the commands, create a new Codewarrior project, adding all items in src/os/Project_Headers to your headers folder, and all items in src/os/Sources to your sources folder. 

Note that some of the files required for the build are not included in this repo. These files are available at [this link](http://cscie92.dce.harvard.edu/spring2021/index.html).

Debug is disabled. It can be enabled by setting #MYFAT_DEBUG and #MYFAT_DEBUG_LITE to true.  defined in myerror.h.

UART2 support is implemented. #UARTIO, which calls certain printing functions based on whether the UART is used, is defined in devices.h. This is enabled by default, and can safely be ignored.

src/os_hw is included in the repo for the purposes of showing my commit history. It is not required for the build.

The modified kinetis_sysinit.c included in the repo must be used in order for the shell to run.

## IDE Notes

I used C99 conventions, so please enable C99 in your IDE when building.

If using Codewarrior, I recommend building in debug mode, as I am using formatted I/O to string functions.

# Supervisor Call Implementation

Supervisor calls and privileged mode have been enabled in this version of the shell. Therefore, attempts to access hardware directly may result in a default breakpoint error. Please use shell commands to access system functions.

Supervisor calls available to the systems programmer include the following --

int SVC_fopen(file_descriptor* descr, char* filename, char mode);

int SVC_fclose(file_descriptor descrf);

int SVC_create(char* filename);

int SVC_delete(char* filename);

int SVC_fgetc (file_descriptor descrf, char* bufp);

int SVC_fputc (file_descriptor descrf, char bufp);

int SVC_fputs (file_descriptor descrf, char* bufp, int buflen);

void* SVC_malloc(unsigned int size);

int SVC_free(void *ptr);

int SVC_dir_ls(int full);

The functions listed above described in detail in the next sections of this document.

int SVC_ischar(file_descriptor descrf);

This supervisor call is used to check whether a character is avaible on the UART -- if a file is not available, it implements a busy wait until one becomes available.

All supervisor calls may be called as one would a normal C function. All supervisor calls except for SVC_malloc and SVC_free return error codes, and should be checked on return in case of error. SVC_malloc returns null in case of error.

# Simple Shell myMalloc and myFree

## Purpose

myMalloc and myFree are implementations of memory allocation and deallocation for the operating system I am developing as part of the Principles of Operating Systems course. They allocate and free, respectively, portions of a 128MB region of memory according to a size parameter passed in by the user, so that programs will have access to the memory resources they require in order to operate.

## Solution

Because the operating system I am writing is likely to be used mostly for small programs and will not be running
    any massively parallel or RAM-intensive applications for now, and because the NXP/Freescale Hardware is quite
    limited in its capabilities, I decided the speed of first-fit, combined with merging adjacent regions when free is called,
    was the optimal choice. Although first-fit can lead to memory fragmentation over time, my operating
    system is not capable of running enough concurrent programs for this to be as much of an issue as speed at the moment.

## Use

myMalloc and myFree can be called from the simple shell using the commands malloc and free, respectively. Memory can be set to a particular value with the memset command. Memchk checks that the value was properly assigned.

# Simple Shell File System and Device-Independent I/O

My file system uses FAT32, 8.3 short filenames, and currently only works with one directory, the current working directory.

## LS
ls will list the contents of the current working directory on an inserted FAT32-formatted SDHC card.

ls expects one argument, either 0 or an 1. If 0, it will print the directory contents without additional attributes. If 1, it will print the contents of the directory with additional, or FULL, attributes.

## STDIN, STDOUT, STDERR

STDIN, STDOUT and STDERR are opened on the UART when the shell launches. A FILE* will print to your UART when the shell launches representing this connection. Although you can use this FILE* in conjunction with the Device-Independent IO commands, that is not necessary in order for shell commands to run.

## FOPEN
fopen is case insensitive -- you can type either lowercase or uppercase. You can ignore trailing spaces when entering a filename. You can include the period before the file extension or exclude it.

fopen accepts a ‘w’ flag for write access, an ‘r’ flag for read-only access, and an 'a' flag for append access, which is recognized only by fputc/fputs.

If the file open is successful, fopen will print a file pointer to the uart. type this pointer value into the console (including 0x for hexadecimal values) in order to access the file.
fopen also opens devices, including pushbuttons and LEDs, and returns a file pointer which allows you to pass input to those devices.

## DEVICE SUPPORT

The device table for fopen is as follows

Device names are case sensitive.

dev_sw1 = pushbutton 1

dev_sw2 = pushbutton 2

dev_E1 … dev_E4 = LEDs 1 … 4. 1 is Red, 2 is Blue, 3 is Green, 4 is Yellow.

dev_UART2 = the UART.

dev_pot = the on-board potentiometer.

dev_temp = the on-board temperature sensor.

dev_TSI1 -> dev_TSI4 = the four on-board capacitive touch pads.

Pushbuttons, LEDs, potentiometer, temperature sensor and touch pads should be opened with the ‘r’ flag, for read.

EG: fopen PTR_ADDR r

the UART responds to fgetc, fputc, and fputs.

The potentiometer and thermistor respond to fgetc by outputting their settings in hexadecimal format.

The touch pads respond to fgetc by returning whether or not they are currently being touched.

Pushbuttons respond only to fgetc, which prints out whether the pushbutton is pressed or not.

LEDs respond to fgetc and fputc. fgetc turns an LED on, fputc (with any character) turns it off.

EG: fputc PTR_ADDR c

## FCLOSE
fclose expects a file pointer in hexadecimal format. It outputs a success message on a successful close, and an error message in case of failure.

## CREATE AND DELETE
Create and delete are case insensitive -- you can type either lowercase or uppercase. You can ignore trailing spaces when entering a filename. You can include the period before the file extension or exclude it. They each take one argument -- a filename.

## FGETC AND FPUTC
fgetc takes one argument – a pointer to a file descriptor. If the device is enabled for fgetc, it will then perform the appropriate action for that device.

fputc takes two arguments – a pointer to a file descriptor, and a character to be ‘put’. If the device is enabled for fgetc, it will then perform the appropriate action for that device.

## FPUTS
fputs takes three arguments - a pointer to a file descriptor, a char* buffer to be written, and a length of string. fputs is enabled for STDIN, STDOUT, and file I/O.

## SEEK
This command allows you to adjust the cursor position of an open file.
Cursor address is shared between read and write operations on a given file.


