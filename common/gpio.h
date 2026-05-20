// SPDX-License-Identifier: MIT
// Copyright 2024 RnD Center "ELVEES", JSC

#ifndef QGPIO_H_
#define QGPIO_H_

#include <stdint.h>

#define GPIO0 GPIO0_BASE
#define GPIO1 GPIO1_BASE

#define GPIO_BANK_A 0
#define GPIO_BANK_B 1
#define GPIO_BANK_C 2
#define GPIO_BANK_D 3

#define GPIO_FUNC_GPIO	 0
#define GPIO_FUNC_PERIPH 1

#define GPIO_DIR_IN  0
#define GPIO_DIR_OUT 1

#define _gpio_set_mask(g, b, offset, mask, val, val_for_zero)       \
	{                                                           \
		if ((val) == (val_for_zero))                        \
			REG((g) + (b) * 0xc + (offset)) &= ~(mask); \
		else                                                \
			REG((g) + (b) * 0xc + (offset)) |= (mask);  \
	}

#define gpio_set_function_mask(g, b, mask, func) \
	_gpio_set_mask(g, b, 0x8, mask, func, GPIO_FUNC_GPIO)
#define gpio_set_direction_mask(g, b, mask, dir) _gpio_set_mask(g, b, 0x4, mask, dir, GPIO_DIR_IN)
#define gpio_set_value_mask(g, b, mask, val_mask)                                                \
	{                                                                                        \
		REG((g) + (b) * 0xc) = (REG((g) + (b) * 0xc) & ~(mask)) | ((val_mask) & (mask)); \
	}

#define gpio_set_function(g, b, pin, func) gpio_set_function_mask(g, b, BIT(pin), func)
#define gpio_set_direction(g, b, pin, dir) gpio_set_direction_mask(g, b, BIT(pin), dir)
#define gpio_set_value(g, b, pin, val)	   gpio_set_value_mask(g, b, BIT(pin), (!!(val)) << (pin))
#define gpio_get_value(g, b, pin)	   ((REG((g) + 0x50 + (b) * 0x4) >> (pin)) & 0x1)

#endif
