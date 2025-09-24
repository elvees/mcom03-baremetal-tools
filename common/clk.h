// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#ifndef CLK_H_
#define CLK_H_

#include <stdint.h>

#include <regs.h>

struct ucg {
	volatile uint32_t CTR[16];
	volatile uint32_t BP;
	volatile uint32_t SYNC;
};

#define UCG_CPU_UCG0	      ((struct ucg *)(TO_VIRT(CPU_UCG)))
#define UCG_SERVICE_UCG0      ((struct ucg *)(TO_VIRT(SERVICE_UCG)))
#define UCG_LSP0_UCG0	      ((struct ucg *)(TO_VIRT(LSP0_UCG2)))
#define UCG_LSP1_UCG0	      ((struct ucg *)(TO_VIRT(LSP1_UCG)))
#define UCG_LSP1_UCG_I2S      ((struct ucg *)(TO_VIRT(LSP1_UCG_I2S)))
#define UCG_HSP_UCG0	      ((struct ucg *)(TO_VIRT(HSP_UCG(0))))
#define UCG_HSP_UCG1	      ((struct ucg *)(TO_VIRT(HSP_UCG(1))))
#define UCG_HSP_UCG2	      ((struct ucg *)(TO_VIRT(HSP_UCG(2))))
#define UCG_HSP_UCG3	      ((struct ucg *)(TO_VIRT(HSP_UCG(3))))
#define UCG_SDR_UCG0	      ((struct ucg *)(TO_VIRT(SDR_UCG0)))
#define UCG_SDR_UCG_PCIE0_REF ((struct ucg *)(TO_VIRT(SDR_UCG_PCIE0_REF)))
#define UCG_SDR_UCG_PCIE1_REF ((struct ucg *)(TO_VIRT(SDR_UCG_PCIE1_REF)))
#define UCG_SDR_UCG_N_DFE     ((struct ucg *)(TO_VIRT(SDR_UCG_N_DFE)))
#define UCG_MEDIA_UCG0	      ((struct ucg *)(TO_VIRT(MEDIA_UCG0)))
#define UCG_MEDIA_UCG1	      ((struct ucg *)(TO_VIRT(MEDIA_UCG1)))
#define UCG_MEDIA_UCG2	      ((struct ucg *)(TO_VIRT(MEDIA_UCG2)))
#define UCG_MEDIA_UCG3	      ((struct ucg *)(TO_VIRT(MEDIA_UCG3)))
#define UCG_IC_UCG0	      ((struct ucg *)(TO_VIRT(IC_UCG0)))
#define UCG_IC_UCG1	      ((struct ucg *)(TO_VIRT(IC_UCG1)))
#define UCG_DDR_UCG0	      ((struct ucg *)(TO_VIRT(DDR_UCG0)))
#define UCG_DDR_UCG1	      ((struct ucg *)(TO_VIRT(DDR_UCG1)))

#define UCG_LPI_EN	   BIT(0)
#define UCG_CLK_EN	   BIT(1)
#define UCG_CLK_EN_STS	   GENMASK(4, 2)
#define UCG_QACTIVE_CTL_EN BIT(6)
#define UCG_Q_FSM_STATE	   GENMASK(9, 7)
#define UCG_DIV_COEFF	   GENMASK(29, 10)
#define UCG_DIV_LOCK	   BIT(30)

#define ucg_chan_is_enabled(ucg, chan) (!!((ucg)->CTR[chan] & UCG_CLK_EN))

#define ucg_chan_get_div(ucg, chan) (((ucg)->CTR[chan] >> 10) & 0xfffff)

#define ucg_chan_set_div(ucg, chan, div) \
	(ucg)->CTR[chan] = ((ucg)->CTR[chan] & UCG_CLK_EN) | (((div) & 0xfffff) << 10)

#define ucg_chan_set_div_and_enable(ucg, chan, div, enable) \
	(ucg)->CTR[chan] = (((div) & 0xfffff) << 10) | ((!!(enable)) << 1)

#define ucg_chan_enable(ucg, chan, enable)                              \
	(ucg)->CTR[chan] = (enable) ? ((ucg)->CTR[chan] | UCG_CLK_EN) : \
				      ((ucg)->CTR[chan] & ~UCG_CLK_EN)

#define PLL_LOCK BIT(31)
#define PLL_NR	 GENMASK(30, 27)
#define PLL_NF	 GENMASK(26, 14)
#define PLL_OD	 GENMASK(13, 10)
#define PLL_MAN	 BIT(9)
#define PLL_SEL	 GENMASK(7, 0)
#define set_pll_man(pll_addr, nr, nf, od)                                             \
	REG(pll_addr) = FIELD_PREP(PLL_NR, (nr) - 1) | FIELD_PREP(PLL_NF, (nf) - 1) | \
			FIELD_PREP(PLL_OD, (od) - 1) | PLL_MAN | FIELD_PREP(PLL_SEL, 1)

unsigned long clk_pll_calc_freq(uint32_t pll_value, uint32_t xti_freq);
int clk_pll_set_value(uintptr_t pll, uint32_t value);
void clk_ucg_bypass_enabled_channels(struct ucg *ucg, int count);
int clk_ucg_set_div(struct ucg *ucg, unsigned int channel, uint32_t div);
int clk_ucg_setup(struct ucg *ucg, const uint32_t *divs, int count, uint32_t sync_mask,
		  uintptr_t pll, uint32_t pll_value);

#endif
