# Final Project Proposal

For my final project, I plan to focus on getting additional hardware modules working, as well as writing simple test programs to demonstrate the functionality of these devices.

1. Initialize the TWR-LCD, reserve framebuffer in RAM, add it to the device-independent I/O system, and enable the UART to echo its characters to the TWR-LCD display.
2. Create an image viewer and/or video player which reads un-encoded image/video files from the SD card and plays them (without sound). After 'fopen'-ing a file, the user will then be able to execute the 'play' command, along with the pointer to the file to be played/displayed. If the file is not playable, an error will be printed to the console. Otherwise, the TWR-LCD will display the image/video. Videos will start at timecode 0, and will start paused. 

*Note: I will only attempt to add video functionality if all the other items on this list are working and I have more time before the project is due. Most likely I will simply create a basic image viewer.*

3. Wire a small speaker to the on-board 12-bit DAC in such a way that it will accept electrical output from the DAC. 
4. Write a basic driver to enable output to the DAC. Add the 12-bit DAC to the device-independent I/O system.
5. Write a shell program to play the 12-tone scale when letter keys 'a' through 'l' are typed at the console. The letters will map to the notes on the chromatic scale. 