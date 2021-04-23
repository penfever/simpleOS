/**
 * util.c
 * routines to perform various utility functions
 * 
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021, 2017, 2015, 2014, 2012 James L. Frankel.  All rights reserved.
 */

#include "util.h"

/* Routine to convert an unsigned char value into its corresponding three
   digit ASCII value.  The returned three ASCII chars are placed in the first
   three characters pointed to by "string." */
void char2ascii(unsigned char ch, char *string) {
  string[0] = (char) (ch/100 + '0');
  ch -= ch/100 * 100;
  string[1] = (char) (ch/10 + '0');
  ch -= ch/10 * 10;
  string[2] = (char) (ch + '0');
}

/* Routine to convert a nibble into its hexadecimal ASCII character */
char nibble2hex(unsigned char nibble) {
  if(nibble <= 9) {
    return (char) (nibble + '0');
  } else if(nibble <= 15) {
    return (char) (nibble - 10 + 'A');
  } else {
    return '?';
  }
}

/* Routine to convert an unsigned short int value into its corresponding four
   character hexadecimal value.  The returned four hexadecimal chars are
   placed in the first four characters pointed to by "string." */
void shortInt2hex(unsigned short int i, char *string) {
  string[0] = nibble2hex((unsigned char) (i/0x1000));
  i -= i/0x1000 * 0x1000;
  string[1] = nibble2hex((unsigned char) (i/0x100));
  i -= i/0x100 * 0x100;
  string[2] = nibble2hex((unsigned char) (i/0x10));
  i -= i/0x10 * 0x10;
  string[3] = nibble2hex((unsigned char) i);
}

/* Routine to convert an unsigned int value into its corresponding eight
   character hexadecimal value.  The returned eight hexadecimal chars are
   placed in the first eight characters pointed to by "string." */
void longInt2hex(unsigned long int i, char *string) {
  string[0] = nibble2hex((unsigned char) (i/0x10000000));
  i -= i/0x10000000 * 0x10000000;
  string[1] = nibble2hex((unsigned char) (i/0x1000000));
  i -= i/0x1000000 * 0x1000000;
  string[2] = nibble2hex((unsigned char) (i/0x100000));
  i -= i/0x100000 * 0x100000;
  string[3] = nibble2hex((unsigned char) (i/0x10000));
  i -= i/0x10000 * 0x10000;
  string[4] = nibble2hex((unsigned char) (i/0x1000));
  i -= i/0x1000 * 0x1000;
  string[5] = nibble2hex((unsigned char) (i/0x100));
  i -= i/0x100 * 0x100;
  string[6] = nibble2hex((unsigned char) (i/0x10));
  i -= i/0x10 * 0x10;
  string[7] = nibble2hex((unsigned char) i);
}

/*mytoupper converts a character to uppercase, if it is a letter between a and z.*/
char mytoupper(char c){
	if ((c >= 'a') && (c <= 'z')){
		c = c - 32;
	}
	return c;
}
