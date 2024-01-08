// SPDX-License-Identifier: MIT
// Copyright 2021-2024 RnD Center "ELVEES", JSC

#include <stdint.h>

#include <gpio.h>
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

	// UART0 in hardware mode
	gpio_set_function_mask(GPIO1, GPIO_BANK_B, BIT(6) | BIT(7), GPIO_FUNC_PERIPH);

	// GPIO1_PORTD_0 in GPIO mode
	gpio_set_function(GPIO1, GPIO_BANK_D, 0, GPIO_FUNC_GPIO);

	// GPIO1_PORTD_0 to output
	gpio_set_direction(GPIO1, GPIO_BANK_D, 0, GPIO_DIR_OUT);

	uart_init(UART0, XTI_FREQUENCY, 115200);
	while (1) {
		gpio_set_value(GPIO1, GPIO_BANK_D, 0, gpio_state);
		uart_puts(UART0, "Hello, world!\n");
		gpio_state = !gpio_state;
		for (volatile int i = 0; i < 10000; i++) {
			asm volatile ("nop");
		}
	}

	return 0;
}
