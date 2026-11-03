// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <clk.h>
#include <console.h>
#include <delay.h>
#include <gpio.h>
#include <mfbsp.h>
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
	CMD_CFG,
	CMD_TS,
	CMD_VM,
	CMD_VERBOSE,
	CMD_TS_MODE_AUTO,
	CMD_TS_MODE_CONT,
	CMD_CAL,
	CMD_SGN_EN,
	CMD_TM_AN,
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
		.cmd = "cfg",
		.help = "Set PVT configuration and reinitialize. Usage: cfg [ts_freq vm_freq]",
		.cmd_id = CMD_CFG,
		.arg_min = 2,
		.arg_max = 2,
		.arg_types = { ARG_UINT, ARG_UINT },
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
		.cmd = "ts_mode_auto",
		.help = "Select manual or auto measure mode (default: auto). 0=manual, 1=auto. Usage: ts_mode_auto <is_auto>",
		.cmd_id = CMD_TS_MODE_AUTO,
		.arg_min = 1,
		.arg_max = 1,
		.arg_types = { ARG_UINT },
	},
	{
		.cmd = "ts_mode_cont",
		.help = "Select single or continuous measure mode (default: single). 0=single, 1=continuous. Usage: ts_mode_cont <is_cont>",
		.cmd_id = CMD_TS_MODE_CONT,
		.arg_min = 1,
		.arg_max = 1,
		.arg_types = { ARG_UINT },
	},
	{
		.cmd = "cal",
		.help = "Set CAL bit (calibration mode) for TS. If non-zero then will read data from external pin. Usage: cal <chan> <value>",
		.cmd_id = CMD_CAL,
		.arg_min = 2,
		.arg_max = 2,
		.arg_types = { ARG_UINT, ARG_UINT },
	},
	{
		.cmd = "sgn_en",
		.help = "Set SGN_EN bit (signature enable) for TS. If non-zero then will read 47.802 or 27.555 degree. Usage: sgn_en <chan> <value>",
		.cmd_id = CMD_SGN_EN,
		.arg_min = 2,
		.arg_max = 2,
		.arg_types = { ARG_UINT, ARG_UINT },
	},
	{
		.cmd = "tm_an",
		.help = "Set TM_AN for TS. Usage: tm_an <chan> <value>",
		.cmd_id = CMD_TM_AN,
		.arg_min = 2,
		.arg_max = 2,
		.arg_types = { ARG_UINT, ARG_UINT },
	},
	{
		.cmd = "tval",
		.help = "Set TVAL bit (test value) for VM. If non-zero then will be read 8960 or 10628 mV. Usage: tval <chan> <value>",
		.cmd_id = CMD_TVAL,
		.arg_min = 2,
		.arg_max = 2,
		.arg_types = { ARG_UINT, ARG_UINT },
	},
	{
		.cmd = "sel_vin",
		.help = "Set SEL_VIN for VM (for debugging purposes). Range [0..16]. Usage: sel_vin <chan> <value>",
		.cmd_id = CMD_SEL_VIN,
		.arg_min = 2,
		.arg_max = 2,
		.arg_types = { ARG_UINT, ARG_UINT },
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

static void print_ts_result(struct console *console, uint32_t channel, const char *name)
{
	int32_t value = pvt_ts_measure_mcelsius(channel);

	if (value == PVT_MEASURE_ERROR)
		uart_printf(console->uart, "Channel[%d:%s]: error\n",
			    channel, name);
	else
		uart_printf(console->uart, "Channel[%d:%s]: %d m" DEGREE_CELSIUS_UTF8_SYM "\n",
			    channel, name, value);
}

static void print_vm_result(struct console *console, uint32_t channel, const char *name)
{
	int32_t value = pvt_vm_measure_mv(channel);

	if (value == PVT_MEASURE_ERROR)
		uart_printf(console->uart, "Channel[%d:%s]: error\n",
			    channel, name);
	else
		uart_printf(console->uart, "Channel[%d:%s]: %d mV\n",
			    channel, name, value);
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
			print_ts_result(console, channel, channel_names[channel]);
		} else {
			for (channel = 0; channel < 4; channel++)
				print_ts_result(console, channel, channel_names[channel]);
		}

		break;
	case CMD_VM:
		if (argc) {
			channel = args[0].uint;
			print_vm_result(console, channel, channel_names[channel]);
		} else {
			for (channel = 0; channel < 4; channel++)
				print_vm_result(console, channel, channel_names[channel]);
		}

		break;
	case CMD_CFG:
		pvt_configure(args[0].uint, args[1].uint);
		value = pvt_get_verbose();
		pvt_set_verbose(VERBOSE_LEVEL_INFO);
		pvt_init();
		pvt_set_verbose(value);
		break;
	case CMD_VERBOSE:
		if (argc)
			pvt_set_verbose(args[0].uint);
		else
			uart_printf(UART0, "verbose: %d\n", pvt_get_verbose());

		break;
	case CMD_TS_MODE_AUTO:
		pvt_ts_set_auto_mode(args[0].uint);
		break;
	case CMD_TS_MODE_CONT:
		pvt_ts_set_continuous_mode(args[0].uint);
		break;
	case CMD_CAL:
		pvt_ts_set_cal(args[0].uint, args[1].uint);
		break;
	case CMD_SGN_EN:
		pvt_ts_set_sgn_en(args[0].uint, args[1].uint);
		break;
	case CMD_TM_AN:
		pvt_ts_set_tm_an(args[0].uint, args[1].uint);
		break;
	case CMD_TVAL:
		pvt_vm_set_tval(args[0].uint, args[1].uint);
		break;
	case CMD_SEL_VIN:
		pvt_vm_set_sel_vin(args[0].uint, args[1].uint);
		break;
	default:
		break;
	}
}

static void setup_lsp1_voltage(void)
{
// BootROM is accessible only for RISC0
#ifdef MIPS32
	uint32_t boot = REG(SERVICE_URB + 0x2000);
	uint32_t bootrom_rev;

	// We can not read BootROM in XIP boot mode
	if ((boot & 0x7) == 0)
		return;

	bootrom_rev = REG(SERVICE_BASE + 0xc00530);

	/* BootROM revision 2 means that this is MCom-03R. In this case if MFBSP0_LDAT4 is tied
	 * high on the board then LSPeriph1 pads is powered by 1.8V. */
	if (bootrom_rev == 2) {
		bool is_mfbsp0_enabled = ucg_chan_is_enabled(UCG_SERVICE_UCG0, 5);

		if (!is_mfbsp0_enabled) {
			ucg_chan_enable(UCG_SERVICE_UCG0, 5, 1); // Enable MFBSP0 clock
			udelay(1);
		}

		mfbsp_gpio_set_pin_dir(MFBSP0, MFBSP_LDAT4, 0);
		udelay(1);
		if (mfbsp_gpio_get_pin_value(MFBSP0, MFBSP_LDAT4))
			REG(LSP1_URB_GPIO1_V18) = 0x1;

		if (!is_mfbsp0_enabled)
			ucg_chan_enable(UCG_SERVICE_UCG0, 5, 0); // Disable MFBSP0 clock
	}
#endif
}

int main(void)
{
	const uint32_t service_ucg_divs[] = {
		APB_DIV,
		CORE_DIV,
		CORE_DIV,
		CORE_DIV,
		CORE_DIV,
		APB_DIV,
		APB_DIV,
		APB_DIV,
		APB_DIV,
		APB_DIV,
		APB_DIV,
		APB_DIV,
		12,
		8,
		1,
		1,
	};
	int ret;

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
	ret = clk_ucg_setup(UCG_SERVICE_UCG0, service_ucg_divs, ARRAY_LENGTH(service_ucg_divs),
			    0xffff, SERVICE_URB_PLL, 1);

	set_tick_freq(clk_pll_calc_freq(REG(SERVICE_URB_PLL), XTI_FREQUENCY) / CORE_DIV);
	setup_lsp1_voltage();

	ucg_chan_enable(UCG_SERVICE_UCG0, 8, 1); // Enable PVT clock
	clk_pll_set_value(LSP1_URB_PLL, 0x2); // PLL output frequency = 81 MHz
	ucg_chan_set_div_and_enable(UCG_LSP1_UCG0, 6, 1, 1); // UART_CLK div=1 (81 MHz)

	// Setup UART0 pads
	REG(LSP1_URB_PAD_CTR(PORTB, 6)) = PAD_CTL_CTL(0x7) | PAD_CTL_SL(0x3);
	REG(LSP1_URB_PAD_CTR(PORTB, 7)) = PAD_CTL_E | PAD_CTL_CTL(0x7) | PAD_CTL_SL(0x3) |
					  PAD_CTL_SUS;

	// UART0 in hardware mode
	gpio_set_function_mask(GPIO1, GPIO_BANK_B, BIT(6) | BIT(7), GPIO_FUNC_PERIPH);

	uart_init(UART0, clk_pll_calc_freq(REG(LSP1_URB_PLL), XTI_FREQUENCY), 115200);
	uart_puts(UART0, "PVT demo\n");
	if (ret)
		uart_puts(UART0, "Error during SERVICE clocks initialization\n");

	pvt_set_verbose(VERBOSE_LEVEL_INFO);
	pvt_init();
	pvt_set_verbose(VERBOSE_LEVEL_ERROR);

	console_cmd_line_restore(&console);
	while (1) {
		console_process(&console);
	}

	return 0;
}
