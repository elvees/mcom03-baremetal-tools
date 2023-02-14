/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#include "machine/asm.h"
#include "machine/regdef.h"
#include "mips/m32c0.h"

.set    noreorder

LEAF(_start)
setup_cpu:
	la	sp, _stack
	la	gp, _gp

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
	jr	a2
	nop
END(_start)
