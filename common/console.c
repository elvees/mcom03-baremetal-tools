// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <console.h>

#define ESC_UP	  0x5b41
#define ESC_DOWN  0x5b42
#define ESC_RIGHT 0x5b43
#define ESC_LEFT  0x5b44

#define ESC_INS	   0x5b327e
#define ESC_DEL	   0x5b337e
#define ESC_PGUP   0x5b357e
#define ESC_PGDOWN 0x5b367e

#define ESC_END	 0x5b46
#define ESC_HOME 0x5b48

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

static void console_find_cmd(struct console *console, struct console_arg *args, int argc)
{
	struct console_cmd *cmd = NULL;
	bool ok;

	for (int i = 0; i < console->cmds_count; i++) {
		if (!strcmp(console->cmds[i].cmd, console->line)) {
			cmd = &console->cmds[i];
			if (argc < console->cmds[i].arg_min || argc > console->cmds[i].arg_max) {
				uart_puts(console->uart, "Error: Wrong arguments count\n");
				return;
			}
			for (int j = 0; j < argc; j++) {
				if (console->cmds[i].arg_types[j] == ARG_UINT) {
					args[j].uint = str2uint(args[j].str, &ok);
					if (!ok) {
						uart_printf(console->uart,
							    "Error: Argument %d must be integer\n",
							    j);
						return;
					}
				} else
					args[j].uint = 0;
			}
			break;
		}
	}
	if (!cmd) {
		uart_printf(console->uart, "Error: Unknown command '%s'\n", console->line);
		return;
	}
	if (console->run_cmd)
		console->run_cmd(console, cmd, args, argc);
}

static void console_parse(struct console *console)
{
	struct console_arg args[5];
	int argc = 0;
	int size = strlen(console->line);
	bool last_space = false;

	for (int i = 0; i < size; i++) {
		if (console->line[i] == ' ') {
			if (!last_space)
				console->line[i] = '\0';
			last_space = true;
		} else {
			if (last_space) {
				if (argc >= 5) {
					uart_puts(console->uart, "Error: Too many arguments\n");
					return;
				}
				args[argc++].str = &console->line[i];
				last_space = false;
			}
		}
	}
	if (console->line[0] == '\0') {
		if (console->each_line_msg) {
			uart_puts(console->uart, console->each_line_msg);
			uart_putc(console->uart, '\n');
		}
		return;
	}

	console_find_cmd(console, args, argc);
}

static void console_move_coursor(struct console *console, int shift, bool to_left)
{
	char buf[6] = "\x1b[0\0\0\0";
	char dir = to_left ? 'D' : 'C';

	if (!shift)
		return;
	else if (shift > 99)
		shift = 99;

	if (shift < 10) {
		buf[2] = shift + '0';
		buf[3] = dir;
	} else {
		buf[2] = shift / 10 + '0';
		buf[3] = shift % 10 + '0';
		buf[4] = dir;
	}

	uart_puts(console->uart, buf);
}

void console_cmd_line_clear(struct console *console)
{
	int len = strlen(console->prompt);

	len += console->size;
	console_move_coursor(console, len, true);
	for (int i = 0; i < len; i++)
		uart_putc(console->uart, ' ');

	console_move_coursor(console, len, true);
}

void console_cmd_line_restore(struct console *console)
{
	uart_printf(console->uart, "%s%s", console->prompt, console->line);
}

void console_process(struct console *console)
{
	uint8_t ch;

	if (!uart_is_char_ready(console->uart))
		return;

	ch = uart_getchar(console->uart);

	switch (ch) {
	case 0x1b: // Esc
		console->is_esc_seq = true;
		console->esc_seq = 0;
		console->esc_seq_pos = 0;
		break;
	case 0x7f: // Backspace
		if (console->pos) {
			for (int i = console->pos; i < console->size + 1; i++)
				console->line[i - 1] = console->line[i];

			console->pos--;
			console->size--;
			console_move_coursor(console, 1, true);
			uart_printf(console->uart, "%s ", &console->line[console->pos]);
			console_move_coursor(console, console->size - console->pos + 1, true);
		}
		break;
	case 0xd:
		uart_putc(console->uart, '\n');
		console_parse(console);
		uart_puts(console->uart, console->prompt);
		console->pos = 0;
		console->size = 0;
		console->line[0] = '\0';
	case 0xa:
		break;
	default:
		if (console->is_esc_seq) {
			// Current char is a part of escape sequence
			console->esc_seq = (console->esc_seq << 8) | ch;
			if ((console->esc_seq_pos && ch >= 0x41) || console->esc_seq_pos > 8) {
				console->is_esc_seq = false;
				switch (console->esc_seq) {
				case ESC_RIGHT:
					if (console->pos < console->size) {
						console->pos++;
						console_move_coursor(console, 1, false);
					}
					break;
				case ESC_LEFT:
					if (console->pos) {
						console->pos--;
						console_move_coursor(console, 1, true);
					}
					break;
				case ESC_HOME:
					if (console->pos) {
						console_move_coursor(console, console->pos, true);
						console->pos = 0;
					}
					break;
				case ESC_END:
					if (console->pos < console->size) {
						console_move_coursor(console,
								     console->size - console->pos,
								     false);
						console->pos = console->size;
					}
					break;
				}
			} else {
				console->esc_seq_pos++;
			}
		} else if (console->size < (sizeof(console->line) - 2)) {
			for (int i = console->size; i >= console->pos; i--)
				console->line[i + 1] = console->line[i];

			console->line[console->pos] = ch;
			uart_puts(console->uart, &console->line[console->pos]);
			console_move_coursor(console, console->size - console->pos, true);
			console->pos++;
			console->size++;
		}
		break;
	}
}

void console_help(struct console *console)
{
	uart_printf(console->uart, "%s\n\n", console->app_name);
	for (int i = 0; i < console->cmds_count; i++) {
		uart_printf(console->uart, "%s    - %s\n", console->cmds[i].cmd,
			    console->cmds[i].help);
	}
}
