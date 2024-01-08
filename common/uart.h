// SPDX-License-Identifier: MIT
// Copyright 2020-2024 RnD Center "ELVEES", JSC

#ifndef UART_H_
#define UART_H_

#include <stdint.h>

#define UART0 ((struct uart *)(TO_VIRT(UART0_BASE)))
#define UART1 ((struct uart *)(TO_VIRT(UART1_BASE)))
#define UART2 ((struct uart *)(TO_VIRT(UART2_BASE)))
#define UART3 ((struct uart *)(TO_VIRT(UART3_BASE)))

struct uart {
	union {
		volatile uint32_t RBR;
		volatile uint32_t DLL;
		volatile uint32_t THR;
	};
	union {
		volatile uint32_t DLH;
		volatile uint32_t IER;
	};
	union {
		volatile uint32_t FCR;
		volatile uint32_t IIR;
	};
	volatile uint32_t LCR;
	volatile uint32_t MCR;
	volatile uint32_t LSR;
	volatile uint32_t MSR;
	volatile uint32_t SCR;
	volatile uint32_t SRBR[16];
	volatile uint32_t FAR;
	volatile uint32_t TFR;
	volatile uint32_t RFW;
	volatile uint32_t USR;
	volatile uint32_t TFL;
	volatile uint32_t RFL;
	volatile uint32_t SRR;
	volatile uint32_t SRTS;
	volatile uint32_t SBCR;
	volatile uint32_t SDMAM;
	volatile uint32_t SFE;
	volatile uint32_t SRT;
	volatile uint32_t STET;
	volatile uint32_t HTX;
	volatile uint32_t DMASA;
	volatile uint32_t TCR;
	volatile uint32_t DE_EN;
	volatile uint32_t RE_EN;
	volatile uint32_t DET;
	volatile uint32_t TAT;
	volatile uint32_t DLF;
	volatile uint32_t CPR;
	volatile uint32_t UCV;
	volatile uint32_t CTR;
};

void uart_putc(struct uart *uart, char c);
void uart_putc_raw(struct uart *uart, char c);
void uart_puts(struct uart *uart, char *s);
uint8_t uart_getchar(struct uart *uart);
int uart_is_char_ready(struct uart *uart);
void uart_clear_input_buffer(struct uart *uart);
void uart_flush(struct uart *uart);
void uart_write(struct uart *uart, char *ptr, int len);
void uart_printf(struct uart *uart, char *s, ...);
uint16_t uart_get_div(struct uart *uart);
void uart_set_div(struct uart *uart, uint16_t div);
void uart_init(struct uart *uart, uint32_t in_freq, uint32_t baudrate);

#endif
