// SPDX-License-Identifier: MIT
// Copyright 2021-2024 RnD Center "ELVEES", JSC

#include <stdarg.h>
#include <stdint.h>

#include <regs.h>
#include <uart.h>

static void _uart_putc(struct uart *uart, char c)
{
	while (!(uart->LSR & 0x20)) {
	}

	uart->THR = c;
}

void uart_putc(struct uart *uart, char c)
{
	_uart_putc(uart, c);
	if (c == '\n')
		_uart_putc(uart, '\r');
}

void uart_putc_raw(struct uart *uart, char c)
{
	_uart_putc(uart, c);
}

void uart_puts(struct uart *uart, char *s)
{
	while (*s) {
		uart_putc(uart, *s);
		s++;
	}
}

uint8_t uart_getchar(struct uart *uart)
{
	while (!(uart->LSR & 0x1)) {
	}

	return uart->RBR;
}

int uart_is_char_ready(struct uart *uart)
{
	return uart->LSR & 0x1;
}

void uart_clear_input_buffer(struct uart *uart)
{
	while (uart->LSR & 0x1)
		(void)uart->RBR;
}

void uart_flush(struct uart *uart)
{
	while (!(uart->LSR & 0x40)) {
	}
}

void uart_write(struct uart *uart, char *ptr, int len)
{
	for (int i = 0; i < len; i++)
		uart_putc(uart, ptr[i]);
}

static void uart_puthex(struct uart *uart, uint32_t hex, uint32_t digits)
{
	static const char tohex[16] = { '0', '1', '2', '3', '4', '5', '6', '7',
					'8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
	int start = 0;

	for (int i = 0; i < 8; i++) {
		if ((hex >> (4 * (7 - i))) & 0xF)
			break;
		start++;
	}
	if (digits < (8 - start))
		digits = 8 - start;

	start = 8 - digits;

	if (start > 7)
		start = 7;

	for (int i = start; i < 8; i++)
		_uart_putc(uart, tohex[(hex >> (4 * (7 - i))) & 0xF]);
}

static void uart_putint(struct uart *uart, uint32_t data, int is_signed)
{
	char buf[20];
	int pos = sizeof(buf);
	int is_neg = is_signed && (((int32_t)data) < 0);

	if (data == 0) {
		buf[--pos] = '0';
	} else if (is_neg)
		data = -((int32_t)data);

	while (data) {
		buf[--pos] = '0' + (data % 10);
		data /= 10;
	}
	if (is_neg) {
		buf[--pos] = '-';
	}
	uart_write(uart, buf + pos, sizeof(buf) - pos);
}

void uart_printf(struct uart *uart, char *s, ...)
{
	uint32_t digits = 0;
	int is_percent_handler = 0;

	va_list args;
	va_start(args, s);
	while (*s) {
		if (*s == '%') {
			digits = 0;
			is_percent_handler = 1;
			s++;
			continue;
		}
		if (is_percent_handler) {
			switch (*s) {
			case '%':
				uart_putc(uart, *s);
				is_percent_handler = 0;
				break;
			case '#':
				if (s[1] == 'x' || s[1] == 'X') {
					s++;
					uart_puts(uart, "0x");
					uart_puthex(uart, va_arg(args, uint32_t), digits);
				} else {
					uart_putc(uart, '%');
					uart_putc(uart, '#');
				}
				is_percent_handler = 0;
				break;
			case 'x':
			case 'X':
				uart_puthex(uart, va_arg(args, uint32_t), digits);
				is_percent_handler = 0;
				break;
			case 'd':
			case 'i':
			case 'u':
				uart_putint(uart, va_arg(args, uint32_t), *s != 'u');
				is_percent_handler = 0;
				break;
			case 's':
				uart_puts(uart, va_arg(args, char *));
				is_percent_handler = 0;
				break;
			default:
				if (*s >= '0' && *s <= '9') {
					digits = digits * 10 + (*s - '0');
				} else {
					uart_putc(uart, '%');
					uart_putc(uart, *s);
					is_percent_handler = 0;
				}
				break;
			}
			s++;
		} else
			uart_putc(uart, *s++);
	}
	va_end(args);
}

uint16_t uart_get_div(struct uart *uart)
{
	uint16_t div;

	uart->LCR = 0x83; // DLAB, 8bit
	div = (uart->DLH >> 8) | uart->DLL;
	uart->LCR = 0x3; // 8bit

	return div;
}

static void _uart_set_div(struct uart *uart, uint16_t div)
{
	uart->LCR = 0x83; // DLAB, 8bit
	uart->DLL = div & 0xff;
	uart->DLH = (div >> 8) & 0xff;
	uart->LCR = 0x3; // 8bit
}

void uart_set_div(struct uart *uart, uint16_t div)
{
	_uart_set_div(uart, div);
}

void uart_init(struct uart *uart, uint32_t in_freq, uint32_t baudrate)
{
	uint32_t div;
	uint32_t counter = 1000;

	div = (in_freq / 16 + (baudrate / 2)) / baudrate;
	_uart_set_div(uart, div);
	while (uart->LCR != 0x3 && counter-- != 0) {
		uart->FCR = 0;
		uart->FCR = 0x1;
		uart->FCR = 0x7;
		(void)uart->THR;
		uart->LCR = 0x3;
	}
	uart->FCR = 1; // enable FIFO
	if ((uart->IIR & 0xc0) != 0xc0)
		uart_puts(uart, "Can not enable UART_FIFO\n");

	uart_clear_input_buffer(uart);
}
