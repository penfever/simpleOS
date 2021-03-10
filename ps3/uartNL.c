/*
 * uartNL.c
 * UART extensions for line input and to perform string output with \n
 * outputted as \r\n Implementation
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021 James L. Frankel.  All rights reserved.
 */

#include <uartNL.h>

/********************************************************************/
/*
 * Input a line using uartGetchar
 *
 * The line is terminated by a UARTNL_END_OF_INPUT_LINE_CHAR character
 * entered by the user.  The size parameter is the total size of the
 * string pointed to by "p" which must include space for a terminating
 * NUL character.
 *
 * All input characters are echoed back on the same UART channel.  An
 * input new-line is echoed as carriage-return then new-line.
 *
 * If the space allocated for the inputted string is exhausted, then the
 * string is correctly NUL terminated and no more characters are input.
 *
 * Parameters:
 *  uartChannel  UART channel on which to output a string
 *  p            pointer to string to be filled in with inputted chars
 *  size         the total number of characters allocated for the string
 */ 
void uartGetline(UART_MemMapPtr uartChannel, char *p, int size) {
  int i;
  char c;
  
  for(i = 0; i < size; i++) {
    c = uartGetchar(uartChannel);
    uartPutchar(uartChannel, c);
    if(c == UARTNL_END_OF_INPUT_LINE_CHAR) {
      if(c == '\r')
	uartPutchar(uartChannel, '\n');
      *p = '\0';
      return;
    }
    *p++ = c;
  }
  *--p = '\0';
}

/********************************************************************/
/*
 * Output a string using uartPutchar, but with \n outputted as \r\n
 *
 * Parameters:
 *  uartChannel  UART channel on which to output a string
 *  p            pointer to string to be output
 */ 
void uartPutsNL(UART_MemMapPtr uartChannel, char *p) {
  while(*p) {
    if(*p == '\n')
      uartPutchar(uartChannel, '\r');
    uartPutchar(uartChannel, *p++);
  }
}
