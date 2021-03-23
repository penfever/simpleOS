# Simple Shell

## Purpose

Simple shell is the shell for an operating system I am developing as part of the Principles of Operating Systems course. 

## Use

Simple shell is currently able to run a variety of commands -- type 'help' for more information on the commands which are available.

The shell supports most normal IO operations over the UART and/or CONSOLE IO.
Certain messages may be enabled for UART only or for CONSOLE only.
Backspace, delete, copy/paste, arrow keys and tab are currently NOT supported on the shell.

## Building the Shell
In order to build the shell and test the commands, create a new Codewarrior project, adding all items in src/os/Project_Headers to your headers folder, and all items in src/os/Sources to your sources folder. Both UART and Console IO are currently enabled (Console IO is outputting debug messages). They can be disabled by setting #MYFAT_DEBUG and #MYFAT_DEBUG_LITE to false. 
#CONSOLEIO and #UARTIO are defined in devices.h
#MYFAT_DEBUG and #MYFAT_DEBUG_LITE are defined in myerror.h
I recommend building in Codewarrior’s debug mode, as I am using formatted I/O to string functions.

src/os_hw is included in the repo for the purposes of showing my commit history. It is not required for the build.


## Codewarrior Notes

I used C99 conventions, so please enable C99 in the Codewarrior Project when testing.

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
ls expects one argument, either 0 or an 1. If 0, it will print the directory contents without additional attributes. If 1, it will print the contents of the directory with additional attributes.

## FOPEN
fopen is case insensitive -- you can type either lowercase or uppercase. You can ignore trailing spaces when entering a filename. You can include the period before the file extension or exclude it.
fopen accepts a ‘w’ flag for write access, and an ‘r’ flag for read-only access. 
If the file open is successful, fopen will print a file pointer to the uart. type this pointer value into the console (including 0x for hexadecimal values) in order to access the file.
fopen also opens devices, including pushbuttons and LEDs, and returns a file pointer which allows you to pass input to those devices.

## DEVICE SUPPORT

The device table for fopen is as follows

Device names are case sensitive.

dev_sw1 = pushbutton 1

dev_sw2 = pushbutton 2

dev_E1 … dev_E4 = LED 1 … 4

Both pushbuttons and LEDs should be opened with the ‘r’ flag, for read.

EG: fopen PTR_ADDR r

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

## FGETS AND FPUTS
These commands are enabled in the shell, but are NOT tested. I do not recommend running them. 😊

## SEEK
This command allows you to adjust the cursor position of an open file.
Cursor address is shared between read and write operations on a given file.


