// SPDX-License-Identifier: MIT
// Copyright 2021-2024 RnD Center "ELVEES", JSC

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <delay.h>
#include <gpio.h>
#include <i2c.h>
#include <qspi.h>
#include <regs.h>
#include <uart.h>

/*
 * Turn A into a string literal without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded).
 */
#define STRINGIZE_NX(A) #A

/* Turn A into a string literal after macro-expanding it. */
#define STRINGIZE(A) STRINGIZE_NX(A)

#define SR1_BUSY 0x1

#define FLASH_READ     0x3
#define FLASH_READ4    0x13
#define FLASH_PROGRAM  0x2
#define FLASH_PROGRAM4 0x12
#define FLASH_ERASE    0xd8
#define FLASH_ERASE4   0xdc

#define APP_NAME \
	"QSPI Flasher (commit: '" STRINGIZE(GIT_SHA1_SHORT) "', build: '" STRINGIZE(BUILD_ID) "')"

#define I2C_BUFFER_SIZE 256

enum cmd_ids {
	CMD_UNKNOWN = 0,
	CMD_HELP,
	CMD_QSPI,
	CMD_ERASE,
	CMD_WRITE,
	CMD_READ,
	CMD_READ_CRC,
	CMD_CUSTOM,
	CMD_BOOTROM,
	CMD_EXIT,
	CMD_I2C_DEV,
	CMD_I2C_READ,
	CMD_I2C_WRITE,
};

enum arg_types {
	ARG_STR,
	ARG_UINT,
};

bool need_exit;
struct qspi *qspi;
struct i2c *i2c;

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
		.help = "turn to write mode (requred binary data) : write <offset> <page_size>",
		.arg_min = 2,
		.arg_max = 2,
		.arg_types = { ARG_UINT, ARG_UINT },
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
#ifdef MIPS32
	{
		.cmd_id = CMD_BOOTROM,
		.cmd = "bootrom",
		.help = "Restart BootROM",
		.arg_min = 0,
		.arg_max = 0,
	},
#endif
#ifdef CAN_RETURN
	{
		.cmd_id = CMD_EXIT,
		.cmd = "exit",
		.help = "Exit to U-Boot",
		.arg_min = 0,
		.arg_max = 0,
	},
#endif
	{
		.cmd_id = CMD_I2C_DEV,
		.cmd = "i2c_dev",
		.help = "Select and prepare I2C controller: "
			"i2c_dev <ctrl_id> <speed> (0 - std, 1 - fast)",
		.arg_min = 2,
		.arg_max = 2,
		.arg_types = { ARG_UINT, ARG_UINT },
	},
	{
		.cmd_id = CMD_I2C_READ,
		.cmd = "i2c_read",
		.help = "Read data from i2c device: i2c_read <addr> <regaddr> <alen> <size> [text|bin]",
		.arg_min = 4,
		.arg_max = 5,
		.arg_types = { ARG_UINT, ARG_UINT, ARG_UINT, ARG_UINT, ARG_STR },
	},
	{
		.cmd_id = CMD_I2C_WRITE,
		.cmd = "i2c_write",
		.help = "Write data to i2c device: i2c_write <addr> <regaddr> <alen> <size>",
		.arg_min = 4,
		.arg_max = 4,
		.arg_types = { ARG_UINT, ARG_UINT, ARG_UINT, ARG_UINT },
	},
};

unsigned long __stack_chk_guard;
void __stack_chk_fail(void)
{
}

void run_bootrom(void)
{
	void (*bootrom)(void) = (void *)0x9fc00000;
	ucg_regs_t *ucg;

	uart_printf(UART0, "Restore default state and restart BootROM...\n");
	uart_flush(UART0);
	ucg = (ucg_regs_t *)(TO_VIRT(SERVICE_UCG));
	ucg->BP_CTR_REG = 0xffff;
	set_tick_freq(XTI_FREQUENCY);
	udelay(1);
	REG(SERVICE_URB_PLL) = 0; // SERVICE PLL to 27 MHz
	udelay(1);
	ucg->CTR_REG[0] = 0x402; // CLK_APB to 27 MHz
	ucg->CTR_REG[1] = 0x402; // CLK_CORE to 27 MHz
	ucg->CTR_REG[2] = 0x402; // CLK_QSPI0 to 27 MHz
	ucg->CTR_REG[13] = 0x402; // CLK_QSPI0_EXT to 27 MHz
	udelay(1);
	ucg->BP_CTR_REG = 0;
	ucg = (ucg_regs_t *)(TO_VIRT(HSP_UCG(0)));
	ucg->CTR_REG[12] = 0x400; // CLK_QSPI1 to 27 MHz and disable
	ucg = (ucg_regs_t *)(TO_VIRT(HSP_UCG(1)));
	ucg->CTR_REG[3] = 0x400; // CLK_QSPI1_EXT to 27 MHz and disable
	REG(XIP_EN_REQ) = 1; // QSPI0 to XIP mode
	REG(HSP_URB_XIP_EN_REQ) = 1; // QSPI1 from XIP mode
	bootrom(); // BootROM will reconfigure all registers and never return to SPI Flasher
}

int qspi_prepare(int id, int v18)
{
	if (id == 0)
		qspi = QSPI0;
	else if (id == 1) {
		qspi = QSPI1;
		if (v18)
			REG(HSP_URB_QSPI1_PADCFG) |= BIT(1);
		else
			REG(HSP_URB_QSPI1_PADCFG) &= ~BIT(1);
	} else
		return -1;

	qspi_init(qspi, 0);

	return 0;
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

	qspi_xfer(qspi, tmp_buf, NULL, buf_len, false);
	qspi_xfer(qspi, NULL, buf, len, true);
}

void qspi_flash_write_enable(void)
{
	uint8_t data = 0x6;

	qspi_xfer(qspi, &data, NULL, 1, true);
}

void qspi_flash_write_disable(void)
{
	uint8_t data = 0x4;

	qspi_xfer(qspi, &data, NULL, 1, true);
}

uint8_t qspi_flash_read_status1(void)
{
	uint8_t cmd = 0x5;
	uint8_t res;

	qspi_xfer(qspi, &cmd, NULL, 1, false);
	qspi_xfer(qspi, NULL, &res, 1, true);

	return res;
}

void qspi_flash_erase(uint32_t offset)
{
	uint8_t data[5];
	int buf_len = qspi_flash_fill_cmd_addr(data, FLASH_ERASE, FLASH_ERASE4, offset);

	qspi_xfer(qspi, &data, NULL, buf_len, true);

	while (qspi_flash_read_status1() & SR1_BUSY) {
	}
}

void qspi_flash_write_page(void *buf, uint32_t len, uint32_t offset)
{
	uint8_t tmp_buf[5];
	int buf_len;

	buf_len = qspi_flash_fill_cmd_addr(tmp_buf, FLASH_PROGRAM, FLASH_PROGRAM4, offset);
	qspi_xfer(qspi, tmp_buf, NULL, buf_len, false);
	qspi_xfer(qspi, buf, NULL, len, true);
	while (qspi_flash_read_status1() & SR1_BUSY) {
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

void iface_write_data(uint32_t offset, uint32_t size)
{
	uint16_t block_size;
	uint16_t expected_crc;
	uint16_t crc;
	uint8_t buf[size];

	while (1) {
		block_size = uart_getchar(UART0);
		block_size |= (uint16_t)uart_getchar(UART0) << 8;
		expected_crc = uart_getchar(UART0);
		expected_crc |= (uint16_t)uart_getchar(UART0) << 8;
		if (!block_size) {
			uart_putc(UART0, '\n');
			return;
		} else if (block_size > sizeof(buf)) {
			uart_puts(UART0, "E\nBlock size is too large\n");
			return;
		}
		crc = crc16_init();
		for (unsigned i = 0; i < block_size; i++) {
			buf[i] = uart_getchar(UART0);
			crc = crc16_update_byte(crc, buf[i]);
		}

		if (crc != expected_crc) {
			uart_putc(UART0, 'C');
			continue;
		}
		qspi_flash_write_enable();
		qspi_flash_write_page(buf, block_size, offset);
		offset += block_size;
		uart_putc(UART0, 'R');
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
		uart_puts(UART0, "Error: ");
		if (len == (uint32_t)-1)
			uart_puts(UART0, "data size is too big\n");
		else
			uart_puts(UART0, "can not parse HEX in data\n");
		return;
	}
	qspi_xfer(qspi, buf, NULL, len, !size);
	while (size) {
		len = size > sizeof(buf) ? sizeof(buf) : size;
		size -= len;
		qspi_xfer(qspi, NULL, buf, len, !size);
		for (unsigned i = 0; i < len; i++)
			uart_printf(UART0, "%02x ", buf[i]);
	}
	uart_putc(UART0, '\n');
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

static void iface_read(uint32_t offset, uint32_t size, char *mode)
{
	int mode_int;
	uint8_t buf[1024];

	if (!mode || !strcmp(mode, "text")) {
		mode_int = 0;
		print_hexdump_header(offset);
	} else if (!strcmp(mode, "bin")) {
		mode_int = 1;
		uart_putc(UART0, '#');
	} else {
		uart_puts(UART0, "Error: Unknown mode\n");
		return;
	}
	while (size) {
		uint32_t len = size > sizeof(buf) ? sizeof(buf) : size;
		qspi_flash_read(buf, len, offset);
		if (mode_int == 0) {
			print_hexdump_body(buf, offset, len);
		} else {
			for (unsigned i = 0; i < len; i++)
				uart_putc_raw(UART0, buf[i]); // Do not add \r to \n
		}
		size -= len;
		offset += len;
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

void cmd_i2c_dev(uint32_t ctrl_id, uint32_t speed)
{
	switch (ctrl_id) {
	case 0:
		i2c = I2C0;
		break;
	case 1:
		i2c = I2C1;
		break;
	case 2:
		i2c = I2C2;
		break;
	case 3:
		i2c = I2C3;
		break;
	case 4:
		i2c = I2C4;
		break;
	default:
		uart_printf(UART0, "Error: wrong ctrl_id %d\n", ctrl_id);
		return;
	}

	if (speed == 0)
		speed = I2C_STANDARD_SPEED;
	else if (speed == 1)
		speed = I2C_FAST_SPEED;

	i2c_init(i2c, speed, XTI_FREQUENCY);
	i2c_pads_cfg(ctrl_id);
}

void cmd_i2c_read(uint32_t addr, uint32_t regaddr, uint32_t alen, uint32_t size, char *mode)
{
	uint8_t buf[I2C_BUFFER_SIZE];

	if (size >= I2C_BUFFER_SIZE)
		size = I2C_BUFFER_SIZE;

	if (!i2c_read(i2c, addr, regaddr, alen, buf, size)) {
		uart_puts(UART0, "Error\n");
		return;
	}

	if (!mode || !strcmp(mode, "text")) {
		print_hexdump_header(regaddr);
		print_hexdump_body(buf, regaddr, size);
	} else if (!strcmp(mode, "bin")) {
		uart_putc(UART0, '#');
		for (uint32_t i = 0; i < size; i++)
			uart_putc_raw(UART0, buf[i]); // Do not add \r to \n
	}
}

void cmd_i2c_write(uint32_t addr, uint32_t regaddr, uint32_t alen, uint32_t size)
{
	uint8_t buf[I2C_BUFFER_SIZE];

	uart_puts(UART0, "Ready for data\n#");
	if (size > I2C_BUFFER_SIZE) {
		uart_puts(UART0, "Error: Structure size is too large\n");
		return;
	}

	if (size >= I2C_BUFFER_SIZE)
		size = I2C_BUFFER_SIZE;

	for (uint32_t i = 0; i < size; i++)
		buf[i] = uart_getchar(UART0);

	if (!i2c_write(i2c, addr, regaddr, alen, buf, size)) {
		uart_puts(UART0, "Error\n");
		return;
	}

	uart_puts(UART0, "Done\n");
}

void iface_execute(char *cmd, char *args[], int argc)
{
	enum cmd_ids cmd_id = CMD_UNKNOWN;
	struct {
		char *str;
		uint32_t uint;
	} arguments[5] = { 0 };

	for (int i = 0; i < ARRAY_LENGTH(commands); i++) {
		if (!strcmp(commands[i].cmd, cmd)) {
			cmd_id = commands[i].cmd_id;
			if (argc < commands[i].arg_min || argc > commands[i].arg_max) {
				uart_puts(UART0, "Error: Wrong arguments count\n");
				return;
			}
			for (int j = 0; j < argc; j++) {
				bool ok;
				arguments[j].str = args[j];
				if (commands[i].arg_types[j] == ARG_UINT) {
					arguments[j].uint = str2uint(args[j], &ok);
					if (!ok) {
						uart_printf(UART0,
							    "Error: Argument %d must be integer\n",
							    j);
						return;
					}
				} else
					arguments[j].uint = 0;
			}
		}
	}
	if (cmd_id == CMD_UNKNOWN) {
		uart_puts(UART0, "Error: Unknown command\n");
		return;
	}

	switch (cmd_id) {
	case CMD_HELP:
		for (int i = 0; i < ARRAY_LENGTH(commands); i++)
			uart_printf(UART0, "%s    - %s\n", commands[i].cmd, commands[i].help);
		break;
	case CMD_QSPI:
		if (qspi_prepare(arguments[0].uint, arguments[1].uint))
			uart_puts(UART0, "Error: Init error\n");
		else
			uart_printf(UART0, "Selected QSPI%u, padcfg v18 = %u\n", arguments[0].uint,
				    !!arguments[1].uint);
		break;
	case CMD_ERASE:
		qspi_flash_write_enable();
		qspi_flash_erase(arguments[0].uint);
		uart_puts(UART0, "OK\n");
		break;
	case CMD_WRITE:
		if (arguments[1].uint == 0 || arguments[1].uint > 32 * 1024) {
			uart_puts(UART0, "E\nWrong page size. Must be 0 < page <= 32768\n");
			break;
		}
		uart_puts(UART0, "Ready for data\n#");
		iface_write_data(arguments[0].uint, arguments[1].uint);
		break;
	case CMD_READ:
		iface_read(arguments[0].uint, arguments[1].uint, arguments[2].str);
		break;
	case CMD_READ_CRC:
		uart_printf(UART0, "%#x\n", iface_read_crc(arguments[0].uint, arguments[1].uint));
		break;
	case CMD_CUSTOM:
		iface_custom(arguments[0].str, arguments[1].uint);
		break;
#ifdef MIPS32
	case CMD_BOOTROM:
		run_bootrom();
		break;
#endif
#ifdef CAN_RETURN
	case CMD_EXIT:
		need_exit = true;
		break;
#endif
	case CMD_I2C_DEV:
		cmd_i2c_dev(arguments[0].uint, arguments[1].uint);
		break;
	case CMD_I2C_READ:
		cmd_i2c_read(arguments[0].uint, arguments[1].uint, arguments[2].uint,
			     arguments[3].uint, arguments[4].str);
		break;
	case CMD_I2C_WRITE:
		cmd_i2c_write(arguments[0].uint, arguments[1].uint, arguments[2].uint,
			      arguments[3].uint);
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
					uart_puts(UART0, "Error: Too many arguments\n");
					return;
				}
				args[argc++] = &cmd_line.line[i];
				last_space = false;
			}
		}
	}
	if (cmd_line.line[0] == '\0') {
		uart_puts(UART0, APP_NAME);
		uart_putc(UART0, '\n');
		return;
	}

	iface_execute(cmd_line.line, args, argc);
}

void iface_process(void)
{
	uint8_t ch;

	if (!uart_is_char_ready(UART0))
		return;

	ch = uart_getchar(UART0);
	if (ch == 0x1b) // Ignore Esc key and esc-sequences
		return;
	if (ch == 0x7f) { // Backspace
		if (cmd_line.pos) {
			cmd_line.line[--cmd_line.pos] = '\0';
			uart_puts(UART0, "\x1b[1D \x1b[1D");
		}
		return;
	}
	if (cmd_line.pos >= sizeof(cmd_line.line) - 2) {
		cmd_line.pos = 0;
		cmd_line.line[0] = '\0';
		uart_puts(UART0, "\nError: commmand is too large\n#");
		return;
	}

	if (ch == '\n')
		return;

	if (ch == '\r') {
		uart_putc(UART0, '\n');
		iface_parse_cmd_line();
		uart_putc(UART0, '#');
		cmd_line.pos = 0;
		cmd_line.line[0] = '\0';
		return;
	}

	uart_putc(UART0, ch); // Echo
	cmd_line.line[cmd_line.pos++] = ch;
	cmd_line.line[cmd_line.pos] = '\0';
}

#ifdef CAN_RETURN
struct clock_settings {
	uint32_t service_pll;
	uint32_t service_ucg_apb;
	uint32_t service_ucg_core;
	uint32_t service_ucg_qspi0;
	uint32_t service_ucg_qspi0_ext;
	uint32_t hsp_pll;
	uint32_t hsp_ucg_qspi1;
	uint32_t hsp_ucg_qspi1_ext;
	bool is_service_changed;
};

static void save_clock_settings(struct clock_settings *clock_settings)
{
	ucg_regs_t *ucg;

	clock_settings->hsp_pll = REG(HSP_URB_PLL);
	ucg = (ucg_regs_t *)(TO_VIRT(SERVICE_UCG));
	clock_settings->service_pll = REG(SERVICE_URB_PLL);
	if ((clock_settings->service_pll & 0xff) != 7)
		clock_settings->is_service_changed = true;

	clock_settings->service_ucg_apb = ucg->CTR_REG[0];
	clock_settings->service_ucg_core = ucg->CTR_REG[1];
	clock_settings->service_ucg_qspi0 = ucg->CTR_REG[2];
	clock_settings->service_ucg_qspi0_ext = ucg->CTR_REG[13];

	ucg = (ucg_regs_t *)(TO_VIRT(HSP_UCG(0)));
	clock_settings->hsp_ucg_qspi1 = ucg->CTR_REG[12];
	ucg = (ucg_regs_t *)(TO_VIRT(HSP_UCG(1)));
	clock_settings->hsp_ucg_qspi1_ext = ucg->CTR_REG[3];
}

static void restore_clock_settings(struct clock_settings *clock_settings)
{
	ucg_regs_t *ucg;

	if (clock_settings->is_service_changed) {
		ucg = (ucg_regs_t *)(TO_VIRT(SERVICE_UCG));
		ucg->BP_CTR_REG = 0xffff;
		/* set_tick_freq() is not required here because this code is under CAN_RETURN ifdef.
		 * CAN_RETURN used only for ARM processor that ignores set_tick_freq() (for ARM
		 * fixed timer frequency is used) */
		udelay(1);
		REG(SERVICE_URB_PLL) = clock_settings->service_pll;
		udelay(1);
		ucg->CTR_REG[0] = clock_settings->service_ucg_apb;
		ucg->CTR_REG[1] = clock_settings->service_ucg_core;
		ucg->CTR_REG[2] = clock_settings->service_ucg_qspi0;
		ucg->CTR_REG[13] = clock_settings->service_ucg_qspi0_ext;
		udelay(1);
		ucg->BP_CTR_REG = 0;
	}
	ucg = (ucg_regs_t *)(TO_VIRT(HSP_UCG(0)));
	ucg->BP_CTR_REG |= BIT(12);
	ucg->CTR_REG[12] = clock_settings->hsp_ucg_qspi1;
	udelay(1);
	ucg->BP_CTR_REG &= ~BIT(12);

	ucg = (ucg_regs_t *)(TO_VIRT(HSP_UCG(1)));
	ucg->BP_CTR_REG |= BIT(3);
	ucg->CTR_REG[3] = clock_settings->hsp_ucg_qspi1_ext;
	udelay(1);
	ucg->BP_CTR_REG &= ~BIT(3);

	REG(HSP_URB_PLL) = clock_settings->hsp_pll;
}
#else
struct clock_settings {};

static void save_clock_settings(struct clock_settings *clock_settings)
{
}

static void restore_clock_settings(struct clock_settings *clock_settings)
{
}
#endif

int main(void)
{
	struct clock_settings clock_settings;
	ucg_regs_t *ucg;

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

	save_clock_settings(&clock_settings);

	ucg = (ucg_regs_t *)(TO_VIRT(SERVICE_UCG));
	ucg->BP_CTR_REG = 0xffff;
	udelay(1);

	if ((REG(SERVICE_URB_PLL) & 0xff) != 7) {
		// Setup SERVICE clocks as in UART boot mode
		REG(SERVICE_URB_PLL) = 7; // SERVICE PLL to 216 MHz
		udelay(1);
		ucg->CTR_REG[0] = 0x2 | (8 << 10); // CLK_APB div=8 (27 MHz)
		ucg->CTR_REG[1] = 0x2 | (1 << 10); // CLK_CORE div=1 (216 MHz)
		ucg->CTR_REG[2] = 0x2 | (1 << 10); // CLK_QSPI0 div=1 (216 MHz)
		ucg->CTR_REG[4] = 0x2 | (1 << 10); // CLK_RISC0 div=1 (216 MHz)
		ucg->CTR_REG[13] = 0x2 | (16 << 10); // CLK_QSPI0_EXT div=16 (13.5 MHz)
	}

	// I2C4 clocks are disabled even in UART boot mode
	ucg->CTR_REG[9] = 0x2 | (8 << 10); // CLK_I2C4 div=8 (27 MHz)
	ucg->CTR_REG[12] = 0x2 | (8 << 10); // CLK_I2C4_EXT div=8 (27 MHz)
	udelay(1);
	ucg->SYNC_CLK_REG = 0xfff;
	ucg->BP_CTR_REG = 0;

	// For MIPS timer uses RISC0 frequency. For ARM this function ignored.
	set_tick_freq(XTI_FREQUENCY * 8);

	// Turn PLLs to bypass mode
	REG(HSP_URB_PLL) = 0;
	REG(LSP0_URB_PLL) = 0;
	REG(LSP1_URB_PLL) = 0;

	REG(HSP_URB_RST) = BIT(2) | BIT(18); // Assert reset for QSPI1

	ucg = (ucg_regs_t *)(TO_VIRT(HSP_UCG(0)));
	ucg->CTR_REG[12] = 0x2 | (2 << 10); // CLK_QSPI1 div=2 (13.5 MHz)

	ucg = (ucg_regs_t *)(TO_VIRT(HSP_UCG(1)));
	ucg->CTR_REG[3] = 0x2 | (1 << 10); // CLK_QSPI1_EXT div=1 (27 MHz)

	REG(HSP_URB_QSPI1_PADCFG) = 0x1;
	REG(HSP_URB_QSPI1_SS_PADCFG) = 0x1fa;
	REG(HSP_URB_QSPI1_SISO_PADCFG) = 0x1fa;
	REG(HSP_URB_QSPI1_SCLK_PADCFG) = 0x1fa;

	REG(HSP_URB_RST) = BIT(18); // Deassert reset for QSPI1

	REG(XIP_EN_REQ) = 0; // Out QSPI0 from XIP mode
	REG(HSP_URB_XIP_EN_REQ) = 0; // Out QSPI1 from XIP mode
	while (REG(XIP_EN_OUT) || REG(HSP_URB_XIP_EN_OUT))
		continue;

	ucg = (ucg_regs_t *)(TO_VIRT(LSP0_UCG2));
	ucg->CTR_REG[5] = 0x2 | (1 << 10); // I2C0_CLK div=1 (27 MHz)
	ucg = (ucg_regs_t *)(TO_VIRT(LSP1_UCG));
	ucg->CTR_REG[1] = 0x2 | (1 << 10); // I2C1_CLK div=1 (27 MHz)
	ucg->CTR_REG[2] = 0x2 | (1 << 10); // I2C2_CLK div=1 (27 MHz)
	ucg->CTR_REG[3] = 0x2 | (1 << 10); // I2C3_CLK div=1 (27 MHz)
	ucg->CTR_REG[6] = 0x2 | (1 << 10); // UART_CLK div=1 (27 MHz)

	// Setup UART0 pads
	REG(LSP1_URB_PAD_CTR(PORTB, 6)) = PAD_CTL_CTL(0x7) | PAD_CTL_SL(0x3);
	REG(LSP1_URB_PAD_CTR(PORTB, 7)) = PAD_CTL_E | PAD_CTL_CTL(0x7) | PAD_CTL_SL(0x3) |
					  PAD_CTL_SUS;
	// UART0 in hardware mode
	gpio_set_function_mask(GPIO1, GPIO_BANK_B, BIT(6) | BIT(7), GPIO_FUNC_PERIPH);

	uart_flush(UART0);
	uart_init(UART0, XTI_FREQUENCY, 115200);
	qspi_prepare(0, 1);
	ucg = (ucg_regs_t *)(TO_VIRT(SERVICE_UCG));
	uart_printf(UART0, "%s\n#", APP_NAME);

	while (!need_exit) {
		iface_process();
	}
	restore_clock_settings(&clock_settings);
#ifdef CAN_RETURN
	uart_puts(UART0, "\nExit from " APP_NAME "\n");
	uart_flush(UART0);
#endif

	return 0;
}
