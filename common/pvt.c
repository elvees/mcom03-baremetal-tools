// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#include <stdbool.h>
#include <stdint.h>

#include <clk.h>
#include <console.h>
#include <delay.h>
#include <regs.h>
#include <uart.h>

#define PVT_BASE      0x1f050000
#define PVT_TS_BASE   (PVT_BASE + 0x10)
#define PVT_VM_BASE   (PVT_BASE + 0x184)
#define PVT_VM_REG(x) (PVT_VM_BASE + (x))
#define PVT_TS_REG(x) (PVT_TS_BASE + (x))

#define PVT_TS_CLK_SYNTH	 PVT_TS_REG(0x0)
#define PVT_TS_SDIF_DISABLE	 PVT_TS_REG(0x4)
#define PVT_TS_SDIF_STATUS	 PVT_TS_REG(0x8)
#define PVT_TS_SDIF_CTRL	 PVT_TS_REG(0xc)
#define PVT_TS_SDIF		 PVT_TS_REG(0x10)
#define PVT_TS_SDIF_RDATA(n)	 PVT_TS_REG(0x14 + (n) * 0x4)
#define PVT_TS_DATA(n)		 PVT_TS_REG(0x34 + (n) * 0x4)
#define PVT_TS_SMPL_CTRL	 PVT_TS_REG(0x54)
#define PVT_TS_SMPL_HI_CLR	 PVT_TS_REG(0x58)
#define PVT_TS_SMPL_LO_SET	 PVT_TS_REG(0x5c)
#define PVT_TS_SMPL_STATUS	 PVT_TS_REG(0x60)
#define PVT_TS_SMPL_CTR		 PVT_TS_REG(0x64)
#define PVT_TS_SMPL_HILO(n)	 PVT_TS_REG(0x68 + (n) * 0x4)
#define PVT_TS_IRQ_STATUS	 PVT_TS_REG(0x88)
#define PVT_TS_ALARMA_IRQ_STATUS PVT_TS_REG(0x8c)
#define PVT_TS_ALARMB_IRQ_STATUS PVT_TS_REG(0x90)
#define PVT_TS_DONE_IRQ		 PVT_TS_REG(0x94)
#define PVT_TS_FAULT_IRQ	 PVT_TS_REG(0x98)
#define PVT_TS_ALARM_IRQ(n)	 PVT_TS_REG(0x9c + (n) * 0x4)
#define PVT_TS_DONE_IRQ_ENA	 PVT_TS_REG(0xbc)
#define PVT_TS_FAULT_IRQ_ENA	 PVT_TS_REG(0xc0)
#define PVT_TS_ALARM_IRQ_ENA(n)	 PVT_TS_REG(0xc4 + (n) * 0x4)
#define PVT_TS_DONE_IRQ_SRC	 PVT_TS_REG(0xe4)
#define PVT_TS_FAULT_IRQ_SRC	 PVT_TS_REG(0xe8)
#define PVT_TS_ALARM_IRQ_SRC(n)	 PVT_TS_REG(0xec + (n) * 0x4)
#define PVT_TS_DONE_IRQ_TEST	 PVT_TS_REG(0x10c)
#define PVT_TS_FAULT_IRQ_TEST	 PVT_TS_REG(0x110)
#define PVT_TS_ALARM_IRQ_TEST(n) PVT_TS_REG(0x114 + (n) * 0x4)
#define PVT_TS_ALARMA_CFG(n)	 PVT_TS_REG(0x134 + (n) * 0x4)
#define PVT_TS_ALARMB_CFG(n)	 PVT_TS_REG(0x154 + (n) * 0x4)

#define PVT_TS_CLK_SYNTH_LO	GENMASK(7, 0)
#define PVT_TS_CLK_SYNTH_HI	GENMASK(15, 8)
#define PVT_TS_CLK_SYNTH_STROBE GENMASK(19, 16)
#define PVT_TS_CLK_SYNTH_ENA	BIT(24)

#define PVT_TS_SDIF_STATUS_BUSY BIT(0)
#define PVT_TS_SDIF_STATUS_LOCK BIT(1)

#define PVT_TS_SDIF_WDATA GENMASK(23, 0)
#define PVT_TS_SDIF_ADDR  GENMASK(26, 24)
#define PVT_TS_SDIF_WRN	  BIT(27)
#define PVT_TS_SDIF_PROG  BIT(31)

#define PVT_VM_CLK_SYNTH	 PVT_VM_REG(0x0)
#define PVT_VM_SDIF_DISABLE	 PVT_VM_REG(0x4)
#define PVT_VM_SDIF_STATUS	 PVT_VM_REG(0x8)
#define PVT_VM_SDIF_CTRL	 PVT_VM_REG(0xc)
#define PVT_VM_SDIF		 PVT_VM_REG(0x10)
#define PVT_VM_SDIF_RDATA(n)	 PVT_VM_REG(0x14 + (n) * 0x4)
#define PVT_VM_DATA(n)		 PVT_VM_REG(0x34 + (n) * 0x40)
#define PVT_VM_SMPL_CTRL	 PVT_VM_REG(0x234)
#define PVT_VM_SMPL_HI_CLR(n)	 PVT_VM_REG(0x238 + (n) * 0x4)
#define PVT_VM_SMPL_LO_SET(n)	 PVT_VM_REG(0x258 + (n) * 0x4)
#define PVT_VM_SMPL_STATUS	 PVT_VM_REG(0x278)
#define PVT_VM_SMPL_CTR		 PVT_VM_REG(0x27c)
#define PVT_VM_SMPL_HILO(n)	 PVT_VM_REG(0x280 + (n) * 0x40)
#define PVT_VM_IRQ_STATUS	 PVT_VM_REG(0x480)
#define PVT_VM_ALARMA_IRQ_STATUS PVT_VM_REG(0x484)
#define PVT_VM_ALARMB_IRQ_STATUS PVT_VM_REG(0x488)
#define PVT_VM_DONE_IRQ		 PVT_VM_REG(0x48c)
#define PVT_VM_FAULT_IRQ	 PVT_VM_REG(0x490)
#define PVT_VM_ALARM_IRQ(n)	 PVT_VM_REG(0x494 + (n) * 0x4)
#define PVT_VM_DONE_IRQ_ENA	 PVT_VM_REG(0x4b4)
#define PVT_VM_FAULT_IRQ_ENA	 PVT_VM_REG(0x4b8)
#define PVT_VM_ALARM_IRQ_ENA(n)	 PVT_VM_REG(0x4bc + (n) * 0x4)
#define PVT_VM_DONE_IRQ_SRC	 PVT_VM_REG(0x4dc)
#define PVT_VM_FAULT_IRQ_SRC	 PVT_VM_REG(0x4e0)
#define PVT_VM_ALARM_IRQ_SRC(n)	 PVT_VM_REG(0x4e4 + (n) * 0x4)
#define PVT_VM_DONE_IRQ_TEST	 PVT_VM_REG(0x504)
#define PVT_VM_FAULT_IRQ_TEST	 PVT_VM_REG(0x508)
#define PVT_VM_ALARM_IRQ_TEST(n) PVT_VM_REG(0x50c + (n) * 0x4)
#define PVT_VM_ALARMA_CFG(n)	 PVT_VM_REG(0x52c + (n) * 0x40)
#define PVT_VM_ALARMB_CFG(n)	 PVT_VM_REG(0x72c + (n) * 0x40)

#define PVT_VM_CLK_SYNTH_LO	GENMASK(7, 0)
#define PVT_VM_CLK_SYNTH_HI	GENMASK(15, 8)
#define PVT_VM_CLK_SYNTH_STROBE GENMASK(19, 16)
#define PVT_VM_CLK_SYNTH_ENA	BIT(24)

#define PVT_VM_SDIF_STATUS_BUSY BIT(0)
#define PVT_VM_SDIF_STATUS_LOCK BIT(1)

#define PVT_VM_SDIF_WDATA GENMASK(23, 0)
#define PVT_VM_SDIF_ADDR  GENMASK(26, 24)
#define PVT_VM_SDIF_WRN	  BIT(27)
#define PVT_VM_SDIF_PROG  BIT(31)

#define PVT_SDA_IP_CTRL	   0x0
#define PVT_SDA_IP_CFG	   0x1
#define PVT_SDA_IP_CFGA	   0x2
#define PVT_SDA_IP_DATA	   0x3
#define PVT_SDA_IP_POLLING 0x4
#define PVT_SDA_IP_TMR	   0x5

#define PVT_SDA_IP_CTRL_PD	 BIT(0)
#define PVT_SDA_IP_CTRL_RESETN	 BIT(1)
#define PVT_SDA_IP_CTRL_RUN_ONCE BIT(2)
#define PVT_SDA_IP_CTRL_RUN_CONT BIT(3)
#define PVT_SDA_IP_CTRL_CLOAD	 BIT(4)
#define PVT_SDA_IP_CTRL_AUTO	 BIT(8)
#define PVT_SDA_IP_CTRL_STOP	 BIT(9)
#define PVT_SDA_IP_CTRL_VM_MODE	 BIT(10)

#define PVT_SDA_IP_CFG_TVAL    BIT(11)
#define PVT_SDA_IP_CFG_SEL_VIN GENMASK(19, 16)

#define PVT_SDA_IP_DATA_DAT   GENMASK(15, 0)
#define PVT_SDA_IP_DATA_TYPE  BIT(16)
#define PVT_SDA_IP_DATA_FAULT BIT(17)
#define PVT_SDA_IP_DATA_DONE  BIT(18)
#define PVT_SDA_IP_DATA_CH    GENMASK(23, 20)

DECLARE_VERBOSE_FUNCTIONS(pvt)

static void pvt_write_ts(uint32_t addr, uint32_t value)
{
	uint32_t tmp;

	if (poll_timeout(REG(PVT_TS_SDIF_STATUS), tmp, !(tmp & PVT_TS_SDIF_STATUS_BUSY), 10,
			 1000000)) {
		VERBOSE_PRINTF(VERBOSE_LEVEL_ERROR, "%s: bus timeout\n", __func__);
		return;
	}

	REG(PVT_TS_SDIF) = FIELD_PREP(PVT_TS_SDIF_WDATA, value) |
			   FIELD_PREP(PVT_TS_SDIF_ADDR, addr) |
			   PVT_TS_SDIF_WRN |
			   PVT_TS_SDIF_PROG;

	VERBOSE_PRINTF(VERBOSE_LEVEL_DEBUG, "TS_SDIF[%d] = %#x\n", addr, value);
}

static void pvt_write_vm(uint32_t addr, uint32_t value)
{
	uint32_t tmp;

	if (poll_timeout(REG(PVT_VM_SDIF_STATUS), tmp, !(tmp & PVT_VM_SDIF_STATUS_BUSY), 10,
			 1000000)) {
		VERBOSE_PRINTF(VERBOSE_LEVEL_ERROR, "%s: bus timeout\n", __func__);
		return;
	}

	REG(PVT_VM_SDIF) = FIELD_PREP(PVT_VM_SDIF_WDATA, value) |
			   FIELD_PREP(PVT_VM_SDIF_ADDR, addr) |
			   PVT_VM_SDIF_WRN |
			   PVT_VM_SDIF_PROG;

	VERBOSE_PRINTF(VERBOSE_LEVEL_DEBUG, "VM_SDIF[%d] = %#x\n", addr, value);
}

static uint32_t pvt_read_vm(uint32_t addr, uint32_t channel)
{
	uint32_t tmp;
	uint32_t value;

	if (poll_timeout(REG(PVT_VM_SDIF_STATUS), tmp, !(tmp & PVT_VM_SDIF_STATUS_BUSY), 10,
			 1000000)) {
		VERBOSE_PRINTF(VERBOSE_LEVEL_ERROR, "%s: bus timeout\n", __func__);
		return 0;
	}

	REG(PVT_VM_SDIF) = FIELD_PREP(PVT_VM_SDIF_ADDR, addr) | PVT_VM_SDIF_PROG;

	if (poll_timeout(REG(PVT_VM_SDIF_STATUS), tmp, !(tmp & PVT_VM_SDIF_STATUS_BUSY), 10,
			 1000000)) {
		VERBOSE_PRINTF(VERBOSE_LEVEL_ERROR, "%s: bus timeout\n", __func__);
		return 0;
	}

	value = REG(PVT_VM_SDIF_RDATA(channel));
	VERBOSE_PRINTF(VERBOSE_LEVEL_DEBUG, "VM_SDIF[%d][%d] is %#x\n", addr, channel, value);

	return value;
}

uint32_t pvt_ts_measure_mcelsius(uint32_t channel)
{
	uint32_t val;

	REG(PVT_TS_DATA(channel));
	pvt_write_ts(PVT_SDA_IP_CTRL,
		     PVT_SDA_IP_CTRL_RUN_ONCE |
			     PVT_SDA_IP_CTRL_AUTO);

	if (poll_timeout(REG(PVT_TS_SMPL_STATUS), val, val & BIT(channel), 10, 1000000)) {
		VERBOSE_PRINTF(VERBOSE_LEVEL_ERROR,
			       "pvt_ts_measure_celsius: read timeout (channel: %d, val: %#x)\n",
			       channel, val);
		return 0;
	}

	val = REG(PVT_TS_DATA(channel));
	if (val & (PVT_SDA_IP_DATA_TYPE | PVT_SDA_IP_DATA_FAULT)) {
		VERBOSE_PRINTF(VERBOSE_LEVEL_ERROR,
			       "pvt_ts_measure_celsius: read fault (channel: %d, val: %#x)\n",
			       channel, val);
		return 0;
	}

	VERBOSE_PRINTF(VERBOSE_LEVEL_INFO, "RAW value: %#x\n", val);
	val &= PVT_SDA_IP_DATA_DAT;

	return val * 237500 / 4094 - 81100;
}

uint32_t pvt_vm_measure_mv(uint32_t channel)
{
	uint32_t val = 0; // to suppress false positive warning from clang-format

	pvt_write_vm(PVT_SDA_IP_CTRL, PVT_SDA_IP_CTRL_RESETN | PVT_SDA_IP_CTRL_RUN_CONT);

	if (poll_timeout(pvt_read_vm(PVT_SDA_IP_DATA, channel), val, val & PVT_SDA_IP_DATA_DONE, 10,
			 1000000)) {
		VERBOSE_PRINTF(VERBOSE_LEVEL_ERROR,
			       "pvt_vm_measure_mv: read timeout (channel: %d, val: %#x)\n",
			       channel, val);
		return 0;
	}

	VERBOSE_PRINTF(VERBOSE_LEVEL_INFO, "RAW value: %#x\n", val);

	if (val & (PVT_SDA_IP_DATA_TYPE | PVT_SDA_IP_DATA_FAULT)) {
		VERBOSE_PRINTF(VERBOSE_LEVEL_ERROR,
			       "pvt_vm_measure_mv: read fault (channel: %d, val: %#x)\n",
			       channel, val);
		return 0;
	}

	val &= PVT_SDA_IP_DATA_DAT;

	return (val + 1) * 1224 / 256;
}

void pvt_vm_set_tval(bool enable)
{
	uint32_t value = pvt_read_vm(PVT_SDA_IP_CFG, 0);

	if (enable)
		value |= PVT_SDA_IP_CFG_TVAL;
	else
		value &= ~PVT_SDA_IP_CFG_TVAL;

	pvt_write_vm(PVT_SDA_IP_CFG, value);
}

void pvt_vm_set_sel_vin(uint32_t vin)
{
	uint32_t value = pvt_read_vm(PVT_SDA_IP_CFG, 0);

	value &= ~PVT_SDA_IP_CFG_SEL_VIN;
	value |= FIELD_PREP(PVT_SDA_IP_CFG_SEL_VIN, vin);
	pvt_write_vm(PVT_SDA_IP_CFG, value);
}

void pvt_init(void)
{
	const uint32_t target_vm_freq = 1000000;
	uint32_t pvt_div;
	uint32_t ucg_div = (REG(SERVICE_UCG + 8 * 4) >> 10) & 0xfffff;
	unsigned long rate = clk_pll_calc_freq(REG(SERVICE_URB_PLL), XTI_FREQUENCY);
	uint32_t nts = 0; // temperature sensors count
	uint32_t nvm = 0; // voltage monitors count

	VERBOSE_PRINTF(VERBOSE_LEVEL_DEBUG, "PLL freq: %d Hz\n", rate);
	if (!ucg_div)
		ucg_div = 1;

	rate /= ucg_div;
	VERBOSE_PRINTF(VERBOSE_LEVEL_DEBUG, "UCG PVT freq: %d Hz\n", rate);
	pvt_div = DIV_ROUND_UP(rate, (2 * target_vm_freq));
	if (!pvt_div)
		pvt_div = 1;

	if (pvt_div > 256) {
		VERBOSE_PRINTF(VERBOSE_LEVEL_ERROR,
			       "Failed to setup clock rate for voltage sensor (div: %d)\n",
			       pvt_div);
		return;
	}
	// 	ip_clock_period = 1000000 / (rate / pvt_div / 2);
	VERBOSE_PRINTF(VERBOSE_LEVEL_DEBUG, "PVT div: %d, IP freq: %d\n", pvt_div, rate / pvt_div / 2);

	// Temperature
	do {
		REG(PVT_TS_SDIF_DISABLE) = BIT(nts);
	} while (REG(PVT_TS_SDIF_DISABLE) == BIT(nts++));
	nts -= 1;
	VERBOSE_PRINTF(VERBOSE_LEVEL_INFO, "Found %d temperature sensors\n", nts);

	REG(PVT_TS_SDIF_CTRL) = 0;
	REG(PVT_TS_CLK_SYNTH) = FIELD_PREP(PVT_TS_CLK_SYNTH_LO, pvt_div - 1) |
				FIELD_PREP(PVT_TS_CLK_SYNTH_HI, pvt_div - 1) |
				FIELD_PREP(PVT_TS_CLK_SYNTH_STROBE, 1) |
				PVT_TS_CLK_SYNTH_ENA;

	pvt_write_ts(PVT_SDA_IP_TMR, DIV_ROUND_UP(50 * rate, 1000000));

	// Voltage monitor
	do {
		REG(PVT_VM_SDIF_DISABLE) = BIT(nvm);
	} while (REG(PVT_VM_SDIF_DISABLE) == BIT(nvm++));
	nvm -= 1;
	VERBOSE_PRINTF(VERBOSE_LEVEL_INFO, "Found %d voltage monitors\n", nvm);

	REG(PVT_VM_SDIF_CTRL) = 0;
	REG(PVT_VM_CLK_SYNTH) = FIELD_PREP(PVT_VM_CLK_SYNTH_LO, pvt_div - 1) |
				FIELD_PREP(PVT_VM_CLK_SYNTH_HI, pvt_div - 1) |
				FIELD_PREP(PVT_VM_CLK_SYNTH_STROBE, 1) |
				PVT_VM_CLK_SYNTH_ENA;

	pvt_write_vm(PVT_SDA_IP_TMR, DIV_ROUND_UP(10 * target_vm_freq, 1000000));

	pvt_write_vm(PVT_SDA_IP_CTRL, 0); // power-down deassert
	mdelay(1);
	pvt_write_vm(PVT_SDA_IP_CTRL, PVT_SDA_IP_CTRL_RESETN); // reset deassert
	mdelay(1);
}
