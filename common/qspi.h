// SPDX-License-Identifier: MIT
// Copyright 2024 RnD Center "ELVEES", JSC

#ifndef QSPI_H_
#define QSPI_H_

#include <stdint.h>

#define QSPI0 (struct qspi *)(TO_VIRT(QSPI0_BASE))
#define QSPI1 (struct qspi *)(TO_VIRT(QSPI1_BASE))

struct qspi {
	volatile uint32_t TX_DATA;
	volatile uint32_t RX_DATA;
	volatile uint32_t reserved08;
	volatile uint32_t CTRL;
	volatile uint32_t CTRL_AUX;
	volatile uint32_t STAT;
	volatile uint32_t SS;
	volatile uint32_t SS_POLARITY;
	volatile uint32_t INTR_EN;
	volatile uint32_t INTR_STAT;
	volatile uint32_t INTR_CLR;
	volatile uint32_t TX_FIFO_LVL;
	volatile uint32_t RX_FIFO_LVL;
	volatile uint32_t reserved34;
	volatile uint32_t MASTER_DELAY;
	volatile uint32_t ENABLE;
	volatile uint32_t GPO_SET;
	volatile uint32_t GPO_CLR;
	volatile uint32_t FIFO_DEPTH;
	volatile uint32_t FIFO_WMARK;
	volatile uint32_t TX_DUMMY;
};

/* Prepare QSPI controller and select SS signal.
 * qspi: QSPI registers pointer.
 * ss: SS signal number.
 */
void qspi_init(struct qspi *qspi, uint8_t ss);

/* Select SS signal.
 * qspi: QSPI registers pointer.
 * ss: SS signal number.
 */
void qspi_select_slave(struct qspi *qspi, uint8_t ss);

/* Transfer data.
 * qspi: QSPI registers pointer.
 * tx_buf: pointer to data for transmit. If NULL then will be send via MOSI `len` bytes of dummy
 *         data.
 * rx_buf: pointer to data for receive. If NULL then no data for receive (all data on MISO will be
 *         ignored).
 * len: length of transmit/receive data.
 * is_last:
 */
void qspi_xfer(struct qspi *qspi, void *tx_buf, void *rx_buf, int len, bool is_last);

#endif
