// SPDX-License-Identifier: MIT
// Copyright 2021 RnD Center "ELVEES", JSC

.section ".text.boot"

.global _start

_start:
	ldr	x1, =_stack
	mov	sp, x1

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
