// SPDX-License-Identifier: MIT
// Copyright 2021 RnD Center "ELVEES", JSC

#include "machine/asm.h"
#include "machine/regdef.h"
#include "mips/m32c0.h"

.set    noreorder

LEAF(_start)
setup_cpu:
#ifdef CAN_RETURN
	addiu	sp,sp,-48
	sw	s0,4(sp)
	sw	s1,8(sp)
	sw	s2,12(sp)
	sw	s3,16(sp)
	sw	s4,20(sp)
	sw	s5,24(sp)
	sw	s6,28(sp)
	sw	s7,32(sp)
	sw	s8,36(sp)
	sw	gp,40(sp)
	sw	ra,44(sp)
#else
	la	sp, _stack
	la	gp, _gp
#endif

/* TODO: Need .data section copy */

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
#ifdef CAN_RETURN
	lw	s0,4(sp)
	lw	s1,8(sp)
	lw	s2,12(sp)
	lw	s3,16(sp)
	lw	s4,20(sp)
	lw	s5,24(sp)
	lw	s6,28(sp)
	lw	s7,32(sp)
	lw	s8,36(sp)
	lw	gp,40(sp)
	lw	ra,44(sp)
	jr	ra
	addiu   sp,sp,48
#endif
END(_start)
