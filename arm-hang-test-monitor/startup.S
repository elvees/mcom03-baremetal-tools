// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#include "machine/asm.h"
#include "machine/regdef.h"
#include "mips/m32c0.h"

.set	noreorder
.set	mips32
.set	noat

#define CP0_STATUS		12
#define CP0_CAUSE		13
#define CP0_CONFIG		16
#define K0_CACHE_MASK		0x00000007
#define K0_CACHE_ENABLE		0x00000003
#define K23_CACHE_MASK		0x70000000
#define KU_CACHE_MASK		0x0E000000
#define K23_CACHE_DISABLE	0x20000000
#define KU_CACHE_ENABLE		0x06000000

LEAF(_start)
setup_cpu:
	li	k0, 0x000000           // disable interrupts, IE[0] = 0, ERL[2] = 0,
	mtc0	k0, $CP0_STATUS        // BEV[22] = 0

	li  	k0, 0xFF7FFFFF         // IV = 0
	mfc0	k1, $CP0_CAUSE
	and 	k1, k1, k0
	mtc0	k1, $CP0_CAUSE

	li  	k0, 0xbfd08000         // Switch to fixed mapping
	li  	v0, 0x1
	sw  	v0, 0(k0)

	mfc0	k1, $CP0_CONFIG        // setup program cache

	li	k0, (~K0_CACHE_MASK)
	and	k1, k1, k0
	li	k0, K0_CACHE_ENABLE
	or	k1, k1, k0

	li	k0, (~K23_CACHE_MASK)
	and	k1, k1, k0
	li	k0, K23_CACHE_DISABLE
	or	k1, k1, k0

	li	k0, (~KU_CACHE_MASK)
	and	k1, k1, k0
	li	k0, KU_CACHE_ENABLE
	or	k1, k1, k0

	mtc0	k1, $CP0_CONFIG

	la	sp, _stack
	la	gp, _gp

	la	a0, __data_start
	la	a1, __data_end
	la	a2, __text_end

	/* Check if .data is already in the RAM */
	subu	a3, a2, a0
	beqz	a3, data_copy_done
	nop
data_copy_loop:
	subu	a3, a1, a0
	blez	a3, data_copy_done
	nop
	lw	a3, 0(a2)
	sw	a3, 0(a0)
	addiu	a0, 4
	b	data_copy_loop
	addiu	a2, 4  /* delayed slot */

data_copy_done:
/* Zero fill .bss/.sbss sections */
	la	a0, __bss_start
	la	a1, __bss_length
	li	v0, 0
bss_zero_loop:
	blez	a1, bss_zero_done
	nop
	sw	v0, 0(a0)
	addiu	a0, a0, 4
	addiu	a1, a1, -4
	b	bss_zero_loop
	nop
bss_zero_done:
	la	a2, main
	jalr	a2
	nop
jtag:
	nop
	b jtag
END(_start)
