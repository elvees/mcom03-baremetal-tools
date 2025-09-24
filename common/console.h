// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <stdbool.h>
#include <stdint.h>

#include <uart.h>

#define ARG_STR	 0
#define ARG_UINT 1

#define VERBOSE_LEVEL_ERROR   0
#define VERBOSE_LEVEL_WARNING 1
#define VERBOSE_LEVEL_INFO    2
#define VERBOSE_LEVEL_DEBUG   3

#define VERBOSE_PRINTF(min_level, msg, ...)                     \
	do {                                                    \
		if (verbose_level >= min_level)                 \
			uart_printf(UART0, msg, ##__VA_ARGS__); \
	} while (0)

// Define for source
#define DECLARE_VERBOSE_FUNCTIONS(name)          \
	uint8_t verbose_level;                   \
	void name##_set_verbose(uint8_t verbose) \
	{                                        \
		verbose_level = verbose;         \
	}                                        \
	uint8_t name##_get_verbose(void)         \
	{                                        \
		return verbose_level;            \
	}

// Define for header
#define DECLARE_VERBOSE(name)                     \
	void name##_set_verbose(uint8_t verbose); \
	uint8_t name##_get_verbose(void);

struct console_cmd {
	char *cmd;
	char *help;
	uint16_t cmd_id;
	uint8_t arg_min;
	uint8_t arg_max;
	uint8_t arg_types[8];
};

struct console_arg {
	char *str;
	uint32_t uint;
};

/* uart - pointer to struct uart
 * cmds - array of commands
 * cmds_count - count of commands
 * prompt - prompt string
 * app_name - application name that will be printed at enter on empty command line
 * pos - current position in `line` (must be initialized as 0)
 * line - buffer for command line (must be initialized with '\0' in line[0])
 */
struct console {
	struct uart *uart;
	struct console_cmd *cmds;
	int cmds_count;
	char *prompt;
	char *app_name;
	char *each_line_msg;
	void (*run_cmd)(struct console *console, struct console_cmd *cmd, struct console_arg *args,
			int argc);
	int pos;
	int size;
	char line[64];
	uint32_t esc_seq;
	bool is_esc_seq; // if true then next input char is part of escape sequence
	uint8_t esc_seq_pos; // how many bytes of escape sequency are received
};

void console_cmd_line_clear(struct console *console);
void console_cmd_line_restore(struct console *console);
void console_process(struct console *console);
void console_help(struct console *console);

#endif
