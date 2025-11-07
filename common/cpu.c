// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#include <cpu.h>
#include <delay.h>
#include <regs.h>

int start_arm_core(int id, unsigned long long addr)
{
	uint32_t val;
	int ret;

	if (REG(CPU_URB_A53SYS_PSTATUS) != PP_ON) {
		REG(CPU_URB_STARTCFG) = 0xf;
		REG(CPU_URB_A53SYS_PPOLICY) = PP_ON;
		ret = poll_timeout(REG(CPU_URB_A53SYS_PSTATUS), val,
				   (val & 0x1f) == PP_ON, 100, 5000000);
		if (ret)
			return ret;

		// Put all cores to reset
		for (int i = 0; i < 4; i++) {
			REG(CPU_URB_A53CPUX_PPOLICY(i)) = PP_WARM_RST;
			ret = poll_timeout(REG(CPU_URB_A53CPUX_PSTATUS(i)), val,
					   (val & 0x1f) == PP_WARM_RST, 100, 5000000);
			if (ret)
				return ret;
		}
	}

	// Set start address
	REG(CPU_URB_RVBADDRH(id)) = (addr >> 32) & 0xffffffff;
	REG(CPU_URB_RVBADDRL(id)) = addr & 0xffffffff;

	// Run core
	REG(CPU_URB_A53CPUX_PPOLICY(id)) = PP_ON;
	ret = poll_timeout(REG(CPU_URB_A53CPUX_PSTATUS(id)), val,
			   (val & 0x1f) == PP_ON, 100, 5000000);
	if (ret)
		return ret;

	return 0;
}
