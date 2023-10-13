// SPDX-License-Identifier: MIT
// Copyright 2021 RnD Center "ELVEES", JSC

.section ".text.boot"

.global _start

_start:
#ifdef CAN_RETURN
	sub	sp, sp, #0x60
	stp	x19, x20, [sp]
	stp	x21, x22, [sp, #0x10]
	stp	x23, x24, [sp, #0x20]
	stp	x25, x26, [sp, #0x30]
	stp	x27, x28, [sp, #0x40]
	stp	x29, x30, [sp, #0x50]
#else
	ldr	x1, =_stack
	mov	sp, x1
#endif

_copy_data:
	ldr	x1, =__data_start  // destination
	ldr	x2, =__data_end
	ldr	x3, =__text_end    // source
	cmp	x1, x3             // If source == destination
	b.eq	_clean_bss
_copy_data_loop:
	cmp	x1, x2
	b.ge	_clean_bss
	ldr	w0, [x3]
	str	w0, [x1]
	add	x1, x1, #4
	add	x3, x3, #4
	b	_copy_data_loop

_clean_bss:
	mov	w0, #0
	ldr	x1, =__bss_start
	ldr	x2, =__bss_end

_clean_bss_loop:
	cmp	x1, x2
	b.ge	_main
	str	w0, [x1]
	add	x1, x1, #4
	b	_clean_bss_loop

_main:
	bl	main
#ifdef CAN_RETURN
	ldp	x19, x20, [sp]
	ldp	x21, x22, [sp, #0x10]
	ldp	x23, x24, [sp, #0x20]
	ldp	x25, x26, [sp, #0x30]
	ldp	x27, x28, [sp, #0x40]
	ldp	x29, x30, [sp, #0x50]
	add	sp, sp, #0x60
	ret
#endif
