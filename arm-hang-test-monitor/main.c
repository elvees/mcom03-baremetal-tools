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

#define STRINGIZE_NX(A) #A
#define STRINGIZE(A)	STRINGIZE_NX(A)

#define DDRINIT_ADDR 0x10000
#define DDRINIT_SIZE 0xf000

#define ARM_HANG_TEST_ADDR 0x20000
#define ARM_HANG_TEST_SIZE 0x2000

#define TEST_RESULT_ADDR	     0x8000
#define TEST_MAGIC_ADDR		     0xf000
#define TEST_COUNTER_ADDR	     0xf004
#define TEST_FAIL_COUNTER_ADDR	     0xf008
#define TEST_MAGIC		     0x12345678
#define TEST_JTAG_SYNC_ADDR	     0xf00c
#define TEST_JTAG_SYNC_MAGIC	     0x87654321
#define TEST_MAX_ITERS_DEFAULT	     20
#define TEST_MAX_ITERS_OVERRIDE_ADDR 0xf010

/* Parameters that can be redefined externally in MDB scripts */
int top_pll_nf = 43;
int top_cpu_div = 2;

int cpu_pll_nf = 42;
int cpu_dbus_div = 2;

int ddr_pll1_nf = 43;
int ddr_cpu_div = 2;

int reset_counters_interval = 2000;

uint32_t cpu_pll_status = 0;
uint32_t top_pll_status = 0;
uint32_t ddr_pll_status = 0;

#ifndef JTAG_BUILD
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
#endif

static int clocks_cfg(void)
{
	int ret;
	uint32_t val;
	uint32_t cpu_divs[3];

	cpu_divs[0] = cpu_dbus_div * 2;
	cpu_divs[1] = cpu_dbus_div / 2;
	cpu_divs[2] = cpu_dbus_div;

	/* Set CPU fastlink clock to 594 МГц.
	 * All other fastlink clock remains 27 MHz.
	 */
	UCG_IC_UCG0->BP = 0xff;
	UCG_IC_UCG1->BP = 0x1f5;

	set_pll_man(IC_URB_PLL, 1, top_pll_nf + 1, 1);
	ret = poll_timeout(REG(IC_URB_PLL), val, val & PLL_LOCK, 0, 100000);
	if (ret) {
		uart_printf(UART0, "IC PLL lock timeout\n");
		return ret;
	}

	ucg_chan_set_div_and_enable(UCG_IC_UCG0, 4, top_cpu_div, 1);
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

	set_pll_man(CPU_URB_PLL, 1, cpu_pll_nf + 1, 1);
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

static void read_pll_status(uint32_t ticks_count)
{
	unsigned long tick_start = _get_tick_counter();
	unsigned long tick = tick_start + ticks_count;

	if (tick < tick_start) {
		while (_get_tick_counter() > tick_start) {
			cpu_pll_status |= REG(CPU_URB_PLL_DIAG);
			ddr_pll_status |= REG(DDR_URB_PLL1_DIAG);
			top_pll_status |= REG(IC_URB_PLL_DIAG);
		}
	}

	while (_get_tick_counter() < tick) {
		cpu_pll_status |= REG(CPU_URB_PLL_DIAG);
		ddr_pll_status |= REG(DDR_URB_PLL1_DIAG);
		top_pll_status |= REG(IC_URB_PLL_DIAG);
	}
}

void mdelay_and_read_pll_status(uint32_t ms)
{
	uint32_t ticks_per_s = _get_tick_freq();
	uint32_t ticks_per_ms = ticks_per_s / 1000;

	while (ms > 1000) {
		read_pll_status(ticks_per_s);
		ms -= 1000;
	}
	read_pll_status(ms * ticks_per_ms);
}

int main(void)
{
	int i, ret;
	uint32_t curr, prev, val;
	char *test_result;
	int max_iters = TEST_MAX_ITERS_DEFAULT;

#ifdef JTAG_BUILD
	while (REG(TEST_JTAG_SYNC_ADDR) != TEST_JTAG_SYNC_MAGIC)
		continue;
	max_iters = REG(TEST_MAX_ITERS_OVERRIDE_ADDR);
#else
	hw_init();

	/* Load ddrinit */
	memcpy((void *)0x80000000, (void *)DDRINIT_ADDR, DDRINIT_SIZE);

	void (*start_ddrinit)(void) = (void *)0x80000000;
	start_ddrinit();
#endif
	uart_printf(UART0,
		    "arm-hang-test-monitor: version: '" STRINGIZE(GIT_SHA1_SHORT) "', build: '" STRINGIZE(BUILD_ID) "'\n");

	ret = clocks_cfg();
	if (ret) {
		while (1)
			continue;
	}

#ifndef JTAG_BUILD
	uart_puts(UART0, "arm-hang-test-monitor: Loading arm-hang-test... \n");

	/* ddrinit maps 0xc000_0000 address to 0x8_9040_0000.
	 * Use it as arm-hang-test start address.
	 */
	memcpy((void *)0xc0000000, (void *)ARM_HANG_TEST_ADDR, ARM_HANG_TEST_SIZE);
#endif
	uart_puts(UART0, "arm-hang-test-monitor: Run arm-hang-test... \n");
	for (i = 0; i < 4; i++) {
		start_arm_core(i, 0x890400000);
	}

	mdelay(10);

	val = REG(TEST_MAGIC_ADDR);
	if ((val != TEST_MAGIC) || (REG(TEST_COUNTER_ADDR) >= reset_counters_interval)) {
		REG(TEST_COUNTER_ADDR) = 0;
		REG(TEST_FAIL_COUNTER_ADDR) = 0;
		REG(TEST_MAGIC_ADDR) = TEST_MAGIC;
	}

	val = REG(TEST_COUNTER_ADDR);
	REG(TEST_COUNTER_ADDR) = val + 1;

#ifdef BOARD_ELVMC03SMARC_R3
	gpio_set_function_mask(GPIO1, GPIO_BANK_B, BIT(2) | BIT(3), GPIO_FUNC_GPIO);
	gpio_set_direction_mask(GPIO1, GPIO_BANK_D, BIT(2) | BIT(3), GPIO_DIR_OUT);

	val = REG(TEST_FAIL_COUNTER_ADDR);
	if (val == 0) {
		gpio_set_value_mask(GPIO1, GPIO_BANK_D, 0xC, 0x4);
	} else {
		gpio_set_value_mask(GPIO1, GPIO_BANK_D, 0xC, 0x8);
	}
#endif
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

		if (curr >= max_iters) {
			test_result = "PASSED";
			break;
		}

		mdelay_and_read_pll_status(2000);

		uart_printf(UART0, "counters: %d, %d, %d, %d\n", curr,
			    REG(TEST_RESULT_ADDR + 8), REG(TEST_RESULT_ADDR + 16),
			    REG(TEST_RESULT_ADDR + 24));

#ifndef JTAG_BUILD
		wdt_reset();
#endif
	}

	uart_printf(UART0, "TEST %s: counters: %d, %d, %d, %d\n", test_result, curr,
		    REG(TEST_RESULT_ADDR + 8), REG(TEST_RESULT_ADDR + 16),
		    REG(TEST_RESULT_ADDR + 24));

	uart_printf(UART0, "TEST FAILED percentage: %d/%d\n", REG(TEST_FAIL_COUNTER_ADDR),
		    REG(TEST_COUNTER_ADDR));

	uart_printf(UART0, "DDR/TOP/CPU PLL NF: %d/%d/%d\n", ddr_pll1_nf, top_pll_nf, cpu_pll_nf);

	uart_printf(UART0, "%d;%d;%d;%d/%d\n",
		    27 * (ddr_pll1_nf + 1) / ddr_cpu_div,
		    27 * (top_pll_nf + 1) / top_cpu_div,
		    27 * (cpu_pll_nf + 1) / cpu_dbus_div,
		    REG(TEST_FAIL_COUNTER_ADDR),
		    REG(TEST_COUNTER_ADDR));

	if (FIELD_GET(PLL_DIAG_RFSLIP, ddr_pll_status) ||
	    FIELD_GET(PLL_DIAG_FBSLIP, ddr_pll_status) ||
	    FIELD_GET(PLL_DIAG_RFSLIP, top_pll_status) ||
	    FIELD_GET(PLL_DIAG_FBSLIP, top_pll_status) ||
	    FIELD_GET(PLL_DIAG_RFSLIP, cpu_pll_status) ||
	    FIELD_GET(PLL_DIAG_FBSLIP, cpu_pll_status))
		uart_printf(UART0, "PLL RF/FB slip detected!: 0x%x, 0x%x, 0x%x\n",
			    ddr_pll_status, top_pll_status, cpu_pll_status);

#ifndef JTAG_BUILD
	wdt_start(0);
#else
	REG(TEST_JTAG_SYNC_ADDR) = 0x0;
#endif
	return 0;
}
