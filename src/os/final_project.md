# Final Project Proposal

For my final project, I plan to focus on getting additional hardware modules working, as well as writing simple test programs to demonstrate the functionality of these devices.

a. Initialize the TWR-LCD-RGB, reserve framebuffer in RAM, add it to the device-independent I/O system, and enable the UART to echo its characters to the TWR-LCD-RGB display.
b. Wire a small speaker to the on-board 12-bit DAC in such a way that it will accept electrical output from the DAC. 
c. Write a basic driver for the 12-bit DAC. Add the 12-bit DAC to the device-independent I/O system.
d. Write a shell command, "cmd_synth", which will play notes through the attached speaker when letter keys 'a' through 'g' are typed at the console. The letters will map to the notes of the scale. In addition to pitch modification, synth will support basic ADSR functionality and multiple waveforms. In order to achieve this, synth will make use of PDB0 in continuous mode to cycle through the hardware buffers of the DAC, which will be pre-initialized to the correct values relative to the reference voltage.