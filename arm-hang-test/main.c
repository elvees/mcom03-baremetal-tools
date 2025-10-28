// Copyright 2025 RnD Center "ELVEES", JSC
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <regs.h>

#define MEM_START_ADDR	  0x890410000UL
#define TEST_COUNTER_ADDR 0x8000

#define THREAD_MEM_STRIDE (128 * 1024)
#define CPU_MEM_STRIDE	  (4 * THREAD_MEM_STRIDE)

#define MEM_SIZE    (128 * 1024)
#define WORDS_COUNT (MEM_SIZE / sizeof(unsigned long))

static inline int run_test(unsigned long volatile *buf0, unsigned long volatile *buf1,
			   unsigned long volatile *buf2, unsigned long volatile *buf3)
{
	unsigned long i, val = 0;

	for (i = 0; i < WORDS_COUNT; i++, buf0++, buf1++, buf2++, buf3++)
		val += *buf0 + *buf1 + *buf2 + *buf3;

	return val;
}

static inline unsigned long get_core_id_aarch64()
{
	unsigned long mpidr_el1;

	asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr_el1));

	return mpidr_el1 & 0xff;
}

int main(void)
{
	unsigned long id = get_core_id_aarch64();
	int i = 0;

	while (1) {
		if ((i % 1024) == 0)
			REG(TEST_COUNTER_ADDR + id * 8) = i / 1024;

		unsigned long thread0_addr = MEM_START_ADDR + id * CPU_MEM_STRIDE;

		run_test((unsigned long volatile *)(thread0_addr),
			 (unsigned long volatile *)(thread0_addr + THREAD_MEM_STRIDE),
			 (unsigned long volatile *)(thread0_addr + 2 * THREAD_MEM_STRIDE),
			 (unsigned long volatile *)(thread0_addr + 3 * THREAD_MEM_STRIDE));
		i++;
	}

	return 0;
}
