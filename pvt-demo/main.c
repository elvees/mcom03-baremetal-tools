// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <clk.h>
#include <console.h>
#include <delay.h>
#include <gpio.h>
#include <pvt.h>
#include <regs.h>
#include <uart.h>

#include <sys/types.h>

#define APB_DIV	 1
#define CORE_DIV 1

#define DEGREE_CELSIUS_UTF8_SYM "\xe2\x84\x83"

// TODO: move to separate file (maybe common.c)
// GCC uses __stack_chk_fail as stack overflow handler
unsigned long __stack_chk_guard;

enum {
	CMD_HELP,
	CMD_TS,
	CMD_VM,
	CMD_VERBOSE,
	CMD_TVAL,
	CMD_SEL_VIN,
};

struct console_cmd console_cmd[] = {
	{
		.cmd = "help",
		.help = "Show help",
		.cmd_id = CMD_HELP,
	},
	{
		.cmd = "ts",
		.help = "Show temperature. Usage: ts [subsystem], where subsystem 0:CPU, 1:SDR, 2:MEDIA, 3:SERVICE",
		.cmd_id = CMD_TS,
		.arg_min = 0,
		.arg_max = 1,
		.arg_types = { ARG_UINT },
	},
	{
		.cmd = "vm",
		.help = "Show voltage. Usage: vm [subsystem], where subsystem 0:CPU, 1:SDR, 2:MEDIA, 3:SERVICE",
		.cmd_id = CMD_VM,
		.arg_min = 0,
		.arg_max = 1,
		.arg_types = { ARG_UINT },
	},
	{
		.cmd = "verbose",
		.help = "Set verbose level. Usage: verbose [level], where level in range [0..3]",
		.cmd_id = CMD_VERBOSE,
		.arg_min = 0,
		.arg_max = 1,
		.arg_types = { ARG_UINT },
	},
	{
		.cmd = "tval",
		.help = "Set TVAL bit (test value) for VM. If non-zero then will be read 8960 or 10628 mV. Usage: tval <value>",
		.cmd_id = CMD_TVAL,
		.arg_min = 1,
		.arg_max = 1,
		.arg_types = { ARG_UINT },
	},
	{
		.cmd = "sel_vin",
		.help = "Set SEL_VIN for VM (for debugging purposes). Usage: sel_vin <value>",
		.cmd_id = CMD_SEL_VIN,
		.arg_min = 1,
		.arg_max = 1,
		.arg_types = { ARG_UINT },
	},
};

void console_run(struct console *console, struct console_cmd *cmd, struct console_arg *args,
		 int argc);

struct console console = {
	.uart = UART0,
	.cmds = console_cmd,
	.cmds_count = ARRAY_LENGTH(console_cmd),
	.prompt = "[pvt-demo]# ",
	.app_name = "PVT test application",
	.run_cmd = console_run,
};

void __stack_chk_fail(void)
{
}

void console_run(struct console *console, struct console_cmd *cmd, struct console_arg *args,
		 int argc)
{
	uint32_t value;
	uint32_t channel;
	const char *const channel_names[4] = { "CPU    ", "SDR    ", "Media  ", "Service" };

	switch (cmd->cmd_id) {
	case CMD_HELP:
		console_help(console);
		break;
	case CMD_TS:
		if (argc) {
			channel = args[0].uint;
			value = pvt_ts_measure_mcelsius(channel);
			uart_printf(console->uart, "Channel[%d:%s]: %d m" DEGREE_CELSIUS_UTF8_SYM "\n",
				    channel, channel_names[channel], value);
		} else {
			for (channel = 0; channel < 4; channel++) {
				value = pvt_ts_measure_mcelsius(channel);
				uart_printf(console->uart,
					    "Channel[%d:%s]: %d m" DEGREE_CELSIUS_UTF8_SYM "\n",
					    channel, channel_names[channel], value);
			}
		}

		break;
	case CMD_VM:
		if (argc) {
			channel = args[0].uint;
			value = pvt_vm_measure_mv(channel);
			uart_printf(console->uart, "Channel[%d:%s]: %d mV\n",
				    channel, channel_names[channel], value);
		} else {
			for (channel = 0; channel < 4; channel++) {
				value = pvt_vm_measure_mv(channel);
				uart_printf(console->uart, "Channel[%d:%s]: %d mV\n",
					    channel, channel_names[channel], value);
			}
		}

		break;
	case CMD_VERBOSE:
		if (argc)
			pvt_set_verbose(args[0].uint);
		else
			uart_printf(UART0, "verbose: %d\n", pvt_get_verbose());

		break;
	case CMD_TVAL:
		pvt_vm_set_tval(args[0].uint);
		break;
	case CMD_SEL_VIN:
		pvt_vm_set_sel_vin(args[0].uint);
		break;
	default:
		break;
	}
}

int main(void)
{
	REG(LSPERIPH1_SUBS_PPOLICY) = PP_ON; // Enable LSPERIPH1
	REG(HSPERIPH_SUBS_PPOLICY) = PP_ON; // Enable HSPERIPH
	REG(CPU_SUBS_PPOLICY) = PP_ON; // Enable CPU
	REG(SDR_SUBS_PPOLICY) = PP_ON; // Enable SDR
	REG(MEDIA_SUBS_PPOLICY) = PP_ON; // Enable MEDIA

	// Enable clock to LSPERIPH1, HSPERIPH, SDR, CPU and MEDIA
	REG(TOP_CLKGATE) |= BIT(6) | BIT(4) | BIT(3) | BIT(2) | BIT(1);

	while ((REG(LSPERIPH1_SUBS_PSTATUS) & 0x1f) != PP_ON ||
	       (REG(HSPERIPH_SUBS_PSTATUS) & 0x1f) != PP_ON ||
	       (REG(CPU_SUBS_PPOLICY) & 0x1f) != PP_ON ||
	       (REG(SDR_SUBS_PPOLICY) & 0x1f) != PP_ON ||
	       (REG(MEDIA_SUBS_PPOLICY) & 0x1f) != PP_ON) {
	}

	set_tick_freq(XTI_FREQUENCY);
	UCG_SERVICE_UCG0->BP = 0x20b7;
	REG(SERVICE_URB_PLL) = 1; // 54 MHz
	udelay(1000);
	for (int i = 0; i < 16; i++)
		UCG_SERVICE_UCG0->CTR[i] &= !BIT(0);

	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 0, APB_DIV, 1);
	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 1, CORE_DIV, 1);
	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 2, CORE_DIV, 1);
	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 3, CORE_DIV, 0);
	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 4, CORE_DIV, 1);
	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 5, APB_DIV, 1);
	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 6, APB_DIV, 0);
	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 7, APB_DIV, 1);
	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 8, APB_DIV, 0);
	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 9, APB_DIV, 0);
	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 10, APB_DIV, 0);
	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 11, APB_DIV, 0);
	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 12, 12, 0);
	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 13, 8, 1);
	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 14, 1, 0);
	ucg_chan_set_div_and_enable(UCG_SERVICE_UCG0, 15, 1, 0);
	udelay(1000);
	UCG_SERVICE_UCG0->SYNC = 0xffff;
	udelay(10);
	UCG_SERVICE_UCG0->BP = 0;

	set_tick_freq(((REG(SERVICE_URB_PLL) & 0xff) + 1) * XTI_FREQUENCY);

	udelay(10);
	ucg_chan_enable(UCG_SERVICE_UCG0, 8, 1); // Enable PVT clock
	udelay(10);

	REG(LSP1_URB_PLL) = 0; // Turn PLL to bypass mode
	ucg_chan_set_div_and_enable(UCG_LSP1_UCG0, 6, 0, 1); // UART_CLK

	// Setup UART0 pads
	REG(LSP1_URB_PAD_CTR(PORTB, 6)) = PAD_CTL_CTL(0x7) | PAD_CTL_SL(0x3);
	REG(LSP1_URB_PAD_CTR(PORTB, 7)) = PAD_CTL_E | PAD_CTL_CTL(0x7) | PAD_CTL_SL(0x3) |
					  PAD_CTL_SUS;

	// UART0 in hardware mode
	gpio_set_function_mask(GPIO1, GPIO_BANK_B, BIT(6) | BIT(7), GPIO_FUNC_PERIPH);

	uart_init(UART0, XTI_FREQUENCY, 115200);
	uart_puts(UART0, "PVT demo\n");

	pvt_init();

	console_cmd_line_restore(&console);
	while (1) {
		console_process(&console);
	}

	return 0;
}
