// SPDX-License-Identifier: MIT
// Copyright 2024 RnD Center "ELVEES", JSC

#include <stdint.h>
#include <delay.h>
#include <regs.h>

uint32_t get_ticks_per_us(void)
{
	return _get_tick_freq() / 1000000;
}

unsigned long get_tick_freq(void)
{
	return _get_tick_freq();
}

unsigned long get_tick_counter(void)
{
	return _get_tick_counter();
}

unsigned long ticks_since(unsigned long tick)
{
	return _get_tick_counter() - tick;
}

static void delay_ticks(uint32_t ticks_count)
{
	unsigned long tick_start = _get_tick_counter();
	unsigned long tick = tick_start + ticks_count;

	if (tick < tick_start)
		while (_get_tick_counter() > tick_start)
			continue;

	while (_get_tick_counter() < tick)
		continue;
}

void udelay(uint32_t us)
{
	uint32_t ticks_per_s = _get_tick_freq();
	uint32_t ticks_per_us = ticks_per_s / 1000000;

	while (us > 1000000) {
		delay_ticks(ticks_per_s);
		us -= 1000000;
	}
	delay_ticks(us * ticks_per_us);
}

void mdelay(uint32_t ms)
{
	uint32_t ticks_per_s = _get_tick_freq();
	uint32_t ticks_per_ms = ticks_per_s / 1000;

	while (ms > 1000) {
		delay_ticks(ticks_per_s);
		ms -= 1000;
	}
	delay_ticks(ms * ticks_per_ms);
}

uint32_t ticks_to_us(unsigned long ticks)
{
	return ticks / (_get_tick_freq() / 1000000);
}

int poll_read32_mask_timeout(uintptr_t reg_addr, uint32_t mask, uint32_t value, unsigned long timeout_us)
{
	uint32_t val;

	return poll_timeout(REG(reg_addr), val, (val & mask) == value, 0, timeout_us);
}
