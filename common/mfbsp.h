// SPDX-License-Identifier: MIT
// Copyright 2026 RnD Center "ELVEES", JSC

#ifndef _MFBSP_H
#define _MFBSP_H

#include <stdbool.h>
#include <regs.h>

#define MFBSP0 ((struct mfbsp *)(TO_VIRT(MFBSP0_BASE + 0x1300)))
#define MFBSP1 ((struct mfbsp *)(TO_VIRT(MFBSP1_BASE + 0x1300)))

struct mfbsp {
	union {
		volatile uint32_t TX_MFBSP;
		volatile uint32_t RX_MFBSP;
	}; /* 0x00 */
	volatile uint32_t CSR_MFBSP; /* 0x04 */
	volatile uint32_t DIR_MFBSP; /* 0x08 */
	volatile uint32_t GPIO_DR; /* 0x0c */
	volatile uint32_t TCTR; /* 0x10 */
	volatile uint32_t RCTR; /* 0x14 */
	volatile uint32_t TSR; /* 0x18 */
	volatile uint32_t RSR; /* 0x1c */
	volatile uint32_t TCTR_RATE; /* 0x20 */
	volatile uint32_t RCTR_RATE; /* 0x24 */
	volatile uint32_t TSTART; /* 0x28 */
	volatile uint32_t RSTART; /* 0x2C */
	volatile uint32_t EMERG_MFBSP; /* 0x30 */
	volatile uint32_t IMASK_MFBSP; /* 0x34 */
};

/*
 * MFBSP modes are defined according to Table 37.4 in chapter 37.2.1
 * of the K1892VM21YA User Manual.
 *
 * Only the two "matching" serial combinations are provided:
 * I2S_TX/I2S_RX and SPI_TX/SPI_RX. Mixed modes (I2S_TX/SPI_RX and
 * SPI_TX/I2S_RX) are not supported yet: they are exotic and not
 * needed by the current use cases. Add them here (and the matching
 * case in mfbsp_set_mode()) if that changes.
 */
enum mfbsp_mode {
	MFBSP_MODE_GPIO,
	MFBSP_MODE_LPORT,
	MFBSP_MODE_SERIAL_I2S_TX_I2S_RX,
	MFBSP_MODE_SERIAL_SPI_TX_SPI_RX,
};

enum mfbsp_pins {
	MFBSP_LACK,
	MFBSP_LCLK,
	MFBSP_LDAT0,
	MFBSP_LDAT1,
	MFBSP_LDAT2,
	MFBSP_LDAT3,
	MFBSP_LDAT4,
	MFBSP_LDAT5,
	MFBSP_LDAT6,
	MFBSP_LDAT7,
};

void mfbsp_set_mode(struct mfbsp *mfbsp, enum mfbsp_mode mode);

/*
 * These functions used to work with MFBSP in GPIO mode.
 */

void mfbsp_gpio_set_pin_dir(struct mfbsp *mfbsp, enum mfbsp_pins pin, uint8_t dir);
void mfbsp_gpio_set_pin_value(struct mfbsp *mfbsp, enum mfbsp_pins pin, bool value);
bool mfbsp_gpio_get_pin_value(struct mfbsp *mfbsp, enum mfbsp_pins pin);

#endif
