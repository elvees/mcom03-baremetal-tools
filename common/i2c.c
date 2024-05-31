// SPDX-License-Identifier: MIT
// Copyright 2024 RnD Center "ELVEES", JSC

#include <stddef.h>

#include <delay.h>
#include <gpio.h>
#include <i2c.h>

#define I2C_CON_MASTER_MODE	 BIT(0)
#define I2C_CON_SPEED_MASK_STD	 BIT(1)
#define I2C_CON_SPEED_MASK_FAST	 BIT(2)
#define I2C_CON_10BITADDR_SLAVE	 BIT(3)
#define I2C_CON_10BITADDR_MASTER BIT(4)
#define I2C_CON_RESTART_EN	 BIT(5)
#define I2C_CON_SLAVE_DISABLE	 BIT(6)

#define I2C_INTR_MASK_STOP_DET BIT(9)

#define I2C_CMD_READ  BIT(8)
#define I2C_CMD_WRITE ~BIT(8)
#define I2C_CMD_STOP  BIT(9)

#define I2C_ENABLE_ENABLE BIT(0)
#define I2C_ENABLE_ABORT  BIT(1)

/* I2C STATUS register bits */
#define I2C_STATUS_TX_NOT_FULL	BIT(1)
#define I2C_STATUS_TX_EMPTY	BIT(2)
#define I2C_STATUS_RX_NOT_EMPTY BIT(3)
#define I2C_STATUS_MST_ACTIVITY BIT(5)

void i2c_init(struct i2c *i2c, uint32_t speed, uint32_t i2c_input_clk)
{
	uint32_t con_val = 0;
	uint32_t scl_val = i2c_input_clk / speed / 2;

	i2c->ENABLE = 0x0;

	con_val = I2C_CON_SLAVE_DISABLE | I2C_CON_RESTART_EN | I2C_CON_MASTER_MODE;
	if (speed == I2C_STANDARD_SPEED) {
		con_val |= I2C_CON_SPEED_MASK_STD;
		i2c->SS_SCL_HCNT = scl_val;
		i2c->SS_SCL_LCNT = scl_val;
	} else if (speed == I2C_FAST_SPEED) {
		con_val |= I2C_CON_SPEED_MASK_FAST;
		i2c->FS_SCL_HCNT = scl_val;
		i2c->FS_SCL_LCNT = scl_val;
	}
	i2c->CON = con_val;
	i2c->INTR_MASK = I2C_INTR_MASK_STOP_DET;
}

void i2c_pads_cfg(uint32_t i2c_num)
{
	if (i2c_num == 0) {
		/* There are no registers for GPIO0 to enable
		 * the pad receiver */
		gpio_set_function_mask(GPIO0, GPIO_BANK_D, BIT(4) | BIT(3), GPIO_FUNC_PERIPH);
	} else if (i2c_num < 4) {
		REG(LSP1_URB_PAD_CTR(PORTA, (2 * i2c_num - 2))) |= PAD_CTL_E;
		REG(LSP1_URB_PAD_CTR(PORTA, (2 * i2c_num - 1))) |= PAD_CTL_E;
		gpio_set_function_mask(GPIO1, GPIO_BANK_A,
				       BIT(2 * i2c_num - 2) | BIT(2 * i2c_num - 1),
				       GPIO_FUNC_PERIPH);
	}
}

static bool i2c_status_wait(struct i2c *i2c, uint32_t bit_mask, uint32_t value)
{
	uint32_t val = 0;

	return !poll_timeout(i2c->STATUS, val, (val & bit_mask) == value, 0, 10000);
}

bool i2c_write(struct i2c *i2c, uint32_t addr, uint32_t regaddr, uint32_t alen, uint8_t *buf,
	       uint32_t size)
{
	bool ret = true;

	i2c->TAR = addr;
	i2c->ENABLE = I2C_ENABLE_ENABLE;

	while (alen) {
		alen--;
		i2c->DATA_CMD = (regaddr >> (alen * 8)) & 0xFF;
	}

	for (uint32_t i = 0; i < size; i++) {
		ret = i2c_status_wait(i2c, I2C_STATUS_TX_NOT_FULL, I2C_STATUS_TX_NOT_FULL);
		if (!ret)
			break;

		if (i < size - 1)
			i2c->DATA_CMD = buf[i];
		else
			i2c->DATA_CMD = buf[i] | I2C_CMD_STOP;
	}

	if (ret)
		ret = i2c_status_wait(i2c, I2C_STATUS_TX_EMPTY, I2C_STATUS_TX_EMPTY);

	if (ret)
		ret = i2c_status_wait(i2c, I2C_STATUS_MST_ACTIVITY, 0);

	if (i2c->RAW_INTR_STAT & RAW_INTR_STAT_TX_ABRT) {
		ret = false;
		(void)i2c->CLR_TX_ABRT;
	}

	i2c->ENABLE = 0;

	return ret;
}

bool i2c_read(struct i2c *i2c, uint32_t addr, uint32_t regaddr, uint32_t alen, uint8_t *buf,
	      uint32_t size)
{
	unsigned long tick_start;
	uint32_t requested = 0;
	uint32_t readed = 0;
	bool ret = true;

	i2c->TAR = addr;
	i2c->ENABLE = I2C_ENABLE_ENABLE;

	while (alen) {
		alen--;
		i2c->DATA_CMD = (regaddr >> (alen * 8)) & 0xFF;
	}

	tick_start = get_tick_counter();

	while (readed < size) {
		if (requested < size && (i2c->STATUS & I2C_STATUS_TX_NOT_FULL)) {
			requested++;
			i2c->DATA_CMD = (requested == size) ? (I2C_CMD_READ | I2C_CMD_STOP) :
							      I2C_CMD_READ;
		}
		if (i2c->STATUS & I2C_STATUS_RX_NOT_EMPTY) {
			buf[readed] = i2c->DATA_CMD & 0xFF;
			readed++;
			tick_start = get_tick_counter(); // Clear timeout counter
		}
		if (ticks_to_us(ticks_since(tick_start)) > 10000) {
			// Timeout. Initiate abort operation...
			i2c->ENABLE |= I2C_ENABLE_ABORT;
			ret = false;
			break;
		}
	}

	ret = i2c_status_wait(i2c, I2C_STATUS_MST_ACTIVITY, 0) & ret;
	i2c->ENABLE = 0;

	return ret;
}
