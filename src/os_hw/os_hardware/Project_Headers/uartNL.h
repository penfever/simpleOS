/*
 * uartNL.h
 * UART extensions for line input and to perform string output with \n
 * outputted as \r\n Header File
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2021, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright (c) 2021 James L. Frankel.  All rights reserved.
 *
 * Last updated: 3:22 PM 18-Mar-2021
 */

#ifndef _UARTNL_H
#define _UARTNL_H

#include <uart.h>

#define UARTNL_BACKSPACE_CHAR '\b'
#define UARTNL_END_OF_INPUT_LINE_CHAR '\r'
#define UARTNL_DELETE_CHAR 0x7f

void uartGetline(UART_MemMapPtr uartChannel, char *p, int size);
void uartPutsNL(UART_MemMapPtr uartChannel, char *p);

#endif /* ifndef _UARTNL_H */
