// SPDX-License-Identifier: MIT
// Copyright 2024 RnD Center "ELVEES", JSC

#ifndef DELAY_H_
#define DELAY_H_

#include <stdint.h>

#define poll_timeout(op, val, cond, sleep_us, timeout_us)                              \
	({                                                                             \
		unsigned long tick_start = get_tick_counter();                         \
		unsigned long timeout = get_ticks_per_us() * timeout_us;               \
		for (;;) {                                                             \
			val = op;                                                      \
			if (cond)                                                      \
				break;                                                 \
			if (timeout_us >= 0 && ticks_since(tick_start) >= (timeout)) { \
				val = op;                                              \
				break;                                                 \
			}                                                              \
			if (sleep_us)                                                  \
				udelay(sleep_us);                                      \
		}                                                                      \
		(cond) ? 0 : -1;                                                       \
	})

// Set new frequency for CPU ticks counter. Used only for MIPS CPU
void set_tick_freq(unsigned long freq);

// Get current ticks counter frequency
unsigned long get_tick_freq(void);

// Get current value of ticks counter
unsigned long get_tick_counter(void);

// Get count of ticks for 1 microsecond
uint32_t get_ticks_per_us(void);

// Return ticks count since 'tick'
unsigned long ticks_since(unsigned long tick);

// Delay for 'us' microseconds
void udelay(uint32_t us);

// Delay for 'ms' milliseconds
void mdelay(uint32_t ms);

// Convert ticks count to microseconds
uint32_t ticks_to_us(unsigned long ticks);

#endif
