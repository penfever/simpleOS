# Simple OS

[Video Presentation: Simple OS Features and Functionality](https://www.youtube.com/watch?v=RgsQV31lbEA)

The Simple OS is a command-line operating system written in the C Programming Language. It is designed to run on the [NXP Kinetis K70 Tower System](https://www.nxp.com/design/development-boards/tower-development-boards/mcu-and-processor-modules/kinetis-modules/kinetis-k70-120-mhz-tower-system-module:TWR-K70F120M). 


## Key Features

* REPL Shell

* Custom implementations of malloc and free

* Console I/O over UART-RS232

* Simplified, Windows-Compatible FAT32 File System
  
* Supervisor calls and privileged mode

* Hardware drivers and support for 18 different devices, including LEDs, system timers, ADC and DAC
  
* Hardware and software interrupts

* Date and time support
  
* Multiprocessing

* Console display support
  
* Digital synthesizer

## Code Attribution

The code for the operating system is located in the src folder.

Multiple authors contributed to the code in simpleOS. Each source file begins with a header which includes the attribution for that file.

The files I authored include (but are not limited to):

main.c

simpleshell.c (REPL shell)

mymalloc.c (first-fit memory allocation)

SDHC_FAT32_Files.c (FAT32 file system)

univio.c (device-independent I/O)

devices.c (hardware drivers and device support)

dateTime.c (date and time support)

procs.c (multiprocessing support)

## K70 Hardware Setup

The Simple OS is designed to take advantage of many built-in features of the K70 MCU and Tower System. In order to use all of the features of the OS, you will need to have access to a [fully assembled K70 Tower](https://cscie92.dce.harvard.edu/spring2021/Photos/K70%20Right%20Above%20with%20J-Link%20&%20Serial%2020210120_141725.jpg) with the [TWR-SER](https://www.nxp.com/part/TWR-SER#/) card and [TWR-LCD-RGB](https://www.nxp.com/part/TWR-LCD-RGB#/) display installed.

A [Segger J-Link](https://www.segger.com/products/debug-probes/j-link/) is recommended (but not required) for debugging messages.

A MicroSD SDHC card is required in order for the operating system to function normally. This card should be formatted as FAT32 and inserted into the K70 MCU board.

### K70 Connection to PC

In order for the Simple OS to compile and run, the K70 hardware must be properly connected to your computer. Depending on the versions of the hardware you have, this may require up to four USB connections.

Debug messages, if enabled, will be sent over either the Segger J-Link or the OSBDM/OSJTAG direct connection on the MCU board. If you are using the Segger J-Link on Windows, you may need to connect both in order for the output to display. New device drivers from Freescale/P&E
Micro are establishing a connection for UART2 over the usual USB cable rather than over
the DE9 connector on the TWR-SER board.

A [Tripp-Lite Keyspan USB to Serial Adapter](https://www.tripplite.com/sku/USA19HS/), or another similar product, is required to handle the normal I/O functions to the console. Appropriate terminal simulation software will also be required, such as [PuTTY](https://www.putty.org/). The Simple OS UART is configured to run at 115,200 baud, with 8 data bits, 1 stop bit and no parity bit.

The TWR-LCD-RGB and DAC draw more power than the OSBDM/OSJTAG connector provides. Therefore, you will need to attach a separate USB cable to a power source, such as your laptop or (ideally) a separate powered hub on the same electrical circuit, so that they share a common ground.

Please note that at least in some cases, you need to plug the MCU board cable into the computer first, before attaching the power cable.

### Synthesizer

The synthesizer requires additional hardware setup in order to operate. Details of the implementation are provided below.

## Building the Simple OS

NXP supports the CodeWarrior development environment.  Here are [some instructions](https://cscie92.dce.harvard.edu/spring2021/InstallingCodeWarrior.txt) on how to get the environment running on your computer.

In order to build and run the Simple OS, you will need to create a new project and import the source files and headers provided into your Sources folder in CodeWarrior. For the sake of tidiness, you may choose to keep your headers in the Project_Headers folder instead -- however, this is not required.

Please enable C99 in CodeWarrior before attempting to build the shell.

If using Codewarrior, I recommend building in debug mode, as the debugger uses console I/O when it is enabled.

## Enabling Debug Mode

Debug mode is disabled by default. It can be enabled by setting either #MYFAT_DEBUG or #MYFAT_DEBUG_LITE to true in myerror.h. The lite mode outputs fewer messages to the console -- this improves system responsiveness, and is the recommended debug mode in most circumstances.

## Operating the Shell

The simple shell is the user's primary mode of interaction with the Simple OS. The shell is able to run a variety of commands -- type 'help' for a comprehensive list.

If you do not have the TWR-LCD-RGB attached, you will need to be connected over the UART in order to see the output of the shell. In either circumstance, you will need the UART in order to pass input into the shell. 

The shell supports quotation marks and escape characters. Backspace, delete, copy/paste, arrow keys and tab are currently NOT supported. Press enter to complete a command.

## Shell Commands

Each shell command is listed below with a brief description of its behavior, followed by the syntax required to invoke it.

The maximum length of a shell command is 256 characters. The maximum number of allowed arguments is 32.

None of the shell commands require quotation marks in order to function.

**exit** will exit this shell.

exit

**help** displays a list of commands and how they work.

help

**echo** will echo back whatever words follow the command itself.

echo (*arguments to be echoed back*)

**date** will print the current date and time (GMT, MS-DOS EPOCH), which is acquired by the system at compile time. The time will continue updating for as long as the application is running.

date

**date** can also set the current system time by accepting an argument in milliseconds or a simplified ISO-8601 formatted string.

date (*system time in ms*)

date (*017-04-29T14:30*)

**malloc** takes as an argument an amount of memory to be allocated and returns a pointer to the allocated memory region.

malloc (*amount*)

**free** frees memory at a given pointer. 

malloc (*0xFFFFFFFF*)

**memset** sets an allocated memory region to a value. 

**memchk** checks that an allocated memory region is set to a value. 

**memorymap** prints a map of all allocated memory. 

memorymap

**malfree** performs 100 semi-random allocations and frees of memory in order to test memory integrity.

malfree

**fopen** opens a file or device in a particular mode. The default mode is r. For devices with write support, w and a modes may also be available -- however, the implementation is device-specific. If successful, fopen will echo back to the shell a pointer to the device which can then be used to issue commands to the device.

fopen is case insensitive -- you can type either lowercase or uppercase. You can ignore trailing spaces when entering a filename. You can include the period before the file extension or exclude it. Quotation marks should not be included.

fopen accepts a ‘w’ flag for write access, an ‘r’ flag for read-only access (default), and an 'a' flag for append access, which is recognized only by fputc/fputs.

If the file open is successful, fopen will print a file pointer to the uart. type this pointer value into the console (including 0x for hexadecimal values) in order to access the file.

fopen also opens devices, including pushbuttons and LEDs, and returns a file pointer which allows you to pass input to those devices.

fopen (*filename*) (*mode*)

**fclose** closes a file or device.

fclose (*0xFFFFFFFF*)

**fgetc** and **fgets** retrieve characters and strings, respectively, from a file or device and print them to the terminal.

fgetc (*0xFFFFFFFF*)

fgets (*0xFFFFFFFF*) (*amount*)

**fputc** and **fputs** send characters or strings to a file or device. 

**create** and **delete** create and delete files on the inserted MicroSD card. 

create (*filename*)

delete (*0xFFFFFFFF*)

**ls** lists the current directory contents without extended attributes.

ls

ls 1 lists with extended attributes.

ls (*1*)

**seek** sets the file cursor to a particular position in a file. Cursor address is shared between read and write operations on a given file.

seek (*0xFFFFFFFF*) (*position*)

**touch2led** activates the LEDs based on whether you are touching their corresponding touch sensor. Touch all 4 to exit.

touch2led

**pot2ser** continuously outputs the potentiometer value to STDOUT as a value between 0 and 255. The K70's potentiometer knob is located on the hardware itself, and can be adjusted manually.

pot2ser

**therm2ser** continuously outputs the thermistor (system temperature) value to STDOUT as a value between 0 and 255

therm2ser

**pb2LED** toggles LEDs based on inputs from the K70's two onboard pushbuttons.

pb2led

**catfile** prints the contents of a file to STDOUT.

catfile (*filename*)

**cat2file** copies characters from serial input to the specified <file> in the root directory. Please note that the file must already exist on the card in order for this command to function. The command terminates when ctrl+D is entered.

cat2file (*filename*)

**spawn** creates a new background process and returns to the shell prompt. 

The following commands currently support spawn: flashled, touch2led, cat2file, pb2led, 
busywait, uartsendmsg

spawn (*command*)

**kill** kills a target process, with the target indicated by its pid number.

kill (*pid*)

**ps** lists all processes currently running on the OS, including their pids and current uptime

ps

**uartsendmsg** sends a message whenever pushbutton 2 is depressed

uartsendmsg

**busywait** loops forever and does nothing

busywait

**multitask** spawns cat2file, flashled and uartsendmsg. Terminate with ctrl+D.

multitask

**flashled** flashes an LED on and off at an interval determined by the user (1-20 in 50ms intervals)

flashled (*interval*)

**synth** launches a synthesizer application which outputs notes over DAC0.

synth (*note duration from 0 to 127*)

0 means that a note will play until interrupted. The other values represent release envelopes of various durations.

Type letters a through g to play those notes.

Type q to quit.

Type w to toggle between square and sawtooth waveforms.

## SVC_malloc and SVC_free

The Simple OS includes customized routines for memory allocation and deallocation of the 128MB region of DDR memory provided in the K70 development environment.  according to a size parameter passed in by the user, so that programs will have access to the memory resources they require in order to operate.

These implementations use a first-fit algorithm in order to partition the memory space. Although first-fit can lead to memory fragmentation over time, the speed advantages of first-fit made it a natural choice for this basic OS.

Memory ownership and protection are implemented to prevent unauthorized programs from modifying regions of memory which do not belong to them.

## FAT32 and Device-Independent I/O

The Simple OS file system uses FAT32, 8.3 short filenames, and currently only works with one directory, the current working directory.

## STDIN, STDOUT, STDERR Streams

STDIN, STDOUT and STDERR are opened on the UART when the shell launches. A FILE* will print to your UART when the shell launches representing this connection. Although you can use this FILE* in conjunction with the Device-Independent IO commands, that is not necessary in order for shell commands to run.
## Devices

Simple OS is designed around device-independent I/O, which means that devices on the system are accessed and manipulated as though they were files. Here are the 'filenames' of the devices available via fopen from the command line.

dev_sw1 = pushbutton 1

dev_sw2 = pushbutton 2

dev_E1 … dev_E4 = LEDs 1 … 4. 1 is Red, 2 is Blue, 3 is Green, 4 is Yellow.

dev_UART2 = the UART.

dev_pot = the on-board potentiometer.

dev_temp = the on-board temperature sensor.

dev_TSI1 -> dev_TSI4 = the four on-board capacitive touch pads.

dev_DAC0 -> dev_DAC1 = the two on-board 12-bit DACs.

Pushbuttons, LEDs, potentiometer, temperature sensor, DAC and touch pads should be opened with the ‘r’ flag, for read.

EG: fopen PTR_ADDR r

the UART responds to fgetc, fputc, and fputs.

The potentiometer and thermistor respond to fgetc by outputting their settings in hexadecimal format.

The touch pads respond to fgetc by returning whether or not they are currently being touched.

Pushbuttons respond only to fgetc, which prints out whether the pushbutton is pressed or not.

LEDs respond to fgetc with a value (ignored by the shell) which corresponds to whether they are on (TRUE) or off (FALSE). fputc DEV_PTR y will turn an LED on, fputc DEV_PTR n will turn it off.

## Supervisor Calls

Supervisor calls and privileged mode are supported in the Simple OS. Therefore, attempts to access hardware directly may result in a default breakpoint error. Please use shell commands to access system functions.

Supervisor calls available to the systems programmer include the following --

```c
/*This supervisor call is for opening files and devices.*/
int SVC_fopen(file_descriptor* descr, char* filename, char mode);
/*This supervisor call is for closing files and devices.*/
int SVC_fclose(file_descriptor descrf);
/*This supervisor call is for creating files.*/
int SVC_create(char* filename);
/*This supervisor call is for deleting files.*/
int SVC_delete(char* filename);
/*This supervisor call gets character-length input from devices and files.*/
int SVC_fgetc (file_descriptor descrf, char* bufp);
/*This supervisor call gets string-length input from devices and files.*/
int SVC_fgets (file_descriptor descrf, char* bufp, int buflen);
/*This supervisor call sends character-length input to devices and files.*/
int SVC_fputc (file_descriptor descrf, char bufp);
/*This supervisor call sends string-length input to devices and files.*/
int SVC_fputs (file_descriptor descrf, char* bufp, int buflen);
/*This supervisor call allocates memory to a process.*/
void* SVC_malloc(unsigned int size);
/*This supervisor call frees memory previously allocated to a process.*/
int SVC_free(void *ptr);
/*This supervisor call prints the contents of a directory.*/
int SVC_dir_ls(int full);
/*This supervisor call sets the current time.*/
int SVC_settime(unsigned long long *newTime);
/*This supervisor call sets or resets the one-shot PDB timer.*/
int SVC_pdb0oneshottimer(uint16_t* delayCount);
/*This supervisor call spawns a new process.*/
int SVC_spawn(int main(int argc, char *argv[]), int argc, char *argv[], struct spawnData* thisSpawn);
/*This supervisor call allows a process to yield its remaining quantum.*/
void SVC_yield(void);
/*This supervisor call blocks a process.*/
void SVC_block(void);
/*This supervisor call wakes a blocked process.*/
int SVC_wake(pid_t targetPid);
/*This supervisor call instructs a process to destroy itself.*/
int SVC_kill(pid_t targetPid);
/*This supervisor call allows a process to wait until another process completes.*/
void SVC_wait(pid_t targetPid);
```

All supervisor calls may be called as one would a normal C function. All supervisor calls except for SVC_malloc and SVC_free return error codes, and should be checked on return in case of error. SVC_malloc returns null in case of error.

## Hardware and Software Interrupts

Several timers are available in the Simple OS in order to complete certain functions. These timers rely on hardware interrupts in order to perform their tasks. The UART also relies on interrupts in order to cache and display characters the user has typed to the terminal. Finally, supervisor calls rely on software interrupts like those documented above in order to segregate privileged and unprivileged tasks.

The priority of these timers and the interrupt service handlers can be found in kinetis_sysinit.c, a standard file which was modified in order to implement this OS.

## Multiprogramming and Context Switching

Spawn, kill and the other multiprogramming commands incorporated into the OS make use of a round robin scheduler in order to implement context switching between processes. State is preserved in a PCB (process control block). Every process in the simple shell has a PCB. They are stored as a circular linked list and associated with a PID, which determines device and memory ownership.

When the time comes to switch out the old process and switch in the new process, state is restored by directly mapping the correct values into the registers on the ARM processor, using a mix of assembly code and C.

## Synth

The synth command turns your K70 into a digital synthesizer! The synth supports two waveforms and a custom release envelope. See the video link at the top of the readme for an example of how this looks (and sounds) in action.

The synth requires any consumer-grade speaker, a breadboard, a one microfarad capacitor, some copper wire, and a method for connecting the speaker to the breadboard. However, one can also verify the output of the synth by connecting any oscilloscope to the ground and DAC0 contacts on the K70 and launching the synth program.

CIRCUIT DIAGRAM

![Circuit Diagram](../../etc/img/circuit.png)

HARDWARE SETUP WITH BREADBOARD

![Hardware Setup with Breadboard](../../etc/img/dac_synth_bb.jpg)

In order to function, the synth initializes DAC0's 16 hardware buffers to preset voltage values, relative to the reference voltage (3.3v). This represents a (very low-resolution) waveform. Synth then launches the PDB timer in continuous mode in order to repeatedly sweep over the hardware buffers at a frequency corresponding to the correct note of the twelve-tone scale (EG: 440Hz for A4).

## Licensing and Attribution

Special thanks to Prof. James Frankel of the Harvard Extension School, whose source code is used extensively throughout this project. The portions of this project written by Prof. Frankel are subject to his copyright and should not be re-used without his express permission.

The portions of this project written by me are open-source, and are released under the MIT License.
