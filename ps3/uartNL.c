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
 *
 * Last updated: 3:28 PM 18-Mar-2021
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
 * If the input entered includes any backspace (BS, Control-H) or
 * delete (DEL) characters, they will be used to delete previous characters
 * already typed, if any exist.
 *
 * Parameters:
 *  uartChannel  UART channel on which to output a string
 *  p            pointer to string to be filled in with inputted chars
 *  size         the total number of characters allocated for the string
 */ 
void uartGetline(UART_MemMapPtr uartChannel, char *p, int size) {
  char *initialStringp;
  int i;
  char c;

  initialStringp = p;
  for(i = 0; i < size; i++) {
    c = uartGetchar(uartChannel);
    if((c == UARTNL_BACKSPACE_CHAR) | (c == UARTNL_DELETE_CHAR)) {
      /* don't echo the backspace or delete until we determine if it
	 is the first character on the line */
      if(p > initialStringp) {
	/* if there is at least one character in the string (i.e., at
	   least one character was already typed and stored in the
	   string), then output backspace, space, backspace to erase
	   the previous character and move back that character's
	   position on the line.  then, make the pointer point back to
	   the previous character in the string */
	uartPutchar(uartChannel, UARTNL_BACKSPACE_CHAR);
	uartPutchar(uartChannel, ' ');
	uartPutchar(uartChannel, UARTNL_BACKSPACE_CHAR);
	p--;
      }
    } else if(c == UARTNL_END_OF_INPUT_LINE_CHAR) {
      /* echo the character */
      uartPutchar(uartChannel, c);
      if(c == '\r')
	uartPutchar(uartChannel, '\n');
      *p = '\0';
      return;
    } else {
      /* echo the character */
      uartPutchar(uartChannel, c);
      *p++ = c;
    }
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
