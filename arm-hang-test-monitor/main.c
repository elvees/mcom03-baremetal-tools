// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#include <stdint.h>
#include <string.h>

#include <clk.h>
#include <cpu.h>
#include <delay.h>
#include <gpio.h>
#include <regs.h>
#include <uart.h>
#include <wdt.h>

#define DDRINIT_ADDR 0x10000
#define DDRINIT_SIZE 0xf000

#define ARM_HANG_TEST_ADDR 0x20000
#define ARM_HANG_TEST_SIZE 0x2000

#define TEST_RESULT_ADDR       0x8000
#define TEST_MAGIC_ADDR	       0xf000
#define TEST_COUNTER_ADDR      0xf004
#define TEST_FAIL_COUNTER_ADDR 0xf008
#define TEST_MAGIC	       0x12345678

static void hw_init(void)
{
	/* Enable clock to CPU, LSP1, DDR, INTERCONN */
	REG(TOP_CLKGATE) |= BIT(2) | BIT(6) | BIT(7) | BIT(8);

	/* Enable LSPERIPH1 */
	REG(LSPERIPH1_SUBS_PPOLICY) = PP_ON;
	while ((REG(LSPERIPH1_SUBS_PSTATUS) & 0x1f) != PP_ON)
		continue;

	/* Enable CPU subsystem */
	REG(CPU_SUBS_PPOLICY) = PP_ON;
	while ((REG(CPU_SUBS_PSTATUS) & 0x1f) != PP_ON)
		continue;

	/* Enable UART and GPIO clocks */
	REG(LSP1_URB_PLL) = 0;
	REG(LSP1_UCG_CTRL4) = 0x2;
	REG(LSP1_UCG_CTRL6) = 0x2;

	/* Put UART0 pads to hardware mode */
	gpio_set_function_mask(GPIO1, GPIO_BANK_B, BIT(6) | BIT(7), GPIO_FUNC_PERIPH);

	uart_init(UART0, XTI_FREQUENCY, 115200);

	/* Required for ddrinit */
	REG(LSP1_UCG_CTRL7) = 2;
	REG(TIMER_LOAD_COUNT(7)) = 0;
	REG(TIMER_CTRL(7)) = 5;

	wdt_start(0xf);
}

static int clocks_cfg(void)
{
	int ret;
	uint32_t val;
	const uint32_t cpu_divs[] = { 4, 1, 2 };

	/* Set CPU fastlink clock to 594 МГц.
	 * All other fastlink clock remains 27 MHz.
	 */
	UCG_IC_UCG0->BP = 0xff;
	UCG_IC_UCG1->BP = 0x1f5;

	set_pll_man(IC_URB_PLL, 1, 44, 1);
	ret = poll_timeout(REG(IC_URB_PLL), val, val & PLL_LOCK, 0, 100000);
	if (ret) {
		uart_printf(UART0, "IC PLL lock timeout\n");
		return ret;
	}

	ucg_chan_set_div_and_enable(UCG_IC_UCG0, 4, 2, 1);
	ret = poll_timeout(UCG_IC_UCG0->CTR[4], val, val & UCG_DIV_LOCK, 0, 100000);
	if (ret) {
		uart_printf(UART0, "IC UCG0 channel 4 lock timeout\n");
		return ret;
	}

	UCG_IC_UCG0->BP &= ~BIT(4);

	/* Configure CPU clocks */
	for (int i = 1; i < 3; i++) {
		UCG_CPU_UCG0->CTR[i] &= ~0x3;
		UCG_CPU_UCG0->CTR[i] |= 0x2;
	}

	mdelay(1);
	UCG_CPU_UCG0->BP = 7;
	mdelay(1);

	set_pll_man(CPU_URB_PLL, 1, 43, 1);
	ret = poll_timeout(REG(CPU_URB_PLL), val, val & PLL_LOCK, 0, 100000);
	if (ret) {
		uart_printf(UART0, "CPU PLL lock timeout\n");
		return ret;
	}

	for (int i = 0; i < 3; i++) {
		ucg_chan_set_div_and_enable(UCG_CPU_UCG0, i, cpu_divs[i], 1);
		ret = poll_timeout(UCG_CPU_UCG0->CTR[i], val, val & UCG_DIV_LOCK, 0, 100000);
		if (ret) {
			uart_printf(UART0, "CPU UCG channel %d lock timeout\n", i);
			return ret;
		}
	}

	mdelay(1);
	UCG_CPU_UCG0->SYNC = 0x7;
	mdelay(1);
	UCG_CPU_UCG0->BP = 0;
	mdelay(1);

	return 0;
}

int main(void)
{
	int i, ret;
	uint32_t curr, prev, val;
	char *test_result;

	hw_init();

	uart_puts(UART0, "arm-hang-test-monitor: Loading ddrinit... \n");
	memcpy((void *)0x80000000, (void *)DDRINIT_ADDR, DDRINIT_SIZE);

	void (*start_ddrinit)(void) = (void *)0x80000000;
	start_ddrinit();

	ret = clocks_cfg();
	if (ret) {
		while (1)
			continue;
	}

	uart_puts(UART0, "arm-hang-test-monitor: Loading arm-hang-test... \n");

	/* ddrinit maps 0xc000_0000 address to 0x8_9040_0000.
	 * Use it as arm-hang-test start address.
	 */
	memcpy((void *)0xc0000000, (void *)ARM_HANG_TEST_ADDR, ARM_HANG_TEST_SIZE);

	uart_puts(UART0, "arm-hang-test-monitor: Run arm-hang-test... \n");
	for (i = 0; i < 4; i++) {
		start_arm_core(i, 0x890400000);
	}

	mdelay(10);

	val = REG(TEST_MAGIC_ADDR);
	if (val != TEST_MAGIC) {
		REG(TEST_COUNTER_ADDR) = 0;
		REG(TEST_FAIL_COUNTER_ADDR) = 0;
		REG(TEST_MAGIC_ADDR) = TEST_MAGIC;
	}

	val = REG(TEST_COUNTER_ADDR);
	REG(TEST_COUNTER_ADDR) = val + 1;

	gpio_set_function_mask(GPIO1, GPIO_BANK_B, BIT(2) | BIT(3), GPIO_FUNC_GPIO);
	gpio_set_direction_mask(GPIO1, GPIO_BANK_D, BIT(2) | BIT(3), GPIO_DIR_OUT);

	val = REG(TEST_FAIL_COUNTER_ADDR);
	if (val == 0) {
		gpio_set_value_mask(GPIO1, GPIO_BANK_D, 0xC, 0x4);
	} else {
		gpio_set_value_mask(GPIO1, GPIO_BANK_D, 0xC, 0x8);
	}

	prev = REG(TEST_RESULT_ADDR);
	i = 0;
	while (1) {
		curr = REG(TEST_RESULT_ADDR);
		if (curr == prev) {
			if (i++ == 5) {
				test_result = "FAILED";
				val = REG(TEST_FAIL_COUNTER_ADDR);
				REG(TEST_FAIL_COUNTER_ADDR) = val + 1;
				break;
			}
		} else {
			i = 0;
			prev = curr;
		}

		if (curr >= 20) {
			test_result = "PASSED";
			break;
		}

		mdelay(2000);

		uart_printf(UART0, "counters: %d, %d, %d, %d\n", curr,
			    REG(TEST_RESULT_ADDR + 8), REG(TEST_RESULT_ADDR + 16),
			    REG(TEST_RESULT_ADDR + 24));

		wdt_reset();
	}

	uart_printf(UART0, "TEST %s: counters: %d, %d, %d, %d\n", test_result, curr,
		    REG(TEST_RESULT_ADDR + 8), REG(TEST_RESULT_ADDR + 16),
		    REG(TEST_RESULT_ADDR + 24));

	uart_printf(UART0, "TEST FAILED percentage: %d/%d\n", REG(TEST_FAIL_COUNTER_ADDR),
		    REG(TEST_COUNTER_ADDR));

	wdt_start(0);

	while (1)
		;

	return 0;
}
