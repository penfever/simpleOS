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
 * Additional code by Benjamin Feuer
 *
 * Copyright (c) 2021, 2017, 2015, 2014, 2012 James L. Frankel.  All rights reserved.
 */
#include "util.h"
#include "simpleshell.h"
#include "myerror.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

char mytoupper(char c){
	if ((c >= 'a') && (c <= 'z')){
		c = c - 32;
	}
	return c;
}

int check_digit (char c) {
    if ((c >= '0') && (c <= '9')) return 1;
    return 0;
}

int check_hex (char c) {
    if ((c >= '0') && (c <= '9')) return 1;
    if ((c >= 'a') && (c <= 'f')) return 1;
    if ((c >= 'A') && (c <= 'F')) return 1;
    return 0;
}

void check_overflow(unsigned long my_num){
  if (my_num == 0){
    error_checker(E_NOINPUT);
  }
  return;
}

int check_digit_all(char* str){
	int len = strlen(str);
	if (len == 0){
		return TRUE;
	}
  for (int i = 0; i < len; i++){
    if (!check_digit(str[i])){
      return FALSE;
    }
  }
  return TRUE;
}

int check_hex_all(char* str){
  for (int i = 0; i < strlen(str); i++){
    if (!check_hex(str[i])){
      return FALSE;
    }
  }
  return TRUE;
}

size_t hex_dec_oct(char* str){
  char* p_str = str + 2;
  int check_str;
  check_str = check_digit_all(str);
  if (!check_digit(str[0])){
    return 0;
  }
  if (str[0] == '0'){
    if (str[1] == 'x' || str[1] == 'X'){
      if (check_hex_all(p_str) == FALSE){
        return 0;
      }
      return strtoul(str, NULL, 16); //return hex
    }
    if (check_str == FALSE){
      return 0;
    }
    return strtoul(str, NULL, 8); //return octal
}
  if (check_str == FALSE){
    return 0;
  }
  return strtoul(str, NULL, 10); //return decimal
}

long long hex_dec_oct_ll(char* str){
  char* p_str = str + 2;
  int check_str;
  check_str = check_digit_all(str);
  if (!check_digit(str[0])){
    return 0;
  }
  if (str[0] == '0'){
    if (str[1] == 'x' || str[1] == 'X'){
      if (check_hex_all(p_str) == FALSE){
        return 0;
      }
      return strtoll(str, NULL, 16); //return hex
    }
    if (check_str == FALSE){
      return 0;
    }
    return strtoll(str, NULL, 8); //return octal
}
  if (check_str == FALSE){
    return 0;
  }
  return strtoll(str, NULL, 10); //return decimal
}
