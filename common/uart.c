/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#include <stdarg.h>
#include <stdint.h>

#include <regs.h>
#include <uart.h>

uart_regs_t *const uart0 = (uart_regs_t *)(TO_VIRT(UART0_BASE));


static void _uart_putc(char c) {
	while (!(uart0->LSR & 0x20)) {}
	uart0->THR = c;
}

void uart_putc(char c) {
	_uart_putc(c);
	if (c == '\n')
		_uart_putc('\r');
}

void uart_putc_raw(char c) {
	_uart_putc(c);
}

void uart_puts(char *s) {
	while (*s) {
		uart_putc(*s);
		s++;
	}
}

uint8_t uart_getchar(void) {
	while (!(uart0->LSR & 0x1)) {}

	return uart0->RBR;
}

int uart_is_char_ready(void) {
	return uart0->LSR & 0x1;
}

void uart_flush(void) {
	while (!(uart0->LSR & 0x40)) {}
}

void uart_write(char *ptr, int len) {
	for (int i = 0; i < len; i++)
		uart_putc(ptr[i]);
}

// static void uart_puthex(uint32_t hex) {
// 	static const char tohex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
// 				       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
//
// 	for (int i = 0; i < 8; i++)
// 		_uart_putc(tohex[(hex >> (4 * (7 - i))) & 0xF]);
// }

static void uart_puthex(uint32_t hex, uint32_t digits) {
	static const char tohex[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
				       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
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
		_uart_putc(tohex[(hex >> (4 * (7 - i))) & 0xF]);
}

static void uart_putint(uint32_t data, int is_signed) {
	char buf[20];
	int pos = sizeof(buf);
	int is_neg = is_signed && (((int32_t)data) < 0);

	if (data == 0) {
		buf[--pos] = '0';
	} else if (is_neg)
		data = (uint32_t)-((int32_t)data);

	while (data) {
		buf[--pos] = '0' + (data % 10);
		data /= 10;
	}
	if (is_neg) {
		buf[--pos] = '-';
	}
	uart_write(buf + pos, sizeof(buf) - pos);
}

void uart_printf(char *s, ...)
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
					uart_putc(*s);
					is_percent_handler = 0;
					break;
				case '#':
					if (s[1] == 'x' || s[1] == 'X') {
						s++;
						uart_puts("0x");
						uart_puthex(va_arg(args, uint32_t), digits);
					} else {
						uart_putc('%');
						uart_putc('#');
					}
					is_percent_handler = 0;
					break;
				case 'x':
				case 'X':
					uart_puthex(va_arg(args, uint32_t), digits);
					is_percent_handler = 0;
					break;
				case 'd':
				case 'i':
				case 'u':
					uart_putint(va_arg(args, uint32_t), *s != 'u');
					is_percent_handler = 0;
					break;
				case 's':
					uart_puts(va_arg(args, char*));
					is_percent_handler = 0;
					break;
				default:
					if (*s >= '0' && *s <= '9') {
						digits = digits * 10 + (*s - '0');
					} else {
						uart_putc('%');
						uart_putc(*s);
						is_percent_handler = 0;
					}
					break;
			}
			s++;
		} else
			uart_putc(*s++);
	}
	va_end(args);
}

void uart_init(uint32_t in_freq, uint32_t baudrate) {
	uint32_t div;
	uint32_t counter = 1000;

	div = (in_freq / 16 + (baudrate / 2)) / baudrate;
	uart0->LCR = 0x83;  // DLAB, 8bit
	uart0->DLL = div & 0xff;
	uart0->DLH = (div >> 8) & 0xff;
	uart0->LCR = 0x3;  // 8bit
	while (uart0->LCR != 0x3 && counter-- != 0) {
		uart0->FCR = 0;
		uart0->FCR = 0x1;
		uart0->FCR = 0x7;
		(void)uart0->THR;
		uart0->LCR = 0x3;
	}
	uart0->FCR = 1;  // enable FIFO
	if ((uart0->IIR & 0xc0) != 0xc0)
		uart_puts("Can not enable UART_FIFO\n");
}
