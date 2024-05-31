// SPDX-License-Identifier: MIT
// Copyright 2024 RnD Center "ELVEES", JSC

#include <stdint.h>

#ifdef MIPS32
static unsigned long tick_freq = XTI_FREQUENCY;

void set_tick_freq(unsigned long freq)
{
	tick_freq = freq;
}

static inline unsigned long _get_tick_freq(void)
{
	return tick_freq;
}

static inline unsigned long _get_tick_counter(void)
{
	unsigned long count;

	asm volatile("mfc0 %0, $9" : "=r"(count));

	return count;
}

#else
void set_tick_freq(unsigned long freq)
{
}

static inline unsigned long _get_tick_freq(void)
{
	unsigned long cntfrq;

	asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq));

	return cntfrq;
}

static inline unsigned long _get_tick_counter(void)
{
	unsigned long cntpct;

	asm volatile("isb sy" : : : "memory");
	asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct));

	return cntpct;
}
#endif

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
