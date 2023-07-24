// SPDX-License-Identifier: MIT
// Copyright 2021 RnD Center "ELVEES", JSC

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <regs.h>
#include <uart.h>

#define SR1_BUSY 0x1

#define FLASH_READ 0x3
#define FLASH_READ4 0x13
#define FLASH_PROGRAM 0x2
#define FLASH_PROGRAM4 0x12
#define FLASH_ERASE 0xd8
#define FLASH_ERASE4 0xdc

#define APP_NAME "QSPI Flasher"

struct qspi {
	volatile uint32_t TX_DATA;
	volatile uint32_t RX_DATA;
	volatile uint32_t reserved08;
	volatile uint32_t CTRL;
	volatile uint32_t CTRL_AUX;
	volatile uint32_t STAT;
	volatile uint32_t SS;
	volatile uint32_t SS_POLARITY;
	volatile uint32_t INTR_EN;
	volatile uint32_t INTR_STAT;
	volatile uint32_t INTR_CLR;
	volatile uint32_t TX_FIFO_LVL;
	volatile uint32_t RX_FIFO_LVL;
	volatile uint32_t reserved34;
	volatile uint32_t MASTER_DELAY;
	volatile uint32_t ENABLE;
	volatile uint32_t GPO_SET;
	volatile uint32_t GPO_CLR;
	volatile uint32_t FIFO_DEPTH;
	volatile uint32_t FIFO_WMARK;
	volatile uint32_t TX_DUMMY;
};

enum cmd_ids {
	CMD_UNKNOWN = 0,
	CMD_HELP,
	CMD_QSPI,
	CMD_ERASE,
	CMD_WRITE,
	CMD_READ,
	CMD_READ_CRC,
	CMD_CUSTOM,
};

enum arg_types {
	ARG_STR,
	ARG_UINT,
};


struct qspi *qspi;
struct {
	char line[64];
	unsigned pos;
} cmd_line;
const struct {
	enum cmd_ids cmd_id;
	char *cmd;
	char *help;
	int arg_min;
	int arg_max;
	enum arg_types arg_types[5];
} commands[] = {
	{
		.cmd_id = CMD_HELP,
		.cmd = "help",
		.help = "show this message",
		.arg_min = 0,
		.arg_max = 0,
	},
	{
		.cmd_id = CMD_QSPI,
		.cmd = "qspi",
		.help = "select active QSPI controller and pad configuration: qspi <id> [v18]",
		.arg_min = 1,
		.arg_max = 2,
		.arg_types = { ARG_UINT, ARG_UINT },
	},
	{
		.cmd_id = CMD_ERASE,
		.cmd = "erase",
		.help = "erase sector: erase <offset_in_bytes>",
		.arg_min = 1,
		.arg_max = 1,
		.arg_types = { ARG_UINT },
	},
	{
		.cmd_id = CMD_WRITE,
		.cmd = "write",
		.help = "turn to write mode (requred binary data) : write <offset>",
		.arg_min = 1,
		.arg_max = 1,
		.arg_types = { ARG_UINT },
	},
	{
		.cmd_id = CMD_READ,
		.cmd = "read",
		.help = "read data from SPI flash: read <offset> <size> [text|bin]",
		.arg_min = 2,
		.arg_max = 3,
		.arg_types = { ARG_UINT, ARG_UINT, ARG_STR },
	},
	{
		.cmd_id = CMD_READ_CRC,
		.cmd = "readcrc",
		.help = "return CRC16 of data: readcrc <offset> <size>",
		.arg_min = 2,
		.arg_max = 2,
		.arg_types = { ARG_UINT, ARG_UINT },
	},
	{
		.cmd_id = CMD_CUSTOM,
		.cmd = "custom",
		.help = "send custom data and read answer: custom <tx_data> <rx_size>",
		.arg_min = 2,
		.arg_max = 2,
		.arg_types = { ARG_STR, ARG_UINT },
	},
};

unsigned long __stack_chk_guard;
void __stack_chk_fail(void)
{
}

int qspi_init(int id, int v18)
{
	if (id == 0)
		qspi = (struct qspi *)(TO_VIRT(QSPI0_BASE));
	else if (id == 1) {
		qspi = (struct qspi *)(TO_VIRT(QSPI1_BASE));
		if (v18)
			REG(HSP_URB_QSPI1_PADCFG) |= BIT(1);
		else
			REG(HSP_URB_QSPI1_PADCFG) &= ~BIT(1);
	} else
		return -1;

	qspi->CTRL = 0x25;
	qspi->CTRL_AUX = 0x700;
	qspi->SS = 0x1;
	qspi->ENABLE = 0x1;

	// Set high level to HOLD and WP pads
	qspi->GPO_SET = 0xc0c;
	qspi->GPO_CLR = 0x10c0;

	return 0;
}

static void qspi_stat_wait_mask(uint32_t mask, uint32_t value)
{
	while ((qspi->STAT & mask) != value) {
	}
}

void qspi_xfer(void *tx_buf, void *rx_buf, int len, bool is_last)
{
	uint8_t *tx = (uint8_t *)tx_buf;
	uint8_t *rx = (uint8_t *)rx_buf;
	int rx_count = 0;
	int tx_count = 0;

	qspi->CTRL_AUX |= 0x80;
	while (tx_count < len) {
		if (tx) {
			qspi->TX_DATA = tx[tx_count];
			tx_count++;
		} else {
			if (qspi->STAT & 0x4) {
				if ((len - tx_count) > 0xf0) {
					qspi->TX_DUMMY = 0xf0;
					tx_count += 0xf0;
				} else {
					qspi->TX_DUMMY = len - tx_count;
					tx_count += len - tx_count;
				}
			}
		}

		while ((qspi->STAT & 0x20) == 0) {
			if (rx)
				rx[rx_count] = qspi->RX_DATA & 0xff;
			else
				(void)qspi->RX_DATA;
			rx_count++;
		}
	}
	while (rx_count < len) {
		qspi_stat_wait_mask(0x20, 0);
		if (rx)
			rx[rx_count] = qspi->RX_DATA & 0xff;
		else
			(void)qspi->RX_DATA;
		rx_count++;
	}
	qspi_stat_wait_mask(0x1, 0);
	if (is_last)
		qspi->CTRL_AUX &= ~0x80;
}

/* Write command and address to the buffer.
 * buf - buffer for filling. Must be 5 bytes length.
 * cmd24 - command for 24-bit addressing.
 * cmd32 - command for 32-bit addressing.
 * addr - address of data in SPI Flash.
 * Return count of filled bytes (4 or 5).
 */
int qspi_flash_fill_cmd_addr(uint8_t *buf, uint8_t cmd24, uint8_t cmd32, uint32_t addr)
{
	int addr_bytes;

	if (addr >> 24) {
		buf[0] = cmd32;
		addr_bytes = 4;
	} else {
		buf[0] = cmd24;
		addr_bytes = 3;
	}

	for (int i = 1; i <= addr_bytes; i++)
		buf[i] = (addr >> ((addr_bytes - i) * 8)) & 0xff;

	return addr_bytes + 1;
}

void qspi_flash_read(void *buf, int len, uint32_t offset)
{
	uint8_t tmp_buf[5];
	int buf_len = qspi_flash_fill_cmd_addr(tmp_buf, FLASH_READ, FLASH_READ4, offset);

	qspi_xfer(tmp_buf, NULL, buf_len, false);
	qspi_xfer(NULL, buf, len, true);
}

void qspi_flash_write_enable(void)
{
	uint8_t data = 0x6;

	qspi_xfer(&data, NULL, 1, true);
}

void qspi_flash_write_disable(void)
{
	uint8_t data = 0x4;

	qspi_xfer(&data, NULL, 1, true);
}

uint8_t qspi_flash_read_status1(void)
{
	uint8_t cmd = 0x5;
	uint8_t res;

	qspi_xfer(&cmd, NULL, 1, false);
	qspi_xfer(NULL, &res, 1, true);

	return res;
}

void qspi_flash_erase(uint32_t offset)
{
	uint8_t data[5];
	int buf_len = qspi_flash_fill_cmd_addr(data, FLASH_ERASE, FLASH_ERASE4, offset);

	qspi_xfer(&data, NULL, buf_len, true);

	while (qspi_flash_read_status1() & SR1_BUSY) {
	}
}

void qspi_flash_write(void *buf, int len, uint32_t offset)
{
	uint8_t tmp_buf[5];
	int buf_len;
	uint8_t *ptr = (uint8_t *)buf;

	while (len) {
		int block_len = len < 256 ? len : 256;
		buf_len = qspi_flash_fill_cmd_addr(tmp_buf, FLASH_PROGRAM, FLASH_PROGRAM4, offset);

		len -= block_len;
		qspi_xfer(tmp_buf, NULL, buf_len, false);
		qspi_xfer(ptr, NULL, block_len, len == 0);
		ptr += block_len;
		offset += block_len;
		while (qspi_flash_read_status1() & SR1_BUSY) {
		}
	}
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

static uint16_t iface_read_crc(uint32_t offset, uint32_t size)
{
	uint8_t buf[1024];
	uint16_t crc = crc16_init();
	uint32_t len;

	while (size) {
		len = size > sizeof(buf) ? sizeof(buf) : size;
		qspi_flash_read(buf, len, offset);
		for (unsigned i = 0; i < len; i++)
			crc = crc16_update_byte(crc, buf[i]);

		size -= len;
		offset += len;
	}

	return crc;
}

static void iface_write_data(uint32_t offset)
{
	uint16_t block_size;
	uint16_t expected_crc;
	uint16_t crc;
	uint8_t buf[256];

	while (1) {
		block_size = uart_getchar();
		block_size |= (uint16_t)uart_getchar() << 8;
		expected_crc = uart_getchar();
		expected_crc |= (uint16_t)uart_getchar() << 8;
		if (!block_size) {
			uart_putc('\n');
			return;
		} else if (block_size > sizeof(buf)) {
			uart_puts("E\nBlock size is too large\n");
			return;
		}
		crc = crc16_init();
		for (unsigned i = 0; i < block_size; i++) {
			buf[i] = uart_getchar();
			crc = crc16_update_byte(crc, buf[i]);
		}

		if (crc != expected_crc) {
			uart_putc('C');
			continue;
		}
		qspi_flash_write_enable();
		qspi_flash_write(buf, block_size, offset);
		offset += block_size;
		uart_putc('R');
	}
}

static inline uint32_t hexchar2uint(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	else if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	else if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	else
		return (uint32_t)-1;
}

/* Parse hex string and fill array with uint8_t elements.
 * s - null-terminated string with long hex. Can be start from "0x" or not. Example: "abcdef123456"
 *     String length must be even.
 * bytes - array to fill
 * len - length of array
 * ok - will be write false if some error occur otherwise will be write true
 * Return count of filled bytes or 0xffffffff if data can not be fit into bytes
 * (also *ok will be false).
 */
static uint32_t hexstr2uint8array(char *s, uint8_t *bytes, uint32_t len, bool *ok)
{
	uint32_t idx = 0;
	uint32_t value;

	if (s[0] == '0' && s[1] == 'x')
		s += 2;

	while (*s) {
		if (idx >= len) {
			if (ok)
				*ok = false;
			return (uint32_t)-1;
		}
		value = hexchar2uint(*s) << 4;
		s++;
		value |= hexchar2uint(*s);
		if (value > 0xff) {
			if (ok)
				*ok = false;
			return idx;
		}
		s++;
		bytes[idx++] = value;
	}
	if (ok)
		*ok = true;

	return idx;
}

static void iface_custom(char *tx_str, uint32_t size)
{
	uint8_t buf[1024];
	uint32_t len;
	bool ok;

	len = hexstr2uint8array(tx_str, buf, sizeof(buf), &ok);
	if (!ok) {
		uart_puts("Error: ");
		if (len == (uint32_t)-1)
			uart_puts("data size is too big\n");
		else
			uart_puts("can not parse HEX in data\n");
		return;
	}
	qspi_xfer(buf, NULL, len, !size);
	while (size) {
		len = size > sizeof(buf) ? sizeof(buf) : size;
		size -= len;
		qspi_xfer(NULL, buf, len, !size);
		for (unsigned i = 0; i < len; i++)
			uart_printf("%02x ", buf[i]);
	}
	uart_putc('\n');
}

int strcmp(char *s1, char *s2)
{
	while (*s1 || *s2) {
		if (*s1 != *s2)
			return -1;
		s1++;
		s2++;
	}

	return 0;
}

static void iface_read(uint32_t offset, uint32_t size, char *mode)
{
	int mode_int;
	uint8_t buf[1024];
	char text[17];

	if (!mode || !strcmp(mode, "text")) {
		mode_int = 0;
		uart_puts("_________");
		for (int i = 0; i < 16; i++) {
			text[i] = ' ';
			uart_printf("_%02x", i);
		}
		text[16] = '\0';

		uart_putc('\n');
		if (offset & 0xf) {
			uart_printf("%08x:", offset & ~0xf);
			for (int i = 0; i < (offset & 0xf); i++)
				uart_puts("   ");
		}
	} else if (!strcmp(mode, "bin")) {
		mode_int = 1;
		uart_putc('#');
	} else {
		uart_puts("Error: Unknown mode\n");
		return;
	}
	while (size) {
		uint32_t len = size > sizeof(buf) ? sizeof(buf) : size;
		qspi_flash_read(buf, len, offset);
		if (mode_int == 0) {
			for (unsigned i = 0; i < len; i++) {
				uint32_t column = (offset + i) & 0xf;
				if (column == 0)
					uart_printf("%08x:", offset + i);
				uart_printf(" %02x", buf[i]);
				text[column] = (buf[i] >= 0x20 && buf[i] < 0x7f) ? buf[i] : '.';
				if (column == 0xf)
					uart_printf("  %s\n", text);
			}
		} else {
			for (unsigned i = 0; i < len; i++)
				uart_putc_raw(buf[i]);  // Do not add \r to \n
		}
		size -= len;
		offset += len;
	}
	if (mode_int == 0) {
		while (offset & 0xf) {
			uint32_t column = offset & 0xf;
			uart_puts("   ");
			text[column] = ' ';
			if (column == 0xf)
				uart_printf("  %s\n", text);
			offset++;
		}
	}
}

static uint32_t str2uint(char *s, bool *ok)
{
	uint32_t value = 0;
	uint32_t val_char;

	if (s[0] == '0' && s[1] == 'x') {
		s = s + 2;
		while (*s) {
			value <<= 4;
			val_char = hexchar2uint(*s);
			if (val_char == (uint32_t)-1) {
				if (ok)
					*ok = false;
				return 0;
			}
			value |= val_char;
			s++;
		}
	} else {
		while (*s) {
			value *= 10;
			if (*s >= '0' && *s <= '9')
				value += *s - '0';
			else {
				if (ok)
					*ok = false;
				return 0;
			}
			s++;
		}
	}
	if (ok)
		*ok = true;

	return value;
}

void iface_execute(char *cmd, char *args[], int argc)
{
	enum cmd_ids cmd_id = CMD_UNKNOWN;
	struct {
		char *str;
		uint32_t uint;
	} arguments[5] = {0};

	for (int i = 0; i < ARRAY_LENGTH(commands); i++) {
		if (!strcmp(commands[i].cmd, cmd)) {
			cmd_id = commands[i].cmd_id;
			if (argc < commands[i].arg_min || argc > commands[i].arg_max) {
				uart_puts("Error: Wrong arguments count\n");
				return;
			}
			for (int j = 0; j < argc; j++) {
				bool ok;
				arguments[j].str = args[j];
				if (commands[i].arg_types[j] == ARG_UINT) {
					arguments[j].uint = str2uint(args[j], &ok);
					if (!ok) {
						uart_printf("Error: Argument %d must be integer\n",
							    j);
						return;
					}
				} else
					arguments[j].uint = 0;
			}
		}
	}
	if (cmd_id == CMD_UNKNOWN) {
		uart_puts("Error: Unknown command\n");
		return;
	}

	switch (cmd_id) {
	case CMD_HELP:
		for (int i = 0; i < ARRAY_LENGTH(commands); i++)
			uart_printf("%s    - %s\n", commands[i].cmd, commands[i].help);
		break;
	case CMD_QSPI:
		if (qspi_init(arguments[0].uint, arguments[1].uint))
			uart_puts("Error: Init error\n");
		else
			uart_printf("Selected QSPI%u, padcfg v18 = %u\n", arguments[0].uint,
				    !!arguments[1].uint);
		break;
	case CMD_ERASE:
		qspi_flash_write_enable();
		qspi_flash_erase(arguments[0].uint);
		uart_puts("OK\n");
		break;
	case CMD_WRITE:
		uart_puts("Ready for data\n#");
		iface_write_data(arguments[0].uint);
		break;
	case CMD_READ:
		iface_read(arguments[0].uint, arguments[1].uint, arguments[2].str);
		break;
	case CMD_READ_CRC:
		uart_printf("%#x\n", iface_read_crc(arguments[0].uint, arguments[1].uint));
		break;
	case CMD_CUSTOM:
		iface_custom(arguments[0].str, arguments[1].uint);
		break;
	default:
		break;
	}
}

void iface_parse_cmd_line(void)
{
	char *args[5];
	int argc = 0;
	bool last_space = false;

	for (int i = 0; i < cmd_line.pos; i++) {
		if (cmd_line.line[i] == ' ') {
			if (!last_space)
				cmd_line.line[i] = '\0';
			last_space = true;
		} else {
			if (last_space) {
				if (argc >= 5) {
					uart_puts("Error: Too many arguments\n");
					return;
				}
				args[argc++] = &cmd_line.line[i];
				last_space = false;
			}
		}
	}
	if (cmd_line.line[0] == '\0') {
		uart_puts(APP_NAME);
		uart_putc('\n');
		return;
	}

	iface_execute(cmd_line.line, args, argc);
}

void iface_process(void)
{
	uint8_t ch;

	if (!uart_is_char_ready())
		return;

	ch = uart_getchar();
	if (ch == 0x1b)  // Ignore Esc key and esc-sequences
		return;
	if (ch == 0x7f) {  // Backspace
		if (cmd_line.pos) {
			cmd_line.line[--cmd_line.pos] = '\0';
			uart_puts("\x1b[1D \x1b[1D");
		}
		return;
	}
	if (cmd_line.pos >= sizeof(cmd_line.line) - 2) {
		cmd_line.pos = 0;
		cmd_line.line[0] = '\0';
		uart_puts("\nError: commmand is too large\n#");
		return;
	}

	if (ch == '\n')
		return;

	if (ch == '\r') {
		uart_putc('\n');
		iface_parse_cmd_line();
		uart_putc('#');
		cmd_line.pos = 0;
		cmd_line.line[0] = '\0';
		return;
	}

	uart_putc(ch);  // Echo
	cmd_line.line[cmd_line.pos++] = ch;
	cmd_line.line[cmd_line.pos] = '\0';
}

static void delay_loop(void)
{
	for (volatile int i = 0; i < 10000; i++) {
	}
}

int main(void)
{
	ucg_regs_t *ucg;

	REG(TOP_CLKGATE) |= BIT(4) | BIT(6);  // Enable clock to HSPERIPH and LSPERIPH1

	REG(LSPERIPH1_SUBS_PPOLICY) = PP_ON;  // Enable LSPERIPH1
	while ((REG(LSPERIPH1_SUBS_PSTATUS) & 0x1f) != PP_ON) {
	}

	REG(HSPERIPH_SUBS_PPOLICY) = PP_ON;  // Enable HSPERIPH
	while ((REG(HSPERIPH_SUBS_PSTATUS) & 0x1f) != PP_ON) {
	}

	if ((REG(SERVICE_URB_PLL) & 0xff) != 7) {
		// Setup SERVICE clocks as in UART boot mode
		ucg = (ucg_regs_t *)(TO_VIRT(SERVICE_UCG));
		ucg->BP_CTR_REG = 0xffff;
		delay_loop();
		REG(SERVICE_URB_PLL) = 7;  // SERVICE PLL to 216 MHz
		delay_loop();
		ucg->CTR_REG[0] = 0x2302;   // CLK_APB div=8 (27 MHz)
		ucg->CTR_REG[1] = 0x702;    // CLK_CORE div=1 (216 MHz)
		ucg->CTR_REG[2] = 0x702;    // CLK_QSPI0 div=1 (216 MHz)
		ucg->CTR_REG[13] = 0x4302;  // CLK_QSPI0_EXT div=16 (13.5 MHz)
		delay_loop();
		ucg->BP_CTR_REG = 0;
	}

	// Turn PLLs to bypass mode
	REG(HSP_URB_PLL) = 0;
	REG(LSP1_URB_PLL) = 0;

	REG(HSP_URB_RST) = BIT(2) | BIT(18);  // Assert reset for QSPI1

	ucg = (ucg_regs_t *)(TO_VIRT(HSP_UCG(0)));
	ucg->CTR_REG[12] = 0xb02;

	ucg = (ucg_regs_t *)(TO_VIRT(HSP_UCG(1)));
	ucg->CTR_REG[3] = 0x702;

	REG(HSP_URB_QSPI1_PADCFG) = 0x1;
	REG(HSP_URB_QSPI1_SS_PADCFG) = 0x1fa;
	REG(HSP_URB_QSPI1_SISO_PADCFG) = 0x1fa;
	REG(HSP_URB_QSPI1_SCLK_PADCFG) = 0x1fa;

	REG(HSP_URB_RST) = BIT(18);  // Deassert reset for QSPI1

	REG(XIP_EN_REQ) = 0;  // Out QSPI0 from XIP mode
	REG(HSP_URB_XIP_EN_REQ) = 0;  // Out QSPI1 from XIP mode
	while (REG(XIP_EN_OUT) || REG(HSP_URB_XIP_EN_OUT)) {
	}

	REG(LSP1_UCG_CTRL6) = 0x2;  // UART_CLK CLK_EN

	// Setup UART0 pads
	REG(LSP1_URB_PAD_CTR(PORTB, 6)) = PAD_CTL_CTL(0x7) | PAD_CTL_SL(0x3);
	REG(LSP1_URB_PAD_CTR(PORTB, 7)) = PAD_CTL_E | PAD_CTL_CTL(0x7) |
					  PAD_CTL_SL(0x3) | PAD_CTL_SUS;
	REG(GPIO1_SWPORTB_CTL) |= 0xc0;  // UART0 in hardware mode

	uart_flush();
	uart_init(XTI_FREQUENCY, 115200);
	qspi_init(0, 1);
	uart_printf("%s\n#", APP_NAME);
	while (1) {
		iface_process();
	}

	return 0;
}
