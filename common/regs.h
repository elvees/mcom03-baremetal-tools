// SPDX-License-Identifier: MIT
// Copyright 2021-2024 RnD Center "ELVEES", JSC

#ifndef REGS_H_
#define REGS_H_

#include <stdint.h>

#define GENMASK(hi, lo) (((1 << ((hi) - (lo) + 1)) - 1) << (lo))

#define __bf_shf(x) (__builtin_ffs(x) - 1)

#define FIELD_PREP(_mask, _val) ({ ((typeof(_mask))(_val) << __bf_shf(_mask)) & (_mask); })
#define FIELD_GET(_mask, _reg)	({ (typeof(_mask))(((_reg) & (_mask)) >> __bf_shf(_mask)); })

#ifdef MIPS32
#define TO_VIRT(x) (0xa0000000 + (x))
#define TO_PHYS(x) ((x) & ~0xa0000000)
#else
#define TO_VIRT(x) (x)
#define TO_PHYS(x) (x)
#endif

#define REG(x)		(*((volatile uint32_t *)((uintptr_t)(TO_VIRT(x)))))
#define BIT(x)		(1UL << (x))
#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

#define HSPERIPH_SUBS_PPOLICY 0x1f000020
#define HSPERIPH_SUBS_PSTATUS 0x1f000024

#define LSPERIPH0_SUBS_PPOLICY 0x1f000028
#define LSPERIPH0_SUBS_PSTATUS 0x1f00002c

#define LSPERIPH1_SUBS_PPOLICY 0x1f000030
#define LSPERIPH1_SUBS_PSTATUS 0x1f000034
#define TOP_CLKGATE	       0x1f001008
#define XIP_EN_REQ	       0x1f002004
#define XIP_EN_OUT	       0x1f002008
#define QSPI0_BASE	       0x1ff00000

#define PP_ON	    0x10
#define PP_WARM_RST 0x8
#define PP_OFF	    0x1

#define SERVICE_BASE	0x1f000000
#define SERVICE_URB	SERVICE_BASE
#define SERVICE_UCG	(SERVICE_BASE + 0x20000)
#define SERVICE_URB_PLL (SERVICE_URB + 0x1000)

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

#define LSP1_BASE  0x01700000
#define UART0_BASE (LSP1_BASE + 0x50000)
#define GPIO1_BASE (LSP1_BASE + 0x80000)
#define I2C1_BASE  (LSP1_BASE + 0x10000)
#define I2C2_BASE  (LSP1_BASE + 0x20000)
#define I2C3_BASE  (LSP1_BASE + 0x30000)
#define LSP1_UCG   (LSP1_BASE + 0xc0000)
#define LSP1_URB   (LSP1_BASE + 0xe0000)

#define GPIO1_SWPORTA_CTL (GPIO1_BASE + 0x08)
#define GPIO1_SWPORTB_CTL (GPIO1_BASE + 0x14)
#define GPIO1_SWPORTD_DR  (GPIO1_BASE + 0x24)
#define GPIO1_SWPORTD_DDR (GPIO1_BASE + 0x28)
#define GPIO1_SWPORTD_CTL (GPIO1_BASE + 0x2c)

#define LSP1_UCG_CTRL4 (LSP1_UCG + 0x10) // GPIO_DBCLK
#define LSP1_UCG_CTRL6 (LSP1_UCG + 0x18) // UART_CLK

#define UCG_CTR_CLK_EN BIT(1)

#define LSP1_URB_PLL		    LSP1_URB
#define LSP1_URB_PAD_CTR(port, num) (LSP1_URB + 0x20 + ((port) * 0x20) + ((num) * 0x4))

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
#define HSP_UCG(x)		  (0x10410000 + ((x) * 0x10000))
#define QSPI1_BASE		  0x10260000

typedef struct {
	volatile uint32_t CTR_REG[16];
	volatile uint32_t BP_CTR_REG;
	volatile uint32_t SYNC_CLK_REG;
} ucg_regs_t;

#endif
