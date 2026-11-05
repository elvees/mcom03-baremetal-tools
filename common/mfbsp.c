// SPDX-License-Identifier: MIT
// Copyright 2026 RnD Center "ELVEES", JSC

#include <mfbsp.h>
#include <gpio.h>

#include <uart.h>

#define CSR_MFBSP_LEN	     BIT(0)
#define CSR_MFSBP_SPI_I2S_EN BIT(9)
#define TCTR_TMODE	     BIT(1)
#define RCTR_RMODE	     BIT(1)

void mfbsp_set_mode(struct mfbsp *mfbsp, enum mfbsp_mode mode)
{
	uint32_t csr = mfbsp->CSR_MFBSP & ~(CSR_MFBSP_LEN | CSR_MFSBP_SPI_I2S_EN);
	uint32_t tctr = mfbsp->TCTR & ~TCTR_TMODE;
	uint32_t rctr = mfbsp->RCTR & ~RCTR_RMODE;

	switch (mode) {
	case MFBSP_MODE_GPIO:
		break;
	case MFBSP_MODE_LPORT:
		csr |= CSR_MFBSP_LEN;
		break;
	case MFBSP_MODE_SERIAL_I2S_TX_I2S_RX:
		csr |= CSR_MFSBP_SPI_I2S_EN;
		break;
	case MFBSP_MODE_SERIAL_SPI_TX_SPI_RX:
		csr |= CSR_MFSBP_SPI_I2S_EN;
		rctr |= RCTR_RMODE;
		tctr |= TCTR_TMODE;
		break;
	}

	mfbsp->CSR_MFBSP = csr;
	mfbsp->TCTR = tctr;
	mfbsp->RCTR = rctr;
}

void mfbsp_gpio_set_pin_dir(struct mfbsp *mfbsp, enum mfbsp_pins pin, uint8_t dir)
{
	uint16_t val = mfbsp->DIR_MFBSP;

	if (dir == GPIO_DIR_OUT)
		val |= BIT(pin);
	else
		val &= ~BIT(pin);

	mfbsp->DIR_MFBSP = val;
}

void mfbsp_gpio_set_pin_value(struct mfbsp *mfbsp, enum mfbsp_pins pin, bool value)
{
	uint16_t val = mfbsp->GPIO_DR;

	if (value)
		val |= BIT(pin);
	else
		val &= ~BIT(pin);

	mfbsp->GPIO_DR = val;
}

bool mfbsp_gpio_get_pin_value(struct mfbsp *mfbsp, enum mfbsp_pins pin)
{
	uint16_t val = mfbsp->GPIO_DR;

	return (val >> pin) & 1;
}
