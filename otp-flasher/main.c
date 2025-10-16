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
	CMD_READ,
	CMD_READ_ECC,
	CMD_BIST,
	CMD_VERIFY,
};

// Parameters synthesis of RTL OTP
SNPS_SSI_Parameters_s Params;

// Writer arguments
OTP_error_t error = 0;
uint32_t buffer[OTP_SIZE / sizeof(uint32_t)];
uint32_t address = 0, size = 0, ecc_brp_flag = OTP_ECC_BRP_DEFAULT, test_mode = 0;
char operation;

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
		.help = "program data to OTP memory (required binary data) : program <offset> [ecc_brp_flags]",
		.arg_min = 1,
		.arg_max = 2,
		.arg_types = { ARG_UINT, ARG_UINT },
	},
	{
		.cmd_id = CMD_READ,
		.cmd = "read",
		.help = "read data from SPI flash: read <offset> <size> [ecc_brp_flags] [text|bin]",
		.arg_min = 2,
		.arg_max = 4,
		.arg_types = { ARG_UINT, ARG_UINT, ARG_UINT, ARG_STR },
	},
	{
		.cmd_id = CMD_READ_ECC,
		.cmd = "readecc",
		.help = "return ECC code for cell by offset: readecc <offset>",
		.arg_min = 1,
		.arg_max = 1,
		.arg_types = { ARG_UINT },
	},
	{
		.cmd_id = CMD_BIST,
		.cmd = "bist",
		.help = "return error if OTP memory is not clean: otp <offset> <size>",
		.arg_min = 2,
		.arg_max = 2,
		.arg_types = { ARG_UINT, ARG_UINT },
	},
	{
		.cmd_id = CMD_VERIFY,
		.cmd = "verify",
		.help = "read data from OTP cell by cell and compare with binary data: verify <offset>",
		.arg_min = 1,
		.arg_max = 1,
		.arg_types = { ARG_UINT },
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

static void print_hexdump_header(uint32_t offset)
{
	uart_puts(UART0, "_________");
	for (int i = 0; i < 16; i++)
		uart_printf(UART0, "_%02x", i);

	uart_putc(UART0, '\n');
	if (offset & 0xf) {
		uart_printf(UART0, "%08x:", offset & ~0xf);
		for (int i = 0; i < (offset & 0xf); i++)
			uart_puts(UART0, "   ");
	}
}

static void print_hexdump_body(uint8_t *buf, uint32_t offset, uint32_t len)
{
	char text[17] = "                ";
	uint32_t column;

	for (unsigned i = 0; i < len; i++) {
		column = (offset + i) & 0xf;
		if (column == 0)
			uart_printf(UART0, "%08x:", offset + i);
		uart_printf(UART0, " %02x", buf[i]);
		text[column] = (buf[i] >= 0x20 && buf[i] < 0x7f) ? buf[i] : '.';
		if (column == 0xf)
			uart_printf(UART0, "  %s\n", text);
	}
	column = (offset + len) & 0xf;
	text[column] = '\0';
	while (column & 0xf) {
		uart_puts(UART0, "   ");
		if (column == 0xf)
			uart_printf(UART0, "  %s\n", text);
		column++;
	}
}

static void iface_read_ecc(uint32_t offset)
{
	if (offset > OTP_SIZE || (offset & 0x3)) {
		uart_puts(UART0, "E\nWrong offset\n");
		return;
	}

	uart_printf(UART0, "%02X\n", OTP_readECC(offset));
}

static void iface_read(uint32_t offset, uint32_t size, uint32_t ecc_brp_flag, char *mode)
{
	int mode_int;
	uint8_t *p_buf8 = (uint8_t *)buffer;

	if ((size % sizeof(uint32_t)) != 0) {
		uart_puts(UART0, "E\nSize must be aligned by 32-bit word\n");
		return;
	}
	if ((offset % sizeof(uint32_t)) != 0) {
		uart_puts(UART0, "E\nOffset must be aligned by 32-bit word\n");
		return;
	}
	if ((ecc_brp_flag & ~ECC_BRP_Msk) != 0) {
		uart_puts(UART0, "E\nWrong ECC/BRP flags\n");
		return;
	}
	if ((offset + size) > OTP_SIZE) {
		uart_puts(UART0, "E\nSize is too large\n");
		return;
	}

	if ((mode[0] == 0) || !strcmp(mode, "text")) {
		mode_int = 0;
		print_hexdump_header(offset);
	} else if (!strcmp(mode, "bin")) {
		mode_int = 1;
		uart_putc(UART0, '#');
	} else {
		uart_puts(UART0, "Error: Unknown mode\n");
		return;
	}
	if (ecc_brp_flag != OTP_ECC_BRP_DEFAULT) {
		ecc_brp_flag &= ECC_BRP_Msk;
		OTP_setEccBrpFlag(ecc_brp_flag);
	}
	OTP_read(offset, buffer, size);
	if (mode_int == 0) {
		print_hexdump_body((uint8_t *)buffer, offset, size);
	} else {
		for (unsigned i = 0; i < size; i++)
			uart_putc_raw(UART0, p_buf8[i]); // Do not add \r to \n
	}
}

static void iface_bist(uint32_t offset, uint32_t size)
{
	if ((offset + size) > OTP_SIZE) {
		uart_puts(UART0, "E\nSize is too large\n");
		return;
	} else if ((size % sizeof(uint32_t)) != 0) {
		uart_puts(UART0, "E\nSize must be aligned by 32-bit word\n");
		return;
	} else if ((offset % sizeof(uint32_t)) != 0) {
		uart_puts(UART0, "E\nOffset must be aligned by 32-bit word\n");
		return;
	}
	OTP_initForBist();
	error = OTP_bist(address, size);
	if (error == OTP_OK) {
		uart_puts(UART0, "Ok\n");
	} else {
		uart_puts(UART0, "E\nError\n");
	}
}

void iface_verify(uint32_t offset)
{
	uint16_t block_size;
	uint16_t expected_crc;
	uint16_t crc;
	uint8_t *p_buf8 = (uint8_t *)buffer;

	while (1) {
		block_size = uart_getchar(UART0);
		block_size |= (uint16_t)uart_getchar(UART0) << 8;
		expected_crc = uart_getchar(UART0);
		expected_crc |= (uint16_t)uart_getchar(UART0) << 8;
		if (!block_size) {
			uart_putc(UART0, '\n');
			return;
		} else if ((offset + block_size) > OTP_SIZE) {
			uart_puts(UART0, "E\nSize is too large\n");
			return;
		} else if ((block_size % sizeof(uint32_t)) != 0) {
			uart_puts(UART0, "E\nSize must be aligned by 32-bit word\n");
			return;
		}
		crc = crc16_init();
		for (unsigned i = 0; i < block_size; i++) {
			p_buf8[i] = uart_getchar(UART0);
			crc = crc16_update_byte(crc, p_buf8[i]);
		}

		if (crc != expected_crc) {
			uart_putc(UART0, 'C');
			continue;
		}

		error = OTP_verify(address, buffer, block_size, 1);
		if (error == OTP_VERIFY_ERR) {
			uart_puts(UART0, "E\nError\n");
			continue;
		} else if (error == OTP_VERIFY_ECC_ERR) {
			uart_puts(UART0, "E\nErrorECC\n");
			continue;
		}

		offset += block_size;
		uart_putc(UART0, 'R');
	}
}

void iface_write_data(uint32_t offset, uint32_t ecc_brp_flag)
{
	uint16_t block_size;
	uint16_t expected_crc;
	uint16_t crc;
	uint8_t *p_buf8 = (uint8_t *)buffer;

	block_size = uart_getchar(UART0);
	block_size |= (uint16_t)uart_getchar(UART0) << 8;
	expected_crc = uart_getchar(UART0);
	expected_crc |= (uint16_t)uart_getchar(UART0) << 8;
	if (!block_size) {
		uart_puts(UART0, "E\nSize can not be zero\n");
		return;
	} else if ((offset + block_size) > OTP_SIZE) {
		uart_puts(UART0, "E\nSize is too large\n");
		return;
	} else if ((block_size % sizeof(uint32_t)) != 0) {
		uart_puts(UART0, "E\nSize must be aligned by 32-bit word\n");
		return;
	} else if ((ecc_brp_flag & ~ECC_BRP_Msk) != 0) {
		uart_puts(UART0, "E\nWrong ECC/BRP flags\n");
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

	OTP_initForProg();
	if (ecc_brp_flag != OTP_ECC_BRP_DEFAULT) {
		ecc_brp_flag &= ECC_BRP_Msk;
		OTP_setEccBrpFlag(ecc_brp_flag);
	}
	error = OTP_program(offset, buffer, block_size);
	if (error != OTP_OK) {
		uart_puts(UART0, "E\nOTP Program failed\n");
		return;
	}

	offset += block_size;
	uart_putc(UART0, 'R');
}

void console_run(struct console *console, struct console_cmd *cmd, struct console_arg *args,
		 int argc)
{
	switch (cmd->cmd_id) {
	case CMD_HELP:
		console_help(console);
		break;
	case CMD_PROGRAM:
		uart_puts(UART0, "Ready for data\n#");
		iface_write_data(args[0].uint, args[1].uint);
		break;
	case CMD_READ:
		iface_read(args[0].uint, args[1].uint, args[2].uint, args[3].str);
		break;
	case CMD_BIST:
		iface_bist(args[0].uint, args[1].uint);
		break;
	case CMD_VERIFY:
		uart_puts(UART0, "Ready for data\n#");
		iface_verify(args[0].uint);
		break;
	case CMD_READ_ECC:
		iface_read_ecc(args[0].uint);
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

	// Enable clock to HSPERIPH, LSPERIPH0 and LSPERIPH1
	REG(TOP_CLKGATE) |= BIT(4) | BIT(5) | BIT(6);

	REG(HSPERIPH_SUBS_PPOLICY) = PP_ON; // Enable HSPERIPH
	while ((REG(HSPERIPH_SUBS_PSTATUS) & 0x1f) != PP_ON)
		continue;

	REG(LSPERIPH0_SUBS_PPOLICY) = PP_ON; // Enable LSPERIPH0
	while ((REG(LSPERIPH0_SUBS_PSTATUS) & 0x1f) != PP_ON)
		continue;

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
	REG(LSP0_URB_PLL) = 0;
	REG(LSP1_URB_PLL) = 0;

	ucg_chan_set_div_and_enable(UCG_LSP1_UCG0, 6, 1, 1); // UART_CLK div=1 (27 MHz)

	// Setup UART0 pads
	REG(LSP1_URB_PAD_CTR(PORTB, 6)) = PAD_CTL_CTL(0x7) | PAD_CTL_SL(0x3);
	REG(LSP1_URB_PAD_CTR(PORTB, 7)) = PAD_CTL_E | PAD_CTL_CTL(0x7) | PAD_CTL_SL(0x3) |
					  PAD_CTL_SUS;
	// UART0 in hardware mode
	gpio_set_function_mask(GPIO1, GPIO_BANK_B, BIT(6) | BIT(7), GPIO_FUNC_PERIPH);

	uart_flush(UART0);
	uart_init(UART0, XTI_FREQUENCY, 115200);
	REG(SEVICE_URB_OTP_MODE) = 0;
	SBPI_getDefaultDwParams(&Params);
	SBPI_initMaster(SPI_OTP, &Params);
	uart_printf(UART0, "%s\n#", APP_NAME);

	while (1) {
		console_process(&console);
	}

	return 0;
}
