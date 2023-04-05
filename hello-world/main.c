// SPDX-License-Identifier: MIT
// Copyright 2021 RnD Center "ELVEES", JSC

#include <stdint.h>

#include <regs.h>
#include <uart.h>

int main(void)
{
	uint32_t gpio_state = 1;

	REG(LSPERIPH1_SUBS_PPOLICY) = PP_ON;  // Enable LSPERIPH1
	REG(TOP_CLKGATE) |= BIT(6);  // Enable clock to LSPERIPH1
	while ((REG(LSPERIPH1_SUBS_PSTATUS) & 0x1f) != PP_ON) {
	}

	REG(LSP1_URB_PLL) = 0;  // Turn PLL to bypass mode
	REG(LSP1_UCG_CTRL4) = 0x2;  // GPIO_DBCLK CLK_EN
	REG(LSP1_UCG_CTRL6) = 0x2;  // UART_CLK CLK_EN

	REG(GPIO1_SWPORTB_CTL) |= 0xc0;  // UART0 in hardware mode
	REG(GPIO1_SWPORTD_CTL) = 0;  // GPIO1_PORTD in GPIO mode
	REG(GPIO1_SWPORTD_DDR) |= 0x1;  // GPIO1_PORTD_0 to output

	uart_init(XTI_FREQUENCY, 115200);
	while (1) {
		REG(GPIO1_SWPORTD_DR) = gpio_state;
		uart_puts("Hello, world!\n");
		gpio_state = !gpio_state;
		for (volatile int i = 0; i < 10000; i++) {
			asm volatile ("nop");
		}
	}

	return 0;
}
