# Final Project Proposal

For my final project, I plan to focus on getting additional hardware modules working, as well as writing simple test programs to demonstrate the functionality of these devices.

a. Initialize the TWR-LCD-RGB, reserve framebuffer in RAM, add it to the device-independent I/O system, and enable the UART to echo its characters to the TWR-LCD-RGB display.
b. Write a shell command, "image", which allows the user to display un-encoded image files from the SD card on the TWR-LCD-RGB screen.
c. Wire a small speaker to the on-board 12-bit DAC in such a way that it will accept electrical output from the DAC. 
d. Write a basic driver for the 12-bit DAC. Add the 12-bit DAC to the device-independent I/O system.
e. Write a shell command, "synth", which will play notes through the attached speaker when letter keys 'a' through 'g' are typed at the console. The letters will map to the notes of the scale.

a preliminary
break-down of the project into smaller tasks including the
functions/components/modules to be written