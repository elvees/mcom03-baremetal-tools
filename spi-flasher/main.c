/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <regs.h>
#include <uart.h>

#define SR1_BUSY 0x1

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
		.help = "select active QSPI controller: qspi <id>",
		.arg_min = 1,
		.arg_max = 1,
		.arg_types = { ARG_UINT },
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
		.help = "send custom command byte and read answer: custom <cmd> <size>",
		.arg_min = 2,
		.arg_max = 2,
		.arg_types = { ARG_UINT, ARG_UINT },
	},
};


int qspi_init(int id)
{
	if (id == 0)
		qspi = (struct qspi *)(TO_VIRT(QSPI0_BASE));
	else if (id == 1)
		qspi = (struct qspi *)(TO_VIRT(QSPI1_BASE));
	else
		return -1;

	qspi->CTRL = 0x25;
	qspi->CTRL_AUX = 0x700;
	qspi->SS = 0x1;
	qspi->ENABLE = 0x1;

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

void qspi_flash_read(void *buf, int len, uint32_t offset)
{
	uint8_t tmp_buf[4] = { 0x3, (offset >> 16) & 0xff, (offset >> 8) & 0xff, offset & 0xff };

	qspi_xfer(tmp_buf, NULL, 4, false);
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
	uint8_t data[4] = { 0xd8, (offset >> 16) & 0xff, (offset >> 8) & 0xff, offset & 0xff };

	qspi_xfer(&data, NULL, 4, true);

	while (qspi_flash_read_status1() & SR1_BUSY) {
	}
}

void qspi_flash_write(void *buf, int len, uint32_t offset)
{
	uint8_t tmp_buf[4] = { 0x2, (offset >> 16) & 0xff, (offset >> 8) & 0xff, offset & 0xff };
	uint8_t *ptr = (uint8_t *)buf;

	while (len) {
		int block_len = len < 256 ? len : 256;

		len -= block_len;
		qspi_xfer(tmp_buf, NULL, 4, false);
		qspi_xfer(ptr, NULL, block_len, len == 0);
		ptr += block_len;
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

static void iface_custom(uint32_t cmd, uint32_t size)
{
	uint8_t buf[1024];
	uint32_t len;

	if (cmd > 0xff) {
		uart_puts("Error: Command must be in range [0 .. 0xff]\n");
		return;
	}
	buf[0] = cmd & 0xff;
	qspi_xfer(buf, NULL, 1, false);
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
	char buf[1024];
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

uint32_t str2uint(char *s, bool *ok)
{
	uint32_t value = 0;

	if (s[0] == '0' && s[1] == 'x') {
		s = s + 2;
		while (*s) {
			value <<= 4;
			if (*s >= '0' && *s <= '9')
				value |= *s - '0';
			else if (*s >= 'a' && *s <= 'f')
				value |= *s - 'a' + 10;
			else if (*s >= 'A' && *s <= 'F')
				value |= *s - 'A' + 10;
			else {
				if (ok)
					*ok = false;
				return 0;
			}
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
		if (qspi_init(arguments[0].uint))
			uart_puts("Error: Init error\n");
		else
			uart_printf("Selected QSPI%u\n", arguments[0].uint);
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
		iface_custom(arguments[0].uint, arguments[1].uint);
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

	REG(HSP_URB_RST) = BIT(2) | BIT(18);  // Assert reset for QSPI1

	ucg = (ucg_regs_t *)(TO_VIRT(HSP_UCG(0)));
	ucg->CTR_REG[12] = 0xb02;

	ucg = (ucg_regs_t *)(TO_VIRT(HSP_UCG(1)));
	ucg->CTR_REG[3] = 0x702;

	REG(HSP_URB_QSPI1_PADCFG) = 0x7;
	REG(HSP_URB_QSPI1_SS_PADCFG) = 0x1fa;
	REG(HSP_URB_QSPI1_SISO_PADCFG) = 0x1fa;
	REG(HSP_URB_QSPI1_SCLK_PADCFG) = 0x1fa;

	REG(HSP_URB_RST) = BIT(18);  // Deassert reset for QSPI1

	REG(XIP_EN_REQ) = 0;  // Out QSPI0 from XIP mode
	REG(HSP_URB_XIP_EN_REQ) = 0;  // Out QSPI1 from XIP mode
	while (REG(XIP_EN_OUT) || REG(HSP_URB_XIP_EN_OUT)) {
	}

	REG(LSP1_UCG_CTRL6) = 0x2;  // UART_CLK CLK_EN
	REG(GPIO1_SWPORTB_CTL) |= 0xc0;  // UART0 in hardware mode

	uart_init(XTI_FREQUENCY, 115200);
	qspi_init(0);
	uart_printf("%s\n#", APP_NAME);
	while (1) {
		iface_process();
	}

	return 0;
}
