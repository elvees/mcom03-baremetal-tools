// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#include <wdt.h>

#define WDT_CRR_RESET_VALUE 0x76

void wdt_reset(void)
{
	REG(WDT_CRR) = WDT_CRR_RESET_VALUE;
}

void wdt_start(uint8_t timeout)
{
	uint32_t torr;

	torr = REG(WDT_TORR);
	torr &= ~0xff;
	torr |= timeout | (timeout << 4);
	REG(WDT_TORR) = torr;

	wdt_reset();

	REG(WDT_CR) = 0x1;
}
