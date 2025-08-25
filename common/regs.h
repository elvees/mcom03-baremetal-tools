// SPDX-License-Identifier: MIT
// Copyright 2021-2024 RnD Center "ELVEES", JSC

#ifndef REGS_H_
#define REGS_H_

#include <stdint.h>

#define GENMASK(hi, lo) (((1 << ((hi) - (lo) + 1)) - 1) << (lo))

#define __bf_shf(x) (__builtin_ffs(x) - 1)

#define FIELD_PREP(_mask, _val) ({ ((typeof(_mask))(_val) << __bf_shf(_mask)) & (_mask); })
#define FIELD_GET(_mask, _reg)	({ (typeof(_mask))(((_reg) & (_mask)) >> __bf_shf(_mask)); })

// TODO add support for RISC1 in SDR subsystem
#define mips_dcache_invalidate()                    \
	do {                                        \
		REG(0x1fd08000) |= BIT(14);         \
		while (REG(0x1fd08000) & BIT(14)) { \
		}                                   \
	} while (0)

#ifdef MIPS32
#define TO_VIRT(x)	    (0xa0000000 + (x))
#define TO_PHYS(x)	    ((x) & ~0xa0000000)
#define dcache_invalidate() mips_dcache_invalidate()

static inline void c0_set(uint32_t reg, uint32_t sel, uint32_t val)
{
	asm volatile("mtc0 %1,$%0,%2" : : "i"(reg), "r"(val), "i"(sel));
}

static inline uint32_t c0_get(uint32_t reg, uint32_t sel)
{
	uint32_t val;

	asm volatile("mfc0 %0,$%1,%2" : "=r"(val) : "i"(reg), "i"(sel));

	return val;
}

#else
#define TO_VIRT(x) (x)
#define TO_PHYS(x) (x)
#define dcache_invalidate()
#endif

#define REG(x)		   (*((volatile uint32_t *)((uintptr_t)(TO_VIRT(x)))))
#define REG64(x)	   (*((volatile uint64_t *)((uintptr_t)(TO_VIRT(x)))))
#define BIT(x)		   (1UL << (x))
#define ARRAY_LENGTH(x)	   (sizeof(x) / sizeof((x)[0]))
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#define CPU_SUBS_PPOLICY 0x1f000000
#define CPU_SUBS_PSTATUS 0x1f000004

#define SDR_SUBS_PPOLICY 0x1f000008
#define SDR_SUBS_PSTATUS 0x1f00000c

#define MEDIA_SUBS_PPOLICY 0x1f000010
#define MEDIA_SUBS_PSTATUS 0x1f000014

#define HSPERIPH_SUBS_PPOLICY 0x1f000020
#define HSPERIPH_SUBS_PSTATUS 0x1f000024

#define LSPERIPH0_SUBS_PPOLICY 0x1f000028
#define LSPERIPH0_SUBS_PSTATUS 0x1f00002c

#define LSPERIPH1_SUBS_PPOLICY 0x1f000030
#define LSPERIPH1_SUBS_PSTATUS 0x1f000034

#define DDR_SUBS_PPOLICY 0x1f000038
#define DDR_SUBS_PSTATUS 0x1f00003c

#define TOP_CLKGATE 0x1f001008
#define XIP_EN_REQ  0x1f002004
#define XIP_EN_OUT  0x1f002008
#define QSPI0_BASE  0x1ff00000

#define PP_ON	    0x10
#define PP_WARM_RST 0x8
#define PP_OFF	    0x1

#define CPU_URB	    0x1000000
#define CPU_URB_PLL (CPU_URB + 0x50)
#define CPU_UCG	    0x1080000

#define SERVICE_BASE	   0x1f000000
#define SERVICE_URB	   SERVICE_BASE
#define SERVICE_UCG	   (SERVICE_BASE + 0x20000)
#define SERVICE_URB_PLL	   (SERVICE_URB + 0x1000)
#define SERVICE_RISC0_VMMU (SERVICE_BASE + 0xd04000)

#define I2C4_BASE    (SERVICE_BASE + 0x90000)
#define LSP0_BASE    0x01600000
#define PDMA0_BASE   LSP0_BASE
#define GPIO0_BASE   (LSP0_BASE + 0x10000)
#define SPI0_BASE    (LSP0_BASE + 0x20000)
#define I2C0_BASE    (LSP0_BASE + 0x30000)
#define UART1_BASE   (LSP0_BASE + 0x40000)
#define UART2_BASE   (LSP0_BASE + 0x50000)
#define UART3_BASE   (LSP0_BASE + 0x60000)
#define LSP0_URB     (LSP0_BASE + 0x80000)
#define LSP0_URB_PLL LSP0_URB

#define LSP0_UCG2      (LSP0_BASE + 0x90000)
#define LSP0_UCG_CTRL0 (LSP0_UCG2) // SYS_CLK
#define LSP0_UCG_CTRL5 (LSP0_UCG2 + 0x14) // I2C0_CLK

#define LSP1_BASE    0x01700000
#define UART0_BASE   (LSP1_BASE + 0x50000)
#define GPIO1_BASE   (LSP1_BASE + 0x80000)
#define I2C1_BASE    (LSP1_BASE + 0x10000)
#define I2C2_BASE    (LSP1_BASE + 0x20000)
#define I2C3_BASE    (LSP1_BASE + 0x30000)
#define LSP1_UCG     (LSP1_BASE + 0xc0000)
#define LSP1_UCG_I2S (LSP1_BASE + 0xd0000)
#define LSP1_URB     (LSP1_BASE + 0xe0000)

#define GPIO1_SWPORTA_CTL (GPIO1_BASE + 0x08)
#define GPIO1_SWPORTB_CTL (GPIO1_BASE + 0x14)
#define GPIO1_SWPORTD_DR  (GPIO1_BASE + 0x24)
#define GPIO1_SWPORTD_DDR (GPIO1_BASE + 0x28)
#define GPIO1_SWPORTD_CTL (GPIO1_BASE + 0x2c)

#define LSP1_UCG_CTRL4 (LSP1_UCG + 0x10) // GPIO_DBCLK
#define LSP1_UCG_CTRL6 (LSP1_UCG + 0x18) // UART_CLK

#define UCG_CTR_CLK_EN BIT(1)

#define LSP1_URB_PLL		      LSP1_URB
#define LSP1_URB_PAD_CTR(port, num)   (LSP1_URB + 0x20 + ((port) * 0x20) + ((num) * 0x4))
#define LSP1_URB_I2S_UCG_RSTN_PPOLICY (LSP1_URB + 0x8)
#define LSP1_URB_I2S_UCG_RSTN_PSTATUS (LSP1_URB + 0xc)

#define PORTA	       0
#define PORTB	       1
#define PORTC	       2
#define PORTD	       3
#define PAD_CTL_SUS    BIT(0)
#define PAD_CTL_PU     BIT(1)
#define PAD_CTL_PD     BIT(2)
#define PAD_CTL_SL(x)  (((x) & 0x3) << 3)
#define PAD_CTL_CTL(x) (((x) & 0x3f) << 5)
#define PAD_CTL_E      BIT(12)
#define PAD_CTL_CLE    BIT(13)
#define PAD_CTL_OD     BIT(14)

// Timer registers
#define LSPERIPH1_TIMERS_BASE  0x1790000
#define TIMER(i)	       (LSPERIPH1_TIMERS_BASE + (i) * 0x14)
#define TIMER_LOAD_COUNT(i)    (TIMER(i) + 0x0)
#define TIMER_CURRENT_VALUE(i) (TIMER(i) + 0x4)
#define TIMER_CTRL(i)	       (TIMER(i) + 0x8)

#define HSP_URB			  0x10400000
#define HSP_URB_PLL		  HSP_URB
#define HSP_URB_RST		  (HSP_URB + 0x8)
#define HSP_URB_XIP_EN_REQ	  (HSP_URB + 0x10)
#define HSP_URB_XIP_EN_OUT	  (HSP_URB + 0x14)
#define HSP_URB_QSPI1_PADCFG	  (HSP_URB + 0x1c)
#define HSP_URB_QSPI1_SS_PADCFG	  (HSP_URB + 0x20)
#define HSP_URB_QSPI1_SISO_PADCFG (HSP_URB + 0x24)
#define HSP_URB_QSPI1_SCLK_PADCFG (HSP_URB + 0x28)
#define HSP_URB_SDMMC0_PADCFG	  (HSP_URB + 0x2c)
#define HSP_URB_SDMMC0_CLK_PADCFG (HSP_URB + 0x30)
#define HSP_URB_SDMMC0_CMD_PADCFG (HSP_URB + 0x34)
#define HSP_URB_SDMMC0_DAT_PADCFG (HSP_URB + 0x38)
#define HSP_URB_SDMMC0_CORECFG_0  (HSP_URB + 0x3c)
#define HSP_URB_SDMMC0_CORECFG_1  (HSP_URB + 0x40)
#define HSP_URB_SDMMC0_CORECFG_2  (HSP_URB + 0x44)
#define HSP_URB_SDMMC0_CORECFG_3  (HSP_URB + 0x48)
#define HSP_URB_SDMMC0_CORECFG_4  (HSP_URB + 0x4c)
#define HSP_URB_SDMMC0_CORECFG_5  (HSP_URB + 0x50)
#define HSP_URB_SDMMC0_CORECFG_6  (HSP_URB + 0x54)
#define HSP_URB_SDMMC0_CORECFG_7  (HSP_URB + 0x58)
#define HSP_URB_SDMMC0_SDHC_DBG0  (HSP_URB + 0x5c)
#define HSP_URB_SDMMC0_SDHC_DBG1  (HSP_URB + 0x60)
#define HSP_URB_SDMMC0_SDHC_DBG2  (HSP_URB + 0x64)
#define HSP_URB_SDMMC1_PADCFG	  (HSP_URB + 0x68)
#define HSP_URB_SDMMC1_CLK_PADCFG (HSP_URB + 0x6c)
#define HSP_URB_SDMMC1_CMD_PADCFG (HSP_URB + 0x70)
#define HSP_URB_SDMMC1_DAT_PADCFG (HSP_URB + 0x74)
#define HSP_URB_SDMMC1_CORECFG_0  (HSP_URB + 0x78)
#define HSP_URB_SDMMC1_CORECFG_1  (HSP_URB + 0x7c)
#define HSP_URB_SDMMC1_CORECFG_2  (HSP_URB + 0x80)
#define HSP_URB_SDMMC1_CORECFG_3  (HSP_URB + 0x84)
#define HSP_URB_SDMMC1_CORECFG_4  (HSP_URB + 0x88)
#define HSP_URB_SDMMC1_CORECFG_5  (HSP_URB + 0x8c)
#define HSP_URB_SDMMC1_CORECFG_6  (HSP_URB + 0x90)
#define HSP_URB_SDMMC1_CORECFG_7  (HSP_URB + 0x94)
#define HSP_URB_SDMMC1_SDHC_DBG0  (HSP_URB + 0x98)
#define HSP_URB_SDMMC1_SDHC_DBG1  (HSP_URB + 0x9c)
#define HSP_URB_SDMMC1_SDHC_DBG2  (HSP_URB + 0xa0)
#define HSP_URB_EMAC_PADCFG	  (HSP_URB + 0x140)
#define HSP_URB_EMAC0_PADCFG	  (HSP_URB + 0x144)
#define HSP_URB_EMAC0_TX_PADCFG	  (HSP_URB + 0x148)
#define HSP_URB_EMAC0_TXC_PADCFG  (HSP_URB + 0x14c)
#define HSP_URB_EMAC0_RX_PADCFG	  (HSP_URB + 0x150)
#define HSP_URB_EMAC0_RXC_PADCFG  (HSP_URB + 0x154)
#define HSP_URB_EMAC0_MD_PADCFG	  (HSP_URB + 0x158)
#define HSP_URB_EMAC0_MDC_PADCFG  (HSP_URB + 0x15c)
#define HSP_URB_EMAC0_BAR	  (HSP_URB + 0x160)
#define HSP_URB_EMAC1_PADCFG	  (HSP_URB + 0x164)
#define HSP_URB_EMAC1_TX_PADCFG	  (HSP_URB + 0x168)
#define HSP_URB_EMAC1_TXC_PADCFG  (HSP_URB + 0x16c)
#define HSP_URB_EMAC1_RX_PADCFG	  (HSP_URB + 0x170)
#define HSP_URB_EMAC1_RXC_PADCFG  (HSP_URB + 0x174)
#define HSP_URB_EMAC1_MD_PADCFG	  (HSP_URB + 0x178)
#define HSP_URB_EMAC1_MDC_PADCFG  (HSP_URB + 0x17c)
#define HSP_URB_EMAC1_BAR	  (HSP_URB + 0x180)
#define HSP_URB_MISC_PADCFG	  (HSP_URB + 0x1a4)

#define SDR_UCG0	  0x1900000
#define SDR_UCG_PCIE0_REF 0x1908000
#define SDR_UCG_PCIE1_REF 0x1c00000
#define SDR_UCG_N_DFE	  0x1c08000
#define SDR_URB		  0x1910000
#define SDR_URB_PLL0	  (SDR_URB)
#define SDR_URB_PLL1	  (SDR_URB + 0x8)
#define SDR_URB_PLL2	  (SDR_URB + 0x10)

#define MEDIA_URB		   0x1320000
#define MEDIA_URB_PLL0		   MEDIA_URB
#define MEDIA_URB_PLL1		   (MEDIA_URB + 0x10)
#define MEDIA_URB_PLL2		   (MEDIA_URB + 0x20)
#define MEDIA_URB_PLL3		   (MEDIA_URB + 0x30)
#define MEDIA_UCG0		   (MEDIA_URB + 0x40)
#define MEDIA_UCG1		   (MEDIA_URB + 0xc0)
#define MEDIA_UCG2		   (MEDIA_URB + 0x140)
#define MEDIA_UCG3		   (MEDIA_URB + 0x1c0)
#define MEDIA_URB_ISP_PPOLICY	   (MEDIA_URB + 0x1000)
#define MEDIA_URB_ISP_PSTATUS	   (MEDIA_URB + 0x1004)
#define MEDIA_URB_GPU_PPOLICY	   (MEDIA_URB + 0x1008)
#define MEDIA_URB_GPU_PSTATUS	   (MEDIA_URB + 0x100c)
#define MEDIA_URB_VPU_PPOLICY	   (MEDIA_URB + 0x1010)
#define MEDIA_URB_VPU_PSTATUS	   (MEDIA_URB + 0x1014)
#define MEDIA_URB_DISPLAY_PPOLICY  (MEDIA_URB + 0x1018)
#define MEDIA_URB_DISPLAY_PSTATUS  (MEDIA_URB + 0x101c)
#define MEDIA_URB_UCG_RST_CTRL	   (MEDIA_URB + 0x1040)
#define MEDIA_URB_MIPI_RX_RST_CTRL (MEDIA_URB + 0x1050)
#define MEDIA_URB_CMOS_RST_CTRL	   (MEDIA_URB + 0x1054)
#define MEDIA_URB_SUBSYSTEM_CFG	   (MEDIA_URB + 0x2000)
#define MEDIA_URB_SUBSYSTEM_CTRL   (MEDIA_URB + 0x2004)
#define MEDIA_URB_SUBSYSTEM_STATUS (MEDIA_URB + 0x2008)

#define IC_BASE 0x1800000

#define IC_URB		      IC_BASE
#define IC_URB_PLL	      IC_URB
#define IC_URB_SLOW_AXI_DLOCK (IC_URB + 0x60)
#define IC_URB_FAST_AXI_DLOCK (IC_URB + 0x64)
#define IC_URB_COH_AXI_DLOCK  (IC_URB + 0x68)
#define IC_URB_COH_REMAP      (IC_URB + 0x6c)

#define IC_UCG0 (IC_BASE + 0x1000)
#define IC_UCG1 (IC_BASE + 0x2000)

#define DDR_BASE     0x4000000
#define DDR_PHY0     DDR_BASE
#define DDR_CTL0     (DDR_BASE + 0x2000000)
#define DDR_PHY1     (DDR_BASE + 0x4000000)
#define DDR_CTL1     (DDR_BASE + 0x6000000)
#define DDR_URB	     (DDR_BASE + 0x8000000)
#define DDR_URB_PLL0 DDR_URB
#define DDR_URB_PLL1 (DDR_URB + 0x8)
#define DDR_UCG0     (DDR_BASE + 0x8010000)
#define DDR_UCG1     (DDR_BASE + 0x8020000)

#define HSP_UCG(x)  (0x10410000 + ((x) * 0x10000))
#define EMAC0_BASE  0x10200000
#define EMAC1_BASE  0x10210000
#define SDMMC0_BASE 0x10220000UL
#define SDMMC1_BASE 0x10230000UL
#define QSPI1_BASE  0x10260000

typedef struct {
	volatile uint32_t CTR_REG[16];
	volatile uint32_t BP_CTR_REG;
	volatile uint32_t SYNC_CLK_REG;
} ucg_regs_t;

#endif
