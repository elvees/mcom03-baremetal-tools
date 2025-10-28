// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

.section ".text.boot"

.align 3

// TT block entries templates   (L1 and L2, NOT L3)
// Assuming table contents:
// 0 = b01000100 = Normal, Inner/Outer Non-Cacheable
// 1 = b11111111 = Normal, Inner/Outer WB/WA/RA
// 2 = b00000000 = Device-nGnRnE
.equ TT_S1_FAULT,           0x0
.equ TT_S1_NORMAL_NO_CACHE, 0x00000000000000401    // Index = 0, AF=1
.equ TT_S1_NORMAL_WBWA,     0x00000000000000405    // Index = 1, AF=1
.equ TT_S1_DEVICE_nGnRnE,   0x00600000000000409    // Index = 2, AF=1, PXN=1, UXN=1

.equ TT_S1_NON_SHARED,      (0 << 8)               // Non-shareable
.equ TT_S1_INNER_SHARED,    (3 << 8)               // Inner-shareable
.equ TT_S1_OUTER_SHARED,    (2 << 8)               // Outer-shareable

.global _start

_start:
	// Required for RTL simulation
	mov	x0, #0
	mov	x1, #0
	mov	x2, #0
	mov	x3, #0
	mov	x4, #0
	mov	x5, #0
	mov	x6, #0
	mov	x7, #0
	mov	x8, #0
	mov	x9, #0
	mov	x10, #0
	mov	x11, #0
	mov	x12, #0
	mov	x13, #0
	mov	x14, #0
	mov	x15, #0
	mov	x16, #0
	mov	x17, #0
	mov	x18, #0
	mov	x19, #0
	mov	x20, #0
	mov	x21, #0
	mov	x22, #0
	mov	x23, #0
	mov	x24, #0
	mov	x25, #0
	mov	x26, #0
	mov	x27, #0
	mov	x28, #0
	mov	x29, #0
	mov	x30, #0

	// Disable trapping of CPTR_EL3 accesses or use of Adv.SIMD/FPU
  	msr	CPTR_EL3, xzr
	mov	x0, #1             // NS=1
	orr	x0, x0, #(1 << 1)  // IRQ=1 IRQs routed to EL3
	orr	x0, x0, #(1 << 2)  // FIQ=1 FIQs routed to EL3
	orr	x0, x0, #(1 << 3)  // EA=1  SError routed to EL3
	orr	x0, x0, #(1 << 8)  // HCE=1 HVC instructions are enabled
	orr	x0, x0, #(1 << 10) // RW=1  Next EL down uses AArch64
	orr	x0, x0, #(1 << 11) // ST=1  Secure EL1 can access CNTPS_TVAL_EL1, CNTPS_CTL_EL1 & CNTPS_CVAL_EL1
                                   // SIF=0 Secure state instruction fetches from Non-secure memory are permitted
                                   // SMD=0 SMC instructions are enabled
                                   // TWI=0 EL2, EL1 and EL0 execution of WFI instructions is not trapped to EL3
                                   // TWE=0 EL2, EL1 and EL0 execution of WFE instructions is not trapped to EL3
	msr	SCR_EL3, x0

	// MMU setup

	ldr	x0, =tt_l1_base // Get address of level 1 for TTBR0_EL3
	msr	TTBR0_EL3, x0   // Set TTBR0_EL3 (NOTE: There is no TTBR1 at EL3)

	// 0 = b01000100 = Normal, Inner/Outer Non-Cacheable
	// 1 = b11111111 = Normal, Inner/Outer WB/WA/RA
	// 2 = b00000000 = Device-nGnRnE
	mov	x0, #0x000000000000FF44
	msr	MAIR_EL3, x0

	mov	x0, #0x19            // T0SZ=0b011001 Limits VA space to 39 bits, translation starts @ l1
	orr	x0, x0, #(0x1 << 8)  // IGRN0=0b01    Walks to TTBR0 are Inner WB/WA
	orr	x0, x0, #(0x1 << 10) // OGRN0=0b01    Walks to TTBR0 are Outer WB/WA
	orr	x0, x0, #(0x3 << 12) // SH0=0b11      Inner Shareable
	orr	x0, x0, #(0x1 << 16) // TBI0=0b0      Top byte not ignored
	   	                     // TG0=0b00      4KB granule
	   	                     // IPS=1         36-bit PA space
	msr	TCR_EL3, x0

	// Ensure changes to system register are visible before MMU enabled
	isb

	// Invalidate TLBs
	tlbi	ALLE3
	dsb	SY
	isb

	// Generate Translation Table
	ldr	x1, =tt_l1_base                // Address of L1 table

	ldr	x0, =TT_S1_DEVICE_nGnRnE       // 0x0000_0000 - 0x3FFF_FFFF
	str	x0, [x1], #8

	ldr	x0, =TT_S1_DEVICE_nGnRnE       // 0x4000_0000 - 0x7FFF_FFFF
	orr	x0, x0, #0x40000000
	str	x0, [x1], #8

	ldr	x0, =TT_S1_NORMAL_WBWA         // 0x8000_0000 - 0xBFFF_FFFF
	orr	x0, x0, #TT_S1_INNER_SHARED
	orr	x0, x0, #0x80000000
	str	x0, [x1], #8

	ldr	x0, =TT_S1_NORMAL_WBWA         // 0xC000_0000 - 0xFFFF_FFFF
	orr	x0, x0, #TT_S1_INNER_SHARED
	orr	x0, x0, #0xc0000000
	str	x0, [x1], #8

	ldr	x0, =TT_S1_DEVICE_nGnRnE       // 0x1_0000_0000 - 0x8_7FFF_FFFF
	mov	x2, #0x100000000
	mov	x3, #0x40000000
	mov	x4, #30
loop:
	orr	x0, x0, x2
	str	x0, [x1], #8
	add	x2, x2, x3
	subs	x4, x4, #1
	bne	loop

	ldr	x0, =TT_S1_NORMAL_WBWA         // 0x8_8000_0000 - 0xF_FFFF_FFFF
	orr	x0, x0, #TT_S1_INNER_SHARED
	mov	x4, #30
loop1:
	orr	x0, x0, x2
	str	x0, [x1], #8
	add	x2, x2, x3
	subs	x4, x4, #1
	bne	loop1

	dsb	SY

	mrs	x0, S3_1_C15_C2_1
	orr	x0, x0, #(0x1 << 6) // The SMP bit.
	msr	S3_1_C15_C2_1, x0

	// Enable MMU
	mov	x0, #(1 << 0)       // M=1 bit       Enable the stage 1 MMU
	orr	x0, x0, #(1 << 2)   // C=1 bit       Enable data and unified caches
	orr	x0, x0, #(1 << 12)  // I=1           Enable instruction fetches to allocate into unified caches
                                    // A=0           Strict alignment checking disabled
                                    // SA=0          Stack alignment checking disabled
                                    // WXN=0         Write permission does not imply XN
                                    // EE=0          EL3 data accesses are little endian
	msr	SCTLR_EL3, x0
	isb

	nop
	nop
	nop
	nop

	ldr	x1, =_stack
	mrs	x2, MPIDR_EL1
	and	x2, x2, #0xff	// x2 == CPU number
	mov	x3, #0x100	// stack size for each CPU = 256 KiB
	mul	x3, x2, x3
	sub	x1, x1, x3
	mov	sp, x1
	bl	main
1:
	wfi
	b	1b

// Translation tables
.section  TT,"ax"

.align 12

.global tt_l1_base

tt_l1_base:
	.fill 4096 , 1 , 0
