// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#include <stdint.h>
#include <stddef.h>

#include <clk.h>
#include <delay.h>
#include <gpio.h>
#include <pvt.h>
#include <regs.h>
#include <uart.h>

/*
 * Turn A into a string literal without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded).
 */
#define STRINGIZE_NX(A) #A

/* Turn A into a string literal after macro-expanding it. */
#define STRINGIZE(A) STRINGIZE_NX(A)

#define TEST_GO	   2
#define TEST_ERROR 3
#define LOG0	   4
#define LOG1	   5

#define ARM_CPU_ADDR 0x8000

#define DEGREE_CELSIUS_UTF8_SYM "\xe2\x84\x83"

/* ARM CPU program can be linked from binary, but this requires to add project, compile it, write
 * link script to add binary from new project to this binary. We add simple ARM program to
 * array in .data section instead. This is easiest way.
 * User manual is not contains alignment requirements for ARM CPU start address. Alignment 0x100 is
 * tested and works.
 */
const uint32_t arm_cpu_prog[] = {
	// _start:
	0xd53800a1, // mrs     x1, mpidr_el1
	0x92401c21, // and     x1, x1, #0xff  // now x1 contains core id
	0xd37df021, // lsl     x1, x1, #3     // x1 = x1 << 3
	0x10ffffa2, // adr     x2, _start     // x2 = absolute address of start of program
	0x8b020021, // add     x1, x1, x2     // x1 = x1 + x2
	0x91040021, // add     x1, x1, #0x100 // x1 = x1 + 0x100
	0xd2800002, // mov     x2, #0x0
	// now x1 is pointer to counter in memory
	// _loop:
	0xd2a00083, // movz    x3, #4, LSL #16   // x3 = 4 << 16 = 262144
	// _loop2:
	0xd1000463, // sub     x3, x3, #0x1   // x3 = x3 - 1
	0xb5ffffe3, // cbnz    x3, _loop2
	0x91000442, // add     x2, x2, #0x1
	0xf9000022, // str     x2, [x1]
	0xd503201f, // nop
	0x17fffffa, // b       _loop
};

const uint32_t risc1_prog[] = {
	0x34048180, // li      a0,0x8180
	// _loop:
	0x3c060050, // lui     a2,0x50   // a2 = 0x500000 = 5242880
	// _loop2:
	0x24c6ffff, // addiu   a2,a2,-1
	0x14c0fffe, // bnez    a2,_loop2
	0x00000000, // nop
	0x24a50001, // addiu   a1,a1,1
	0xac850000, // sw      a1,0(a0)
	0x1000fff9, // b       _loop
	0x00000000, // nop
};

static void hang(void)
{
	while (1) {
	}
}

static uint32_t get_pll_freq_khz(uintptr_t addr)
{
	return clk_pll_calc_freq(REG(addr), XTI_FREQUENCY / 1000);
}

static int setup_pll(uintptr_t addr, uint32_t value, const char *name)
{
	uint32_t val;
	int ret = 0;

	if (name)
		uart_printf(UART0, "%s", name);

	REG(addr) = value;

	// if SEL=0 then PLL turn in bypass mode and LOCK bit will not set
	if ((value & 0xff))
		ret = poll_timeout(REG(addr), val, val & BIT(31), 0, 1000000);

	if (name)
		uart_printf(UART0, ret ? " LOCK bit timeout\n" : ": %d kHz\n",
			    get_pll_freq_khz(addr));

	return ret;
}

static int setup_ucg(struct ucg *ucg, int count, const int32_t *divs,
		     const char **names, uintptr_t pll_addr, const char *ucg_name)
{
	uint32_t pll_freq_khz = 0;

	uart_printf(UART0, " %s\n", ucg_name);
	if (pll_addr && names)
		pll_freq_khz = get_pll_freq_khz(pll_addr);

	for (int i = 0; i < count; i++) {
		uint32_t val;
		int ret;

		if (divs[i] > 0)
			ucg_chan_set_div_and_enable(ucg, i, divs[i], 1);

		if (pll_addr && names && names[i]) {
			uint32_t freq;
			uint32_t div = ucg_chan_get_div(ucg, i);

			if (!div)
				div = 1;

			freq = pll_freq_khz / div;
			uart_printf(UART0, "  ch %2d %7d kHz  %s\n", i, freq, names[i]);
		}

		if (divs[i] < 0)
			continue;

		ret = poll_timeout(ucg->CTR[i], val, val & BIT(30), 0, 100000);
		if (ret) {
			uart_printf(UART0, "UCG channel %d lock timeout\n", i);
			return ret;
		}
	}

	return 0;
}

static int ppolicy_set(uintptr_t addr, uint8_t new_value)
{
	uint32_t val;
	int ret;

	REG(addr) = new_value;

	ret = poll_timeout(REG(addr + 0x4), val, (val & 0x1f) == new_value, 100, 5000000);
	if (ret)
		uart_printf(UART0, "Failed to set %#x PPOLICY with address %#x\n", new_value, addr);

	return ret;
}

static int ppolicy_on(uintptr_t addr)
{
	return ppolicy_set(addr, PP_ON);
}

static int setup_subs_ic(void)
{
	const int32_t divs0[] = {
		DIV_ROUND_UP(1188, 200),
		DIV_ROUND_UP(1188, 300),
		DIV_ROUND_UP(1188, 300),
		DIV_ROUND_UP(1188, 200),
		DIV_ROUND_UP(1188, 600),
		DIV_ROUND_UP(1188, 300),
		DIV_ROUND_UP(1188, 100),
		DIV_ROUND_UP(1188, 600),
	};
	const int32_t divs1[] = {
		DIV_ROUND_UP(1188, 40),
		-1,
		DIV_ROUND_UP(1188, 150),
		-1,
		DIV_ROUND_UP(1188, 600),
		DIV_ROUND_UP(1188, 300),
		DIV_ROUND_UP(1188, 100),
		DIV_ROUND_UP(1188, 150),
		DIV_ROUND_UP(1188, 200),
	};
	const char *names0[] = {
		"ddr_dp",
		"ddr_vpu",
		"ddr_gpu",
		"ddr_isp",
		"ddr_cpu",
		"cpu_acp",
		"ddr_lsperiph0",
		"axi_coh_comm",
	};
	const char *names1[] = {
		"axi_slow_comm",
		NULL,
		"axi_fast_comm",
		NULL,
		"ddr_sdr_dsp",
		"ddr_sdr_pcie",
		"ddr_lsperiph1",
		"ddr_service",
		"ddr_hsperiph",
	};

	if (UCG_IC_UCG0->BP != 0xff || UCG_IC_UCG1->BP != 0x1ff) {
		UCG_IC_UCG0->BP = 0xff;
		UCG_IC_UCG1->BP = 0x1ff;
	}

	if (setup_pll(IC_URB_PLL, 43, "IC_PLL")) // 1188 MHz
		return -1;

	if (setup_ucg(UCG_IC_UCG0, ARRAY_LENGTH(divs0), divs0, names0, IC_URB_PLL, "UCG0"))
		return -1;

	if (setup_ucg(UCG_IC_UCG1, ARRAY_LENGTH(divs1), divs1, names1, IC_URB_PLL, "UCG1"))
		return -1;

	UCG_IC_UCG0->BP = 0;
	UCG_IC_UCG1->BP = 0;
	uart_puts(UART0, "\n");

	return 0;
}

#if defined(USE_CPU)
static int setup_subs_cpu(void)
{
	const int32_t divs[] = { 4, 1, 2 };
	const char *names[] = {
		"clk_sys",
		"clk_core",
		"clk_dbus",
	};

	if (ppolicy_on(CPU_SUBS_PPOLICY))
		return -1;

	UCG_CPU_UCG0->BP = 0x7;
	if (setup_pll(CPU_URB_PLL, 42, "CPU_PLL")) // 1161 MHz
		return -1;

	if (setup_ucg(UCG_CPU_UCG0, ARRAY_LENGTH(divs), divs, names, CPU_URB_PLL, "UCG"))
		return -1;

	UCG_CPU_UCG0->SYNC = 0x7;
	UCG_CPU_UCG0->BP = 0;
	uart_puts(UART0, "\n");

	return 0;
}
#else
static int setup_subs_cpu(void)
{
	return 0;
}
#endif

static int setup_subs_service(void)
{
	const int32_t apb = DIV_ROUND_UP(594, 100);
	const int32_t core = DIV_ROUND_UP(594, 600);
	const int32_t divs[] = {
		apb,
		core,
		core,
		core,
		core,
		apb,
		apb,
		apb,
		apb,
		apb,
		apb,
		apb,

		DIV_ROUND_UP(594, 100),
		DIV_ROUND_UP(594, 27),
		DIV_ROUND_UP(594, 100),
		DIV_ROUND_UP(594, 50),
	};
	const char *names[] = {
		"clk_apb",
		"clk_core",
		"clk_qspi0",
		"clk_bpam",
		"clk_risc0",
		"clk_mfbsp0",
		"clk_mfbsp1",
		"clk_mailbox0",
		"clk_pvtctr",
		"clk_i2c4",
		"clk_trng",
		"clk_spiotp",
		"clk_i2c4_ext",
		"clk_qspi0_ext",
		"clkout_ext",
		"risc0_tck_ucg",
	};

	set_tick_freq(XTI_FREQUENCY);
	UCG_SERVICE_UCG0->BP = 0xffff;
	if (setup_pll(SERVICE_URB_PLL, 21, "SERVICE_PLL")) // 594 MHz
		return -1;

	if (setup_ucg(UCG_SERVICE_UCG0, ARRAY_LENGTH(divs), divs, names, SERVICE_URB_PLL, "UCG"))
		return -1;

	UCG_SERVICE_UCG0->SYNC = 0xffff;
	UCG_SERVICE_UCG0->BP = 0;
	set_tick_freq(594000000);
	uart_puts(UART0, "\n");

	return 0;
}

#if defined(USE_SDR)
static int setup_subs_sdr(void)
{
	const int32_t divs0[] = {
		DIV_ROUND_UP(1188, 200),
		DIV_ROUND_UP(1188, 600),
		DIV_ROUND_UP(1188, 400),
		DIV_ROUND_UP(1188, 200),
		DIV_ROUND_UP(1188, 500),
		DIV_ROUND_UP(1188, 600),
		DIV_ROUND_UP(1188, 600),
		DIV_ROUND_UP(1188, 600),
		DIV_ROUND_UP(1188, 50),
		DIV_ROUND_UP(1188, 400),
		DIV_ROUND_UP(1188, 600),
		DIV_ROUND_UP(1188, 200),
		DIV_ROUND_UP(1188, 100),
	};
	const char *names0[] = {
		"clk_cfg",
		"ext_aclk",
		"bbd_aclk",
		"pci_aclk",
		"vcu_aclk",
		"acc0_clk",
		"acc1_clk",
		"acc2_clk",
		"aux_pci_clk",
		"gnss_clk",
		"dfe_a_clk",
		"vcu_tck",
		"lvds_clk",
	};
	const int32_t divs_pcie0_ref[] = {
		DIV_ROUND_UP(1188, 100),
	};
	const int32_t divs_pcie1_ref[] = {
		DIV_ROUND_UP(1188, 100),
	};
	const char *names_pcie0_ref[] = {
		"pci_ref_alt_clk",
	};
	const int32_t divs_n_dfe[] = {
		DIV_ROUND_UP(1188, 600),
	};
	const char *names_n_dfe[] = {
		"dfe_n",
	};

	if (ppolicy_on(SDR_SUBS_PPOLICY))
		return -1;

	UCG_SDR_UCG0->BP = 0xffff;
	if (setup_pll(SDR_URB_PLL0, 43, "SDR_PLL0")) // 1188 MHz
		return -1;

	if (setup_ucg(UCG_SDR_UCG0, ARRAY_LENGTH(divs0), divs0, names0, SDR_URB_PLL0, "UCG0"))
		return -1;

	UCG_SDR_UCG0->BP = 0;
	REG(SDR_URB + 0x20) = 0x100; // UCG PCIe0_REF reset deassert
	REG(SDR_URB + 0x30) = 0x100; // UCG PCIe1_REF reset deassert
	REG(SDR_URB + 0x40) = 0x100; // UCG N_DFE reset deassert
	if (setup_ucg(UCG_SDR_UCG_PCIE0_REF, ARRAY_LENGTH(divs_pcie0_ref), divs_pcie0_ref,
		      names_pcie0_ref, SDR_URB_PLL0, "UCG_PCIE0_REF"))
		return -1;

	if (setup_ucg(UCG_SDR_UCG_PCIE1_REF, ARRAY_LENGTH(divs_pcie1_ref), divs_pcie1_ref,
		      names_pcie0_ref, SDR_URB_PLL0, "UCG_PCIE1_REF"))
		return -1;

	if (setup_pll(SDR_URB_PLL1, 23, "SDR_PLL1")) // 648 MHz
		return -1;

	if (setup_ucg(UCG_SDR_UCG_N_DFE, ARRAY_LENGTH(divs_n_dfe), divs_n_dfe,
		      names_n_dfe, SDR_URB_PLL1, "UCG_N_DFE"))
		return -1;

	if (setup_pll(SDR_URB_PLL2, 14, "SDR_PLL2")) // 405 MHz
		return -1;

	REG(SDR_URB + 0x4c) = 0x300; // enable DSP clock
	REG(SDR_URB + 0x50) = 0x93; // enable PCIe0 clock
	REG(SDR_URB + 0x54) = 0x93; // enable PCIe1 clock

	REG(SDR_URB + 0x78) = 0x7; // deassert ACC resets
	REG(SDR_URB + 0x7c) = 0x3; // deassert GNSS resets
	REG(SDR_URB + 0x80) = 0x1; // deassert A_DFE reset
	REG(SDR_URB + 0x84) = 0x3; // deassert JESD0 resets
	REG(SDR_URB + 0x88) = 0x3; // deassert JESD1 resets
	REG(SDR_URB + 0x8c) = 0x1; // deassert N_DFE reset
	REG(SDR_URB + 0x90) = 0x1; // deassert EXT reset
	REG(SDR_URB + 0x94) = 0x1; // deassert BBD reset
	REG(SDR_URB + 0x98) = 0x1; // deassert PCI reset
	if (ppolicy_on(SDR_URB + 0x60)) // enable DSP0
		return -1;

	if (ppolicy_on(SDR_URB + 0x68)) // enable DSP1
		return -1;

	if (ppolicy_on(SDR_URB + 0x70)) // enable RISC1
		return -1;

	uart_puts(UART0, "\n");

	return 0;
}
#else
static int setup_subs_sdr(void)
{
	return 0;
}
#endif

#if defined(USE_MEDIA)
static int setup_subs_media(void)
{
	const int32_t divs0[] = {
		DIV_ROUND_UP(1998, 250),
		DIV_ROUND_UP(1998, 380),
	};
	const char *names0[] = {
		"sys_aclk",
		"isp_sys_clk",
	};
	const int32_t divs1[] = {
		DIV_ROUND_UP(594, 460),
		DIV_ROUND_UP(594, 300),
		DIV_ROUND_UP(594, 300),
	};
	const char *names1[] = {
		"disp_aclk",
		"disp_mclk",
		"disp_pxlclk",
	};
	const int32_t divs2[] = {
		DIV_ROUND_UP(2592, 520),
		DIV_ROUND_UP(2592, 500),
		DIV_ROUND_UP(2592, 500),
	};
	const char *names2[] = {
		"gpu_sys_clk",
		"gpu_mem_clk",
		"gpu_core_clk",
	};
	const int32_t divs3[] = {
		DIV_ROUND_UP(2376, 27),
		DIV_ROUND_UP(2376, 56),
		DIV_ROUND_UP(2376, 56),
		DIV_ROUND_UP(2376, 27),
		DIV_ROUND_UP(2376, 56),
		DIV_ROUND_UP(2376, 150),
		DIV_ROUND_UP(2376, 150),
		DIV_ROUND_UP(2376, 20),
		DIV_ROUND_UP(2376, 570),
	};
	const char *names3[] = {
		"mipi_rx_refclk",
		"mipi_rx0_cfg_clk",
		"mipi_rx1_cfg_clk",
		"mipi_tx_refclk",
		"mipi_tx_cfg_clk",
		"cmos0_clk",
		"cmos1_clk",
		"mipi_txclkesc",
		"vpu_clk",
	};

	if (ppolicy_on(MEDIA_SUBS_PPOLICY))
		return -1;

	UCG_MEDIA_UCG0->BP = 0x3;
	if (setup_pll(MEDIA_URB_PLL0, 73, "MEDIA_PLL0")) // 1998 MHz
		return -1;

	if (setup_ucg(UCG_MEDIA_UCG0, ARRAY_LENGTH(divs0), divs0, names0, MEDIA_URB_PLL0, "UCG0"))
		return -1;

	UCG_MEDIA_UCG0->BP = 0;

	UCG_MEDIA_UCG1->BP = 0x7;
	if (setup_pll(MEDIA_URB_PLL1, 21, "MEDIA_PLL1")) // 594 MHz
		return -1;

	if (setup_ucg(UCG_MEDIA_UCG1, ARRAY_LENGTH(divs1), divs1, names1, MEDIA_URB_PLL1, "UCG1"))
		return -1;

	UCG_MEDIA_UCG1->BP = 0;

	UCG_MEDIA_UCG2->BP = 0x7;
	if (setup_pll(MEDIA_URB_PLL2, 95, "MEDIA_PLL2")) // 2592 MHz
		return -1;

	if (setup_ucg(UCG_MEDIA_UCG2, ARRAY_LENGTH(divs2), divs2, names2, MEDIA_URB_PLL2, "UCG2"))
		return -1;

	UCG_MEDIA_UCG2->BP = 0;

	UCG_MEDIA_UCG3->BP = 0x1ff;
	if (setup_pll(MEDIA_URB_PLL3, 87, "MEDIA_PLL3")) // 2376 MHz
		return -1;

	if (setup_ucg(UCG_MEDIA_UCG3, ARRAY_LENGTH(divs3), divs3, names3, MEDIA_URB_PLL3, "UCG3"))
		return -1;

	UCG_MEDIA_UCG3->BP = 0;

	if (ppolicy_on(MEDIA_URB_ISP_PPOLICY))
		return -1;

	if (ppolicy_on(MEDIA_URB_GPU_PPOLICY))
		return -1;

	if (ppolicy_on(MEDIA_URB_VPU_PPOLICY))
		return -1;

	if (ppolicy_on(MEDIA_URB_DISPLAY_PPOLICY))
		return -1;

	uart_puts(UART0, "\n");

	return 0;
}
#else
static int setup_subs_media(void)
{
	return 0;
}
#endif

#if defined(USE_DDR)
static int setup_subs_ddr(void)
{
	const int32_t divs0[] = {
		DIV_ROUND_UP(783, 800),
		-1,
		DIV_ROUND_UP(783, 800),
	};
	const char *names0[] = {
		"ddr0_clk",
		NULL,
		"ddr1_clk",
	};
	const int32_t divs1[] = {
		DIV_ROUND_UP(1188, 100),
		DIV_ROUND_UP(1188, 600),
		DIV_ROUND_UP(1188, 300),
		DIV_ROUND_UP(1188, 200),
		DIV_ROUND_UP(1188, 300),
		DIV_ROUND_UP(1188, 300),
		DIV_ROUND_UP(1188, 200),
		DIV_ROUND_UP(1188, 600),
		DIV_ROUND_UP(1188, 150),
		DIV_ROUND_UP(1188, 200),
		DIV_ROUND_UP(1188, 100),
		DIV_ROUND_UP(1188, 100),
	};
	const char *names1[] = {
		"sys_clk",
		"sdr_clk",
		"pcie_clk",
		"isp_clk",
		"gpu_clk",
		"vpu_clk",
		"dp_clk",
		"cpu_clk",
		"service_clk",
		"hsperiph_clk",
		"lsperiph0_clk",
		"lsperiph1_clk",
	};

	if (ppolicy_on(DDR_SUBS_PPOLICY))
		return -1;

	UCG_DDR_UCG0->BP = 0xf;
	UCG_DDR_UCG1->BP = 0xfff;
	if (setup_pll(DDR_URB_PLL0, 28, "DDR_PLL0")) // 783 MHz
		return -1;

	if (setup_ucg(UCG_DDR_UCG0, ARRAY_LENGTH(divs0), divs0, names0, DDR_URB_PLL0, "UCG0"))
		return -1;

	if (setup_pll(DDR_URB_PLL1, 43, "DDR_PLL1")) // 1188 MHz
		return -1;

	if (setup_ucg(UCG_DDR_UCG1, ARRAY_LENGTH(divs1), divs1, names1, DDR_URB_PLL1, "UCG1"))
		return -1;

	UCG_DDR_UCG0->BP = 0;
	UCG_DDR_UCG1->BP = 0;
	uart_puts(UART0, "\n");

	return 0;
}
#else
static int setup_subs_ddr(void)
{
	return 0;
}
#endif

#if defined(USE_HSP)
static int setup_subs_hsperiph(void)
{
	const int32_t divs0[] = {
		DIV_ROUND_UP(1125, 225),
		DIV_ROUND_UP(1125, 225),
		DIV_ROUND_UP(1125, 94),
		DIV_ROUND_UP(1125, 225),
		DIV_ROUND_UP(1125, 125),
		DIV_ROUND_UP(1125, 125),
		DIV_ROUND_UP(1125, 125),
		DIV_ROUND_UP(1125, 125),
		DIV_ROUND_UP(1125, 125),
		DIV_ROUND_UP(1125, 225),
		DIV_ROUND_UP(1125, 225),
		DIV_ROUND_UP(1125, 225),
		DIV_ROUND_UP(1125, 125),
	};
	const char *names0[] = {
		"sys_clk",
		"dma_clk",
		"ctr_clk",
		"spram0_clk",
		"emac0_clk",
		"emac1_clk",
		"usb0_clk",
		"usb1_clk",
		"nfc_clk",
		"pdma2_clk",
		"sdmmc0_clk",
		"sdmmc1_clk",
		"qspi_clk",
	};
	const int32_t divs1[] = {
		DIV_ROUND_UP(1125, 250),
		DIV_ROUND_UP(1125, 250),
		DIV_ROUND_UP(1125, 250),
		DIV_ROUND_UP(1125, 250),
		DIV_ROUND_UP(1125, 130),
	};
	const char *names1[] = {
		"sdmmc0_xin_clk",
		"sdmmc1_xin_clk",
		"nfc_clk_flash",
		"qspi_ext_clk",
		"ust_clk",
	};
	const int32_t divs2[] = {
		DIV_ROUND_UP(1125, 125),
		DIV_ROUND_UP(1125, 125),
		DIV_ROUND_UP(1125, 125),
		DIV_ROUND_UP(1125, 125),
	};
	const char *names2[] = {
		"emac0_clk_1588",
		"emac0_rgmii_txc",
		"emac1_clk_1588",
		"emac1_rgmii_txc",
	};
	const int32_t divs3[] = {
		DIV_ROUND_UP(1125, 125),
		DIV_ROUND_UP(1125, 125),
		DIV_ROUND_UP(1125, 125),
		DIV_ROUND_UP(1125, 125),
	};
	const char *names3[] = {
		"usb0_ref_alt_clk",
		"usb0_suspend_clk",
		"usb1_ref_alt_clk",
		"usb1_suspend_clk",
	};

	if (ppolicy_on(HSPERIPH_SUBS_PPOLICY))
		return -1;

	UCG_HSP_UCG0->BP = 0x1fff;
	UCG_HSP_UCG1->BP = 0x1f;
	UCG_HSP_UCG2->BP = 0xf;
	UCG_HSP_UCG3->BP = 0xf;
	REG(HSP_URB + 0xc) = 0; // REFCLK = PLL for all UCGs
	if (setup_pll(HSP_URB_PLL,
		      FIELD_PREP(PLL_NR, 2) | FIELD_PREP(PLL_NF, 249) | FIELD_PREP(PLL_OD, 1) | PLL_MAN | 0x1,
		      "HSP_PLL")) // 1125 MHz
		return -1;

	if (setup_ucg(UCG_HSP_UCG0, ARRAY_LENGTH(divs0), divs0, names0, HSP_URB_PLL, "UCG0"))
		return -1;

	if (setup_ucg(UCG_HSP_UCG1, ARRAY_LENGTH(divs1), divs1, names1, HSP_URB_PLL, "UCG1"))
		return -1;

	if (setup_ucg(UCG_HSP_UCG2, ARRAY_LENGTH(divs2), divs2, names2, HSP_URB_PLL, "UCG2"))
		return -1;

	if (setup_ucg(UCG_HSP_UCG3, ARRAY_LENGTH(divs3), divs3, names3, HSP_URB_PLL, "UCG3"))
		return -1;

	UCG_HSP_UCG0->BP = 0;
	UCG_HSP_UCG1->BP = 0;
	UCG_HSP_UCG2->BP = 0;
	UCG_HSP_UCG3->BP = 0;

	REG(HSP_URB_RST) = 0x3ff0000; // deassert all resets in HSPERIPH

	uart_puts(UART0, "\n");

	return 0;
}
#else
static int setup_subs_hsperiph(void)
{
	return 0;
}
#endif

static int setup_subs_lsperiph0(void)
{
	const int32_t divs[] = {
		DIV_ROUND_UP(189, 200),
		DIV_ROUND_UP(189, 14),
		DIV_ROUND_UP(189, 14),
		DIV_ROUND_UP(189, 14),
		DIV_ROUND_UP(189, 200),
		DIV_ROUND_UP(189, 200),
		DIV_ROUND_UP(189, 1),
	};
	const char *names[] = {
		"sys_clk",
		"uart3_clk",
		"uart1_clk",
		"uart2_clk",
		"ssi0_clk",
		"i2c0_clk",
		"gpio0_clk",
	};

	if (ppolicy_on(LSPERIPH0_SUBS_PPOLICY))
		hang();

	UCG_LSP0_UCG0->BP = 0x7f;
	if (setup_pll(LSP0_URB_PLL, 6, "LSP0_PLL")) // 189 MHz
		return -1;

	if (setup_ucg(UCG_LSP0_UCG0, ARRAY_LENGTH(divs), divs, names, LSP0_URB_PLL, "UCG"))
		return -1;

	UCG_LSP0_UCG0->BP = 0;
	uart_puts(UART0, "\n");

	return 0;
}

static int setup_subs_lsperiph1(void)
{
	const int32_t divs0[] = {
		DIV_ROUND_UP(594, 150),
		DIV_ROUND_UP(594, 150),
		DIV_ROUND_UP(594, 150),
		DIV_ROUND_UP(594, 150),
		DIV_ROUND_UP(594, 125),
		DIV_ROUND_UP(594, 200),
		DIV_ROUND_UP(594, 15),
		DIV_ROUND_UP(594, 150),
		DIV_ROUND_UP(594, 150),
		DIV_ROUND_UP(594, 150),
	};
	const char *names0[] = {
		"sys_clk",
		"i2c1_clk",
		"i2c2_clk",
		"i2c3_clk",
		"gpio1_dbclk",
		"ssi1_clk",
		"uart0_clk",
		"timers0_clk",
		"pwm0_clk",
		"wdt1_clk",
	};
	const int32_t divs_i2s[] = {
		DIV_ROUND_UP(594, 50),
	};
	const char *names_i2s[] = {
		"i2s0_clk",
	};

	if (ppolicy_on(LSPERIPH1_SUBS_PPOLICY))
		hang();

	// UART0 in hardware mode
	gpio_set_function_mask(GPIO1, GPIO_BANK_B, BIT(6) | BIT(7), GPIO_FUNC_PERIPH);

	ppolicy_on(LSP1_URB_I2S_UCG_RSTN_PPOLICY);
	UCG_LSP1_UCG0->CTR[6] = 0x2;
	UCG_LSP1_UCG0->BP = 0x3ff;
	UCG_LSP1_UCG_I2S->BP = 0x1;

	// UART0 must be setup first
	setup_pll(LSP1_URB_PLL, 21, NULL);
	ucg_chan_set_div_and_enable(UCG_LSP1_UCG0, 6, divs0[6], 1);
	mdelay(1);
	UCG_LSP1_UCG0->BP &= ~BIT(6);
	uart_init(UART0, 14850000, 9600);

	// Call setup_pll() again to print message to UART
	if (setup_pll(LSP1_URB_PLL, 21, "LSP1_PLL")) // 594 MHz
		return -1;

	if (setup_ucg(UCG_LSP1_UCG0, ARRAY_LENGTH(divs0), divs0, names0, LSP1_URB_PLL, "UCG0"))
		return -1;

	if (setup_ucg(UCG_LSP1_UCG_I2S, ARRAY_LENGTH(divs_i2s), divs_i2s, names_i2s, LSP1_URB_PLL, "UCG_I2S"))
		return -1;

	UCG_LSP1_UCG0->BP = 0;
	UCG_LSP1_UCG_I2S->BP = 0;
	uart_puts(UART0, "\n");

	return 0;
}

static int setup_subsystems(void)
{
	set_tick_freq(get_pll_freq_khz(SERVICE_URB_PLL) * 1000);

	// interconnect UCG channels must be in bypass at subsystem enable
	UCG_IC_UCG0->BP = 0xff;
	UCG_IC_UCG1->BP = 0x1ff;

	REG(TOP_CLKGATE) = 0x1ff; // enable clocks for all subsystems

	// LSPeriph1 clocks must be setup first because UART0 is depends on LSPeriph1 UCG.
	if (setup_subs_lsperiph1())
		hang();

	/* LSPeriph0 must be enabled second because LEDs is depends on LSPeriph0
	 * After that if error occurred we return with code -1 (instead of hang) and main()
	 * function can signalling by TEST_ERROR LED.
	 */
	if (setup_subs_lsperiph0())
		return -1;

	if (setup_subs_hsperiph())
		return -1;

	if (setup_subs_ic())
		return -1;

	if (setup_subs_cpu())
		return -1;

	if (setup_subs_service())
		return -1;

	if (setup_subs_sdr())
		return -1;

	if (setup_subs_media())
		return -1;

	if (setup_subs_ddr())
		return -1;

	return 0;
}

#if defined(USE_CPU)
static int run_arm_cpu(void)
{
	uart_printf(UART0, "Starting ARM from address %#x...\n", ARM_CPU_ADDR);

	// copy program for ARM CPU to SPRAM
	for (int i = 0; i < ARRAY_LENGTH(arm_cpu_prog); i++)
		REG(ARM_CPU_ADDR + (i * 4)) = arm_cpu_prog[i];

	if (ppolicy_on(0x1000040)) // a53sys_ppolicy
		return -1;

	// all cores to reset
	for (int i = 0; i < 4; i++) {
		if (ppolicy_set(0x1000000 + (i * 0x10), PP_WARM_RST))
			return -1;
	}

	// set start address for all cores
	for (int i = 0; i < 4; i++) {
		REG(0x1000118 + (i * 0x8)) = 0;
		REG(0x100011c + (i * 0x8)) = ARM_CPU_ADDR;
	}

	// run all cores
	for (int i = 0; i < 4; i++) {
		if (ppolicy_on(0x1000000 + (i * 0x10)))
			return -1;
	}

	return 0;
}
#endif

#if defined(USE_SDR)
static void run_risc1(void)
{
	uart_puts(UART0, "Starting RISC1...\n");

	// copy program for RISC1 to RISC1 CRAM
	for (int i = 0; i < ARRAY_LENGTH(risc1_prog); i++)
		REG(0x3b00000 + (i * 4)) = risc1_prog[i];

	REG(SDR_URB + 0x140) = 0x1; // Soft NMU interrupt
}
#endif

static void set_led(uint32_t led, uint32_t state)
{
#if defined(IS_PMI_BOARD)
	gpio_set_value(GPIO0, GPIO_BANK_A, led, state)
#elif defined(IS_SMARC3_BOARD)
	uint32_t bank;
	uint32_t g;

	switch (led) {
	case TEST_GO:
		g = GPIO1;
		bank = GPIO_BANK_D;
		led = 3;
		break;
	case TEST_ERROR:
		g = GPIO0;
		bank = GPIO_BANK_D;
		led = 7;
		break;
	default:
		return;
	}

	gpio_set_value(g, bank, led, !state)
#endif
}

int main(void)
{
	int ret = setup_subsystems();
	uint32_t go_state = 0;
	uint32_t all_gpios_state = 0;
	unsigned long last_tick_go_led;
	unsigned long last_tick_all_gpios;
	unsigned long last_tick_pvt;
#if defined(USE_CPU) || defined(USE_SDR)
	unsigned long last_tick_cpus_check;
#endif
#if defined(USE_CPU)
	uintptr_t arm_cpu_counters_addr = ARM_CPU_ADDR + 0x100;
	uint32_t arm_cpu_counters_values[4] = { 0 };
#endif
#if defined(USE_SDR)
	uintptr_t risc1_counter_addr = ARM_CPU_ADDR + 0x180;
	uint32_t risc1_counter_value = 0;
#endif

#if defined(IS_PMI_BOARD)
	gpio_set_function_mask(GPIO0, GPIO_BANK_A, 0xff, GPIO_FUNC_GPIO);
	gpio_set_function_mask(GPIO0, GPIO_BANK_B, 0xff, GPIO_FUNC_GPIO);
	gpio_set_function_mask(GPIO0, GPIO_BANK_C, 0xff, GPIO_FUNC_GPIO);
	gpio_set_function_mask(GPIO0, GPIO_BANK_D, 0xff, GPIO_FUNC_GPIO);

	gpio_set_direction_mask(GPIO0, GPIO_BANK_A, 0xff, GPIO_DIR_OUT);
	gpio_set_direction_mask(GPIO0, GPIO_BANK_B, 0xff, GPIO_DIR_OUT);
	gpio_set_direction_mask(GPIO0, GPIO_BANK_C, 0xff, GPIO_DIR_OUT);
	gpio_set_direction_mask(GPIO0, GPIO_BANK_D, 0xff, GPIO_DIR_OUT);
	gpio_set_value_mask(GPIO0, GPIO_BANK_A, 0xff, 0);
	gpio_set_value_mask(GPIO0, GPIO_BANK_B, 0xff, 0);
	gpio_set_value_mask(GPIO0, GPIO_BANK_C, 0xff, 0);
	gpio_set_value_mask(GPIO0, GPIO_BANK_D, 0xff, 0);
	REG(LSP0_URB + 0x8) = 0xffffffff; // disable Pull-Up

	REG(LSP1_URB_GPIO1_V18) = 0x1; // setup GPIO1 to 1.8V
	for (int p = 0; p < 4; p++)
		for (int i = 0; i < 8; i++)
			REG(LSP1_URB_PAD_CTR(p, i)) = 0x78; // 4mA

	gpio_set_function_mask(GPIO1, GPIO_BANK_A, 0xff, GPIO_FUNC_GPIO);
	gpio_set_function_mask(GPIO1, GPIO_BANK_B, 0x3f, GPIO_FUNC_GPIO); // pins 6 and 7 is UART0
	gpio_set_function_mask(GPIO1, GPIO_BANK_C, 0xff, GPIO_FUNC_GPIO);
	gpio_set_function_mask(GPIO1, GPIO_BANK_D, 0xff, GPIO_FUNC_GPIO);

	gpio_set_direction_mask(GPIO1, GPIO_BANK_A, 0xff, GPIO_DIR_OUT);
	gpio_set_direction_mask(GPIO1, GPIO_BANK_B, 0x3f, GPIO_DIR_OUT);
	gpio_set_direction_mask(GPIO1, GPIO_BANK_C, 0xff, GPIO_DIR_OUT);
	gpio_set_direction_mask(GPIO1, GPIO_BANK_D, 0xff, GPIO_DIR_OUT);

	gpio_set_value_mask(GPIO1, GPIO_BANK_A, 0xff, 0);
	gpio_set_value_mask(GPIO1, GPIO_BANK_B, 0x3f, 0);
	gpio_set_value_mask(GPIO1, GPIO_BANK_C, 0xff, 0);
	gpio_set_value_mask(GPIO1, GPIO_BANK_D, 0xff, 0);
#elif defined(IS_SMARC3_BOARD)
	gpio_set_function_mask(GPIO1, GPIO_BANK_D, BIT(3), GPIO_FUNC_GPIO);
	gpio_set_function_mask(GPIO0, GPIO_BANK_D, BIT(7), GPIO_FUNC_GPIO);
	gpio_set_function_mask(GPIO0, GPIO_BANK_B, BIT(2), GPIO_FUNC_GPIO);
	gpio_set_direction(GPIO1, GPIO_BANK_D, 3, GPIO_DIR_OUT);
	gpio_set_direction(GPIO0, GPIO_BANK_D, 7, GPIO_DIR_OUT);
	gpio_set_direction(GPIO0, GPIO_BANK_B, 2, GPIO_DIR_OUT);
#endif

	uart_puts(UART0,
		  "Soak test (commit: '" STRINGIZE(GIT_SHA1_SHORT) "', build: '" STRINGIZE(BUILD_ID) "')\n");

#if defined(USE_CPU)
	for (int i = 0; i < 4; i++)
		REG(arm_cpu_counters_addr + (i * 0x8)) = 0;

	if (!ret)
		ret = run_arm_cpu();
#endif

	if (ret) {
		set_led(TEST_ERROR, 1);
		hang();
	}

#if defined(USE_SDR)
	REG(risc1_counter_addr) = 0;
	run_risc1();
#endif

	set_led(LOG0, 0);
	set_led(LOG1, 1);
	set_led(TEST_GO, 1);
	set_led(TEST_ERROR, 1);
	mdelay(1000);
	set_led(TEST_GO, 0);
	set_led(TEST_ERROR, 0);

	pvt_init();

	last_tick_go_led = get_tick_counter();
	last_tick_all_gpios = get_tick_counter();
	last_tick_pvt = get_tick_counter();
#if defined(USE_CPU) || defined(USE_SDR)
	last_tick_cpus_check = get_tick_counter();
#endif

	uart_puts(UART0, "Start main cycle\n");
	while (1) {
		if (ticks_to_us(ticks_since(last_tick_go_led)) >= 250000) {
			last_tick_go_led = get_tick_counter();
			set_led(TEST_GO, go_state);
			go_state = !go_state;
		}
		if (ticks_to_us(ticks_since(last_tick_all_gpios)) >= 50000) {
			last_tick_all_gpios = get_tick_counter();
#if defined(IS_PMI_BOARD)
			gpio_set_value_mask(GPIO0, GPIO_BANK_A, 0xc3, all_gpios_state);
			gpio_set_value_mask(GPIO0, GPIO_BANK_B, 0xff, all_gpios_state);
			gpio_set_value_mask(GPIO0, GPIO_BANK_C, 0xff, all_gpios_state);
			gpio_set_value_mask(GPIO0, GPIO_BANK_D, 0xff, all_gpios_state);
			gpio_set_value_mask(GPIO1, GPIO_BANK_A, 0xff, all_gpios_state);
			gpio_set_value_mask(GPIO1, GPIO_BANK_B, 0x3f, all_gpios_state);
			gpio_set_value_mask(GPIO1, GPIO_BANK_C, 0xff, all_gpios_state);
			gpio_set_value_mask(GPIO1, GPIO_BANK_D, 0xff, all_gpios_state);
#elif defined(IS_SMARC3_BOARD)
			gpio_set_value_mask(GPIO0, GPIO_BANK_B, BIT(2), all_gpios_state);
#endif
			all_gpios_state ^= 0xff;
		}

		if (ticks_to_us(ticks_since(last_tick_pvt)) >= 5000000) {
			last_tick_pvt = get_tick_counter();
			uart_printf(UART0, "temp = %d m" DEGREE_CELSIUS_UTF8_SYM "\n",
				    pvt_ts_measure_mcelsius(3));
		}

#if defined(USE_CPU) || defined(USE_SDR)
		/* Each cores of ARM CPU and RISC1 increasing uint32_t in SPRAM. They do it once in
		 * ~200ms. To prevent false errors we check values each 1s.
		 */
		if (ticks_to_us(ticks_since(last_tick_cpus_check)) >= 1000000) {
			uint32_t v;

			last_tick_cpus_check = get_tick_counter();
#if defined(USE_CPU)
			// check ARM cores
			for (int i = 0; i < 4; i++) {
				v = REG(arm_cpu_counters_addr + (i * 0x8));
				if (v == arm_cpu_counters_values[i]) {
					uart_printf(UART0, "ARM CPU core %d is stopped %#x\n", i, v);
					set_led(TEST_ERROR, 1);
				}

				arm_cpu_counters_values[i] = v;
			}
#endif // defined(USE_CPU)

#if defined(USE_SDR)
			// check RISC1
			v = REG(risc1_counter_addr);
			if (v == risc1_counter_value) {
				uart_printf(UART0, "RISC1 is stopped\n");
				set_led(TEST_ERROR, 1);
			}

			risc1_counter_value = v;
#endif // defined(USE_SDR)
		}
#endif // defined(USE_CPU) || defined(USE_SDR)
	}

	return 0;
}
