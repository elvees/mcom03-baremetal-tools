// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <clk.h>
#include <console.h>
#include <delay.h>
#include <snps_ssi.h>
#include <otp.h>
#include <gpio.h>
#include <regs.h>
#include <uart.h>

/*
 * Turn A into a string literal without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded).
 */
#define STRINGIZE_NX(A) #A

/* Turn A into a string literal after macro-expanding it. */
#define STRINGIZE(A) STRINGIZE_NX(A)

#define APP_NAME \
	"OTP Flasher (commit: '" STRINGIZE(GIT_SHA1_SHORT) "', build: '" STRINGIZE(BUILD_ID) "')"

enum cmd_ids {
	CMD_HELP,
	CMD_PROGRAM,
	CMD_PROGRAM_RAW,
	CMD_READ,
	CMD_BIST,
	CMD_BISR,
};

uint32_t buffer[OTP_WORDS_COUNT];
uint8_t buffer_ecc[OTP_WORDS_COUNT];

// Globals
struct console_cmd console_cmd[] = {
	{
		.cmd_id = CMD_HELP,
		.cmd = "help",
		.help = "show this message",
		.arg_min = 0,
		.arg_max = 0,
	},
	{
		.cmd_id = CMD_PROGRAM,
		.cmd = "program",
		.help = "program data to OTP memory (required binary data) : program <otp_addr>",
		.arg_min = 1,
		.arg_max = 1,
		.arg_types = { ARG_UINT },
	},
	{
		.cmd_id = CMD_PROGRAM_RAW,
		.cmd = "program_raw",
		.help = "program data with custom ECC to OTP memory : program <otp_addr> <value> <ecc>",
		.arg_min = 3,
		.arg_max = 3,
		.arg_types = { ARG_UINT, ARG_UINT, ARG_UINT },
	},
	{
		.cmd_id = CMD_READ,
		.cmd = "read",
		.help = "read OTP data: read <otp_addr> <count> [ecc_brp_flags]",
		.arg_min = 2,
		.arg_max = 3,
		.arg_types = { ARG_UINT, ARG_UINT, ARG_UINT },
	},
	{
		.cmd_id = CMD_BIST,
		.cmd = "bist",
		.help = "found non-clean cell or leaky bits: otp <otp_addr> <count>",
		.arg_min = 2,
		.arg_max = 2,
		.arg_types = { ARG_UINT, ARG_UINT },
	},
	{
		.cmd_id = CMD_BISR,
		.cmd = "bisr",
		.help = "repair leaky bits: otp <otp_addr> <count>",
		.arg_min = 2,
		.arg_max = 2,
		.arg_types = { ARG_UINT, ARG_UINT },
	},
};

unsigned long __stack_chk_guard;
void __stack_chk_fail(void)
{
}

static uint16_t crc16_init(void)
{
	return 0xffff;
}

static uint16_t crc16_update_byte(uint16_t crc, uint8_t data)
{
	crc ^= (uint16_t)data << 8;
	for (int i = 0; i < 8; i++)
		crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;

	return crc;
}

static void bist(uint32_t otp_addr, uint32_t count, int is_bisr)
{
	uint16_t err_addr = 0;
	int ret;

	if ((otp_addr + count) > OTP_WORDS_COUNT) {
		uart_puts(UART0, "Error: Size is too large\n");
		return;
	}

	ret = otp_bist(otp_addr, count, is_bisr, &err_addr);
	if (ret)
		uart_printf(UART0, "BIST error%s at OTP address %d\n",
			    (ret == OTP_ERR_BUS ? " (bus timeout)" : ""), err_addr);
	else
		uart_puts(UART0, "Ok\n");
}

static void program_data(uint32_t otp_addr)
{
	char *err = "";
	uint16_t block_size;
	uint16_t expected_crc;
	uint16_t crc;
	uint16_t err_addr = 0;
	uint8_t *p_buf8 = (uint8_t *)buffer;
	int ret;

	block_size = uart_getchar(UART0);
	block_size |= (uint16_t)uart_getchar(UART0) << 8;
	expected_crc = uart_getchar(UART0);
	expected_crc |= (uint16_t)uart_getchar(UART0) << 8;
	if (!block_size) {
		uart_puts(UART0, "E\nSize can not be zero\n");
		return;
	} else if ((block_size % sizeof(uint32_t)) != 0) {
		uart_puts(UART0, "E\nSize must be aligned by 32-bit word\n");
		return;
	} else if ((otp_addr + (block_size / sizeof(uint32_t))) > OTP_WORDS_COUNT) {
		uart_puts(UART0, "E\nSize is too large\n");
		return;
	}
	uart_putc(UART0, 'R');

	crc = crc16_init();
	for (unsigned i = 0; i < block_size; i++) {
		p_buf8[i] = uart_getchar(UART0);
		crc = crc16_update_byte(crc, p_buf8[i]);
	}

	if (crc != expected_crc) {
		uart_printf(UART0, "E\nCorrupted data from UART (CRC %#x != %#x)\n", crc, expected_crc);
		return;
	}

	ret = otp_program(buffer, NULL, otp_addr, block_size / 4, &err_addr);
	if (ret == OTP_ERR_BUS)
		err = ": Internal SBPI bus timeout";
	else if (ret == OTP_ERR_PROG_SOAK_LIMIT)
		err = ": Soak limit exceeded";
	else if (ret == OTP_ERR_PROG_COMPARE)
		err = ": Compare mismatch";

	if (ret)
		uart_printf(UART0, "E\nOTP program failed at address %d %s\n", err_addr, err);
	else
		uart_putc(UART0, 'R');
}

static void program_raw(uint32_t otp_addr, uint32_t value, uint32_t ecc)
{
	int ret;
	char *err = "";

	if (ecc > 0xff) {
		uart_printf(UART0, "Error: ECC must be in [0..0xFF]\n");
		return;
	}

	if (otp_addr >= OTP_WORDS_COUNT) {
		uart_printf(UART0, "Error: Incorrect address\n");
		return;
	}

	buffer[0] = value;
	buffer_ecc[0] = ecc & 0xff;
	ret = otp_program(buffer, buffer_ecc, otp_addr, 1, NULL);
	if (ret == OTP_ERR_BUS)
		err = ": Internal SBPI bus timeout";
	else if (ret == OTP_ERR_PROG_SOAK_LIMIT)
		err = ": Soak limit exceeded";
	else if (ret == OTP_ERR_PROG_COMPARE)
		err = ": Compare mismatch";

	if (ret)
		uart_printf(UART0, "OTP program failed%s\n", err);
	else
		uart_puts(UART0, "Done\n");
}

static void read_words(uint32_t idx_first, uint32_t count, uint32_t flags)
{
	if (idx_first >= OTP_WORDS_COUNT || (idx_first + count) > OTP_WORDS_COUNT) {
		uart_printf(UART0, "Error: Index or count is too big\n");
		return;
	}

	if (flags & ~OTP_FLAG_MASK) {
		uart_printf(UART0, "Error: Incorrect flags\n");
		return;
	}

	otp_read(buffer, buffer_ecc, idx_first, count, flags);
	for (uint32_t i = 0; i < count; i++) {
		uint32_t ecc = otp_calculate_ecc(buffer[i]);
		char *ecc_check = ecc == (buffer_ecc[i] & 0x3f) ? "ok" : "error";
		uart_printf(UART0, "[%d] Data: %#x, ECC: %#x (%s)\n", idx_first + i, buffer[i],
			    buffer_ecc[i], ecc_check);
	}
}

void console_run(struct console *console, struct console_cmd *cmd, struct console_arg *args,
		 int argc)
{
	uint32_t flags;

	switch (cmd->cmd_id) {
	case CMD_HELP:
		console_help(console);
		break;
	case CMD_PROGRAM:
		uart_puts(UART0, "Ready for data\n#");
		program_data(args[0].uint);
		break;
	case CMD_PROGRAM_RAW:
		program_raw(args[0].uint, args[1].uint, args[2].uint);
		break;
	case CMD_READ:
		flags = argc > 2 ? args[2].uint : 0;
		read_words(args[0].uint, args[1].uint, flags);
		break;
	case CMD_BIST:
		bist(args[0].uint, args[1].uint, 0);
		break;
	case CMD_BISR:
		bist(args[0].uint, args[1].uint, 1);
		break;
	default:
		break;
	}
}

struct console console = {
	.uart = UART0,
	.cmds = console_cmd,
	.cmds_count = ARRAY_LENGTH(console_cmd),
	.prompt = "#",
	.app_name = APP_NAME,
	.each_line_msg = APP_NAME,
	.run_cmd = console_run,
};

static uint32_t get_ucg_enabled_mask(struct ucg *ucg)
{
	uint32_t mask = 0;

	for (int i = 0; i < 16; i++) {
		if (ucg_chan_is_enabled(ucg, i))
			mask |= BIT(i);
	}

	return mask;
}

int main()
{
	uint32_t ucg_div_apb;
	uint32_t ucg_div_core;
	uint32_t ucg_enabled_mask;

	// Enable clock to LSPERIPH1 (HSPERIPH is enabled by default)
	REG(TOP_CLKGATE) |= BIT(6);

	REG(LSPERIPH1_SUBS_PPOLICY) = PP_ON; // Enable LSPERIPH1
	while ((REG(LSPERIPH1_SUBS_PSTATUS) & 0x1f) != PP_ON)
		continue;

	ucg_enabled_mask = get_ucg_enabled_mask(UCG_SERVICE_UCG0);
	UCG_SERVICE_UCG0->BP = ucg_enabled_mask;
	udelay(1);
	REG(SERVICE_URB_PLL) = 7; // SERVICE PLL to 216 MHz
	udelay(1);
	ucg_div_apb = DIV_ROUND_UP(216, 25); // APB must be 25 MHz or less to works with OTP
	ucg_div_core = DIV_ROUND_UP(216, 216);
	ucg_chan_set_div(UCG_SERVICE_UCG0, 0, ucg_div_apb); // CLK_APB
	ucg_chan_set_div(UCG_SERVICE_UCG0, 1, ucg_div_core); // CLK_CORE
	ucg_chan_set_div(UCG_SERVICE_UCG0, 2, ucg_div_core); // CLK_QSPI0
	ucg_chan_set_div(UCG_SERVICE_UCG0, 3, ucg_div_core); // CLK_BPAM
	ucg_chan_set_div(UCG_SERVICE_UCG0, 4, ucg_div_core); // CLK_RISC0
	ucg_chan_set_div(UCG_SERVICE_UCG0, 5, ucg_div_apb); // CLK_MFBSP0
	ucg_chan_set_div(UCG_SERVICE_UCG0, 6, ucg_div_apb); // CLK_MFBSP1
	ucg_chan_set_div(UCG_SERVICE_UCG0, 7, ucg_div_apb); // CLK_MAILBOX0
	ucg_chan_set_div(UCG_SERVICE_UCG0, 8, ucg_div_apb); // CLK_PVTCTR
	ucg_chan_set_div(UCG_SERVICE_UCG0, 9, ucg_div_apb); // CLK_I2C4
	ucg_chan_set_div(UCG_SERVICE_UCG0, 10, ucg_div_apb); // CLK_TRNG
	ucg_chan_set_div(UCG_SERVICE_UCG0, 11, ucg_div_apb); // CLK_SPIOTP
	udelay(1);
	UCG_SERVICE_UCG0->SYNC = 0xfff;
	UCG_SERVICE_UCG0->BP = 0;
	ucg_chan_enable(UCG_SERVICE_UCG0, 11, 1); // CLK_SPIOTP

	// For MIPS timer uses RISC0 frequency. For ARM this function ignored.
	set_tick_freq(XTI_FREQUENCY * 8);

	// Turn PLLs to bypass mode
	REG(HSP_URB_PLL) = 0;
	REG(LSP1_URB_PLL) = 0;

	ucg_chan_set_div_and_enable(UCG_LSP1_UCG0, 6, 1, 1); // UART0_CLK div=1 (27 MHz)

	// Setup UART0 pads
	REG(LSP1_URB_PAD_CTR(PORTB, 6)) = PAD_CTL_CTL(0x7) | PAD_CTL_SL(0x3);
	REG(LSP1_URB_PAD_CTR(PORTB, 7)) = PAD_CTL_E | PAD_CTL_CTL(0x7) | PAD_CTL_SL(0x3) |
					  PAD_CTL_SUS;
	// UART0 in hardware mode
	gpio_set_function_mask(GPIO1, GPIO_BANK_B, BIT(6) | BIT(7), GPIO_FUNC_PERIPH);

	uart_flush(UART0);
	uart_init(UART0, XTI_FREQUENCY, 115200);
	REG(SERVICE_URB_OTP_MODE) = 0;
	SBPI_initMaster(SPI_OTP);
	uart_printf(UART0, "%s\n#", APP_NAME);

	while (1) {
		console_process(&console);
	}

	return 0;
}
