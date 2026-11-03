// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#ifndef PVT_H_
#define PVT_H_

#include <stdbool.h>
#include <stdint.h>

#include <console.h>

#define PVT_MEASURE_ERROR (int32_t)0x80000000

DECLARE_VERBOSE(pvt)

int32_t pvt_ts_measure_mcelsius(uint32_t channel);
int32_t pvt_vm_measure_mv(uint32_t channel);
int pvt_ts_write(uint32_t channel, uint32_t addr, uint32_t value);
int32_t pvt_ts_read(uint32_t channel, uint32_t addr);
int pvt_vm_write(uint32_t channel, uint32_t addr, uint32_t value);
int32_t pvt_vm_read(uint32_t channel, uint32_t addr);
void pvt_ts_set_auto_mode(bool enable);
void pvt_ts_set_continuous_mode(bool enable);
void pvt_ts_set_cal(unsigned int channel, bool enable);
void pvt_ts_set_sgn_en(unsigned int channel, bool enable);
void pvt_ts_set_tm_an(unsigned int channel, uint32_t tm_an);
void pvt_vm_set_tval(unsigned int channel, bool enable);
void pvt_vm_set_sel_vin(unsigned int channel, uint32_t vin);
void pvt_configure(uint32_t cfg_ts_freq, uint32_t cfg_vm_freq);
void pvt_init(void);

#endif
