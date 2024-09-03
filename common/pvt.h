// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#include <stdbool.h>
#include <stdint.h>

#include <console.h>

DECLARE_VERBOSE(pvt)

uint32_t pvt_ts_measure_mcelsius(uint32_t channel);
uint32_t pvt_vm_measure_mv(uint32_t channel);
void pvt_vm_set_tval(bool enable);
void pvt_vm_set_sel_vin(uint32_t vin);
void pvt_init(void);
