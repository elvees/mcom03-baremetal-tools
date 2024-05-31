// SPDX-License-Identifier: MIT
// Copyright 2024 RnD Center "ELVEES", JSC

#include <stdint.h>
#include <stdbool.h>

#include <qspi.h>
#include <regs.h>

#define CTRL_CONT_XFER BIT(0)
#define CTRL_MSB1ST    BIT(2)
#define CTRL_CPHA      BIT(3)
#define CTRL_CPOL      BIT(4)
#define CTRL_MASTER    BIT(5)
#define CTRL_DMA       BIT(10)
#define CTRL_MWAIT_EN  BIT(11)

static void _qspi_select_slave(struct qspi *qspi, uint8_t ss)
{
	qspi->SS = BIT(ss);
}

void qspi_init(struct qspi *qspi, uint8_t ss)
{
	qspi->CTRL = CTRL_MASTER | CTRL_MSB1ST | CTRL_CONT_XFER;
	qspi->CTRL_AUX = 0x700; // bitsize = 8 bits
	_qspi_select_slave(qspi, ss);
	qspi->ENABLE = 0x1;

	// Set high level to HOLD and WP pads
	qspi->GPO_SET = 0xc0c;
	qspi->GPO_CLR = 0x10c0;
}

void qspi_select_slave(struct qspi *qspi, uint8_t ss)
{
	_qspi_select_slave(qspi, ss);
}

static void qspi_stat_wait_mask(struct qspi *qspi, uint32_t mask, uint32_t value)
{
	while ((qspi->STAT & mask) != value) {
	}
}

void qspi_xfer(struct qspi *qspi, void *tx_buf, void *rx_buf, int len, bool is_last)
{
	uint8_t *tx = (uint8_t *)tx_buf;
	uint8_t *rx = (uint8_t *)rx_buf;
	int rx_count = 0;
	int tx_count = 0;

	qspi->CTRL_AUX |= 0x80; // assert SS signal
	while (tx_count < len) {
		if (tx) {
			qspi->TX_DATA = tx[tx_count];
			tx_count++;
		} else {
			if (qspi->STAT & 0x4) {
				if ((len - tx_count) > 0xf0) {
					qspi->TX_DUMMY = 0xf0;
					tx_count += 0xf0;
				} else {
					qspi->TX_DUMMY = len - tx_count;
					tx_count += len - tx_count;
				}
			}
		}

		while ((qspi->STAT & 0x20) == 0) {
			if (rx)
				rx[rx_count] = qspi->RX_DATA & 0xff;
			else
				(void)qspi->RX_DATA;
			rx_count++;
		}
	}
	while (rx_count < len) {
		qspi_stat_wait_mask(qspi, 0x20, 0); // wait for clear RX_EMPTY
		if (rx)
			rx[rx_count] = qspi->RX_DATA & 0xff;
		else
			(void)qspi->RX_DATA;
		rx_count++;
	}
	qspi_stat_wait_mask(qspi, 0x1, 0); // wait while xfer in progress
	if (is_last)
		qspi->CTRL_AUX &= ~0x80; // deassert SS signal
}
