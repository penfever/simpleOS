/**
 * intSerialIO.h
 * Header file for serial input and output interrupt test program
 * 
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 * 
 * Copyright (c) 2021, 2014 James L. Frankel.  All rights reserved.
 * 
 * Last revised 1:58 PM 22-Mar-2021
 */

#ifndef _INT_SERIAL_IO_H
#define _INT_SERIAL_IO_H

/* The size of the buffer used to store characters input from serial port 2 */
#define SERIAL_PORT_2_INPUT_BUFFER_SIZE 128
/* The size of the buffer used to store characters to be output to serial port 2 */
#define SERIAL_PORT_2_OUTPUT_BUFFER_SIZE 128

void interruptSerialPort2(void);
char getcharFromBuffer(void);
void putcharIntoBuffer(char ch);
void putsIntoBuffer(char *s);
void intSerialIOInit(void);
int intSerIoProj();
int main(void);

#endif /* ifndef _INT_SERIAL_IO_H */
