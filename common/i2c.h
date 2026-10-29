// SPDX-License-Identifier: MIT
// Copyright 2024 RnD Center "ELVEES", JSC

#ifndef _I2C_H
#define _I2C_H

#include <stdbool.h>
#include <stdint.h>

#include <regs.h>

#define I2C0 ((struct i2c *)(TO_VIRT(I2C0_BASE)))
#define I2C1 ((struct i2c *)(TO_VIRT(I2C1_BASE)))
#define I2C2 ((struct i2c *)(TO_VIRT(I2C2_BASE)))
#define I2C3 ((struct i2c *)(TO_VIRT(I2C3_BASE)))
#define I2C4 ((struct i2c *)(TO_VIRT(I2C4_BASE)))

/* I2C registers */
struct i2c {
	volatile uint32_t CON; /* 0x00 */
	volatile uint32_t TAR; /* 0x04 */
	volatile uint32_t SAR; /* 0x08 */
	volatile uint32_t HS_MADDR; /* 0x0c */
	volatile uint32_t DATA_CMD; /* 0x10 */
	volatile uint32_t SS_SCL_HCNT; /* 0x14 */
	volatile uint32_t SS_SCL_LCNT; /* 0x18 */
	volatile uint32_t FS_SCL_HCNT; /* 0x1c */
	volatile uint32_t FS_SCL_LCNT; /* 0x20 */
	volatile uint32_t HS_SCL_HCNT; /* 0x24 */
	volatile uint32_t HS_SCL_LCNT; /* 0x28 */
	volatile uint32_t INTR_STAT; /* 0x2c */
	volatile uint32_t INTR_MASK; /* 0x30 */
	volatile uint32_t RAW_INTR_STAT; /* 0x34 */
	volatile uint32_t RX_TL; /* 0x38 */
	volatile uint32_t TX_TL; /* 0x3c */
	volatile uint32_t CLR_INTR; /* 0x40 */
	volatile uint32_t CLR_RX_UNDER; /* 0x44 */
	volatile uint32_t CLR_RX_OVER; /* 0x48 */
	volatile uint32_t CLR_TX_OVER; /* 0x4c */
	volatile uint32_t CLR_RD_REQ; /* 0x50 */
	volatile uint32_t CLR_TX_ABRT; /* 0x54 */
	volatile uint32_t CLR_RX_DONE; /* 0x58 */
	volatile uint32_t CLR_ACTIVITY; /* 0x5c */
	volatile uint32_t CLR_STOP_DET; /* 0x60 */
	volatile uint32_t CLR_START_DET; /* 0x64 */
	volatile uint32_t CLR_GEN_CALL; /* 0x68 */
	volatile uint32_t ENABLE; /* 0x6c */
	volatile uint32_t STATUS; /* 0x70 */
	volatile uint32_t TXFLR; /* 0x74 */
	volatile uint32_t RXFLR; /* 0x78 */
	volatile uint32_t SDA_HOLD; /* 0x7c */
	volatile uint32_t TX_ABRT_SOURCE; /* 0x80 */
	volatile uint32_t SLV_DATA_NACK_ONLY; /* 0x84 */
	volatile uint32_t DMA_CR; /* 0x88 */
	volatile uint32_t DMA_TDLR; /* 0x8c */
	volatile uint32_t DMA_RDLR; /* 0x90 */
	volatile uint32_t SDA_SETUP; /* 0x94 */
	volatile uint32_t ACK_GENERAL_CALL; /* 0x98 */
	volatile uint32_t ENABLE_STATUS; /* 0x9c */
	volatile uint32_t FS_SPKLEN; /* 0xa0 */
	volatile uint32_t HS_SPKLEN; /* 0xa4 */
	volatile uint32_t CLR_RESTART_DET; /* 0xa8 */
	volatile uint32_t SCL_STUCK_AT_LOW_TIMEOUT; /* 0xac */
	volatile uint32_t SDA_STUCK_AT_LOW_TIMEOUT; /* 0xb0 */
	volatile uint32_t CLR_SCL_STUCK_DET; /* 0xb4 */
	volatile uint32_t DEVICE_ID; /* 0xb8 */
	volatile uint32_t SMBUS_CLK_LOW_SEXT; /* 0xbc */
	volatile uint32_t SMBUS_CLK_LOW_MEXT; /* 0xc0 */
	volatile uint32_t SMBUS_THIGH_MAX_IDLE_COUNT; /* 0xc4 */
	volatile uint32_t SMBUS_INTR_STAT; /* 0xc8 */
	volatile uint32_t SMBUS_INTR_MASK; /* 0xcc */
	volatile uint32_t SMBUS_RAW_INTR_STAT; /* 0xd0 */
	volatile uint32_t CLR_SMBUS_INTR; /* 0xd4 */
	volatile uint32_t OPTIONAL_SAR; /* 0xd8 */
	volatile uint32_t SMBUS_UDID_LSB; /* 0xdc */
	volatile uint32_t RESERVED[(0xf4 - 0xe0) / sizeof(uint32_t)]; /* 0xe0 --- 0xf0 */
	volatile uint32_t COMP_PARAM_1; /* 0xf4 */
	volatile uint32_t COMP_VERSION; /* 0xf8 */
	volatile uint32_t COMP_TYPE; /* 0xfc */
};

/* Register bits */
#define RAW_INTR_STAT_TX_ABRT BIT(6)

/* CONFIGS */
#define I2C_STANDARD_SPEED 100000
#define I2C_FAST_SPEED	   400000

/* Initialize I2C controller */
void i2c_init(struct i2c *i2c, uint32_t speed, uint32_t i2c_input_clk);

/* Setup pads, that used by I2C controller */
void i2c_pads_cfg(uint32_t i2c_num);

/* Read data from I2C device
 * addr - address of device on the bus
 * regaddr - address of register in device
 * alen - count of bytes of register address
 * buf - buffer to place data
 * size - size of data to read
 *
 * Example of read from EEPROM with bus address 0x50 with 2-byte address (alen=2, regaddr=0x123)
 * <START> <addr+rd_bit> <regaddr_hi> <regaddr_lo> <RESTART> (read data)..... <STOP>
 * <START> <10100001> <0x01> <0x23> <RESTART> ..... <STOP> */
bool i2c_read(struct i2c *i2c, uint32_t addr, uint32_t regaddr, uint32_t alen, uint8_t *buf,
	      uint32_t size);

/* Write data to I2C device */
bool i2c_write(struct i2c *i2c, uint32_t addr, uint32_t regaddr, uint32_t alen, uint8_t *buf,
	       uint32_t size);

#endif /* _I2C_H */
