// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#include <stdbool.h>
#include <stdint.h>

#include <clk.h>
#include <delay.h>
#include <regs.h>

/* Calculate frequency for PLL value.
 * pll_value - value from PLL register.
 * xti_freq - PLL input frequency.
 * Return PLL output frequency. If xti_freq in Hz then return value in Hz.
 * If xti_freq in kHz then return value in kHz.
 */
unsigned long clk_pll_calc_freq(uint32_t pll_value, uint32_t xti_freq)
{
	if (pll_value & PLL_MAN) {
		uint32_t nr = FIELD_GET(PLL_NR, pll_value) + 1;
		uint32_t nf = FIELD_GET(PLL_NF, pll_value) + 1;
		uint32_t od = FIELD_GET(PLL_OD, pll_value) + 1;
		return (xti_freq / nr) * nf / od;
	} else {
		return xti_freq * (FIELD_GET(PLL_SEL, pll_value) + 1);
	}
}

/* Set value to PLL register and wait for lock.
 * pll - address of PLL register.
 * value - value to write into register.
 * Return 0 on success or -1 if LOCK bit didn't set.
 */
int clk_pll_set_value(uintptr_t pll, uint32_t value)
{
	REG(pll) = value;

	return poll_read32_mask_timeout(pll, PLL_LOCK, PLL_LOCK, 100000);
}

static int _clk_ucg_set_div_no_bypass(struct ucg *ucg, unsigned int channel, uint32_t div,
				      bool is_enabled)
{
	uint32_t val = is_enabled ? UCG_CLK_EN : 0;

	ucg->CTR[channel] = FIELD_PREP(UCG_DIV_COEFF, div) | val;

	return poll_read32_mask_timeout(TO_PHYS((uintptr_t)&ucg->CTR[channel]), UCG_DIV_LOCK,
					UCG_DIV_LOCK, 100000);
}

/* Setup divider for UCG channel. Don't affects to channel enable/disable.
 * channel - number of UCG channel.
 * div - divider to set.
 * Return 0 on success or -1 if LOCK bit didn't set.
 */
int clk_ucg_set_div(struct ucg *ucg, unsigned int channel, uint32_t div)
{
	int ret;
	bool is_enabled = ucg_chan_is_enabled(ucg, channel);

	if (is_enabled)
		ucg->BP |= BIT(channel);

	ret = _clk_ucg_set_div_no_bypass(ucg, channel, div, is_enabled);
	if (is_enabled)
		ucg->BP &= ~BIT(channel);

	return ret;
}

/* Set PLL (optionally) and modify dividers of all channels in UCG. Don't affects to channel
 * enable/disable. Also can be set SYNC.
 * divs - array of dividers. If zero then this channel will not be modified.
 * count - count of elements in divs array.
 * sync_mask - mask synchronize UCG channels.
 * pll - if non-zero then address of PLL to setup.
 * pll_value - if pll is non-zero then value to write into PLL register.
 * Return 0 on success or -1 if LOCK bit for PLL or any UCG channel didn't set.
 */
int clk_ucg_setup(struct ucg *ucg, const uint32_t *divs, int count, uint32_t sync_mask,
		  uintptr_t pll, uint32_t pll_value)
{
	uint32_t bp = 0;
	int ret = 0;

	for (int i = 0; i < count; i++) {
		if (divs[i] && ucg_chan_is_enabled(ucg, i)) {
			bp |= BIT(i);
		}
	}

	ucg->BP = bp;
	if (pll)
		ret = clk_pll_set_value(pll, pll_value);

	for (int i = 0; i < count; i++) {
		if (divs[i]) {
			if (_clk_ucg_set_div_no_bypass(ucg, i, divs[i], bp & BIT(i)))
				ret = -1;
		}
	}

	if (sync_mask)
		ucg->SYNC = sync_mask;

	ucg->BP = 0;

	return ret;
}
