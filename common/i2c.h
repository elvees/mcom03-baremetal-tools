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
	volatile uint32_t CON;
	volatile uint32_t TAR;
	volatile uint32_t SAR;
	volatile uint32_t HS_MADDR;
	volatile uint32_t DATA_CMD;
	volatile uint32_t SS_SCL_HCNT;
	volatile uint32_t SS_SCL_LCNT;
	volatile uint32_t FS_SCL_HCNT;
	volatile uint32_t FS_SCL_LCNT;
	volatile uint32_t HS_SCL_HCNT;
	volatile uint32_t HS_SCL_LCNT;
	volatile uint32_t INTR_STAT;
	volatile uint32_t INTR_MASK;
	volatile uint32_t RAW_INTR_STAT;
	volatile uint32_t RX_TL;
	volatile uint32_t TX_TL;
	volatile uint32_t CLR_INTR;
	volatile uint32_t CLR_RX_UNDER;
	volatile uint32_t CLR_RX_OVER;
	volatile uint32_t CLR_TX_OVER;
	volatile uint32_t CLR_RD_REQ;
	volatile uint32_t CLR_TX_ABRT;
	volatile uint32_t CLR_RX_DONE;
	volatile uint32_t CLR_ACTIVITY;
	volatile uint32_t CLR_STOP_DET;
	volatile uint32_t CLR_START_DET;
	volatile uint32_t CLR_GEN_CALL;
	volatile uint32_t ENABLE;
	volatile uint32_t STATUS;
	volatile uint32_t TXFLR;
	volatile uint32_t RXFLR;
	volatile uint32_t SDA_HOLD;
	volatile uint32_t TX_ABRT_SOURCE;
	volatile uint32_t SLV_DATA_NACK_ONLY;
	volatile uint32_t DMA_CR;
	volatile uint32_t DMA_TDLR;
	volatile uint32_t DMA_RDLR;
	volatile uint32_t SDA_SETUP;
	volatile uint32_t ACK_GENERAL_CALL;
	volatile uint32_t ENABLE_STATUS;
	volatile uint32_t FS_SPKLEN;
	volatile uint32_t HS_SPKLEN;
	volatile uint32_t CLR_RESTART_DET;
	volatile uint32_t SCL_STUCK_AT_LOW_TIMEOUT;
	volatile uint32_t SDA_STUCK_AT_LOW_TIMEOUT;
	volatile uint32_t CLR_SCL_STUCK_DET;
	volatile uint32_t DEVICE_ID;
	volatile uint32_t SMBUS_CLK_LOW_SEXT;
	volatile uint32_t SMBUS_CLK_LOW_MEXT;
	volatile uint32_t SMBUS_THIGH_MAX_IDLE_COUNT;
	volatile uint32_t SMBUS_INTR_STAT;
	volatile uint32_t SMBUS_INTR_MASK;
	volatile uint32_t SMBUS_RAW_INTR_STAT;
	volatile uint32_t CLR_SMBUS_INTR;
	volatile uint32_t OPTIONAL_SAR;
	volatile uint32_t SMBUS_UDID_LSB;
	volatile uint32_t RESERVED;
	volatile uint32_t COMP_PARAM_1;
	volatile uint32_t COMP_VERSION;
	volatile uint32_t COMP_TYPE;
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
 * Example of read from EEPROM with bus addres 0x50 with 2-byte address (alen=2, regaddr=0x123)
 * <START> <addr+rd_bit> <regaddr_hi> <regaddr_lo> <RESTART> (read data)..... <STOP>
 * <START> <10100001> <0x01> <0x23> <RESTART> ..... <STOP> */
bool i2c_read(struct i2c *i2c, uint32_t addr, uint32_t regaddr, uint32_t alen, uint8_t *buf,
	      uint32_t size);

/* Write data to I2C device */
bool i2c_write(struct i2c *i2c, uint32_t addr, uint32_t regaddr, uint32_t alen, uint8_t *buf,
	       uint32_t size);

#endif /* _I2C_H */
