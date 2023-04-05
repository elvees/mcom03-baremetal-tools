// SPDX-License-Identifier: MIT
// Copyright 2020 RnD Center "ELVEES", JSC

#ifndef UART_H_
#define UART_H_

#include <stdint.h>

void uart_putc(char c);
void uart_putc_raw(char c);
void uart_puts(char *s);
uint8_t uart_getchar(void);
int uart_is_char_ready(void);
void uart_flush(void);
void uart_write(char *ptr, int len);
void uart_printf(char *s, ...);
void uart_init(uint32_t in_freq, uint32_t baudrate);

#endif
