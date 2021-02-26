# simple shell

## Purpose

Simple shell is the shell for an operating system I am developing as part of the Principles of Operating Systems course. 

## Use

Simple shell is currently able to run a variety of commands -- type 'help' for more information on the commands which are available.

# myMalloc and myFree

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


