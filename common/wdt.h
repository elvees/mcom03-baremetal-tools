// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#ifndef WDT_H_
#define WDT_H_

#include <regs.h>

void wdt_reset(void);
void wdt_start(uint8_t timeout);

#endif
