/**
 * util.h
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
#include <stdio.h>
#include <stdlib.h>

#ifndef _UTIL_H
#define _UTIL_H

/* Function prototypes */

/* Routine to convert an unsigned char value into its corresponding three
   digit ASCII value.  The returned three ASCII chars are placed in the first
   three characters pointed to by "string." */
void char2ascii(unsigned char ch, char *string);

/* Routine to convert a nibble into its hexadecimal ASCII character */
char nibble2hex(unsigned char nibble);

/* Routine to convert an unsigned short int value into its corresponding four
   character hexadecimal value.  The returned four hexadecimal chars are
   placed in the first four characters pointed to by "string." */
void shortInt2hex(unsigned short int i, char *string);

/* Routine to convert an unsigned int value into its corresponding eight
   character hexadecimal value.  The returned eight hexadecimal chars are
   placed in the first eight characters pointed to by "string." */
void longInt2hex(unsigned long int i, char *string);

/*Routine converts a character to uppercase, if it is a letter between a and z.*/
char mytoupper(char c);

/*Routine checks whether a character is a digit. Returns FALSE if false, TRUE if true.*/
int check_digit(char c);

/*Routine checks whether a character is a hexadecimal character. Returns FALSE if false, TRUE if true.*/
int check_hex (char c);

/*Helper function accepts a string and returns true if every character in the string is a digit.*/
int check_digit_all(char* str);

/*Helper function accepts a string and returns true if every character in the string is a hex digit.*/
int check_hex_all(char* str);

/*Routine checks strtoul output for integer overflow error and calls error_checker to output any errors encountered to the terminal.*/
void check_overflow(unsigned long my_num);

/*Routine parses a user string str and returns it in hex, octal or decimal form, if
it is an unsigned int or long. If it is not an integer or some other error has occurred, returns 0.*/
size_t hex_dec_oct(char* str);

/*Routine parses a user string str and returns it in hex, octal or decimal form, if
it is an long long integer. If it is not an integer or some other error has occurred, returns 0.*/
long long hex_dec_oct_ll(char* str);

#endif /* ifndef _UTIL_H */
