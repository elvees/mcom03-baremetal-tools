// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#ifndef _OTP_H_
#define _OTP_H_

#include <stdint.h>
#include <regs.h>
#include <snps_ssi.h>

#define OTP_FLAG_ECC_DIS BIT(0)
#define OTP_FLAG_ECC_GEN BIT(1)
#define OTP_FLAG_ECC_TST BIT(2)
#define OTP_FLAG_BRP_DIS BIT(3)
#define OTP_FLAG_BRP_GEN BIT(4)
#define OTP_FLAG_MASK	 (OTP_FLAG_ECC_DIS | OTP_FLAG_ECC_GEN | OTP_FLAG_ECC_TST | OTP_FLAG_BRP_DIS | OTP_FLAG_BRP_GEN)

#define OTP_WORDS_COUNT 128

#define OTP_ERR			-1
#define OTP_ERR_BUS		-2
#define OTP_ERR_PROG_SOAK_LIMIT -3
#define OTP_ERR_PROG_COMPARE	-4

void SBPI_initMaster(SNPS_SSI_regs_t *SSI);

int otp_program(uint32_t *buffer, uint8_t *ecc, uint16_t idx_start, uint32_t count, uint16_t *err_addr);
int otp_bist(uint16_t otp_addr, uint16_t count, int is_bisr, uint16_t *err_addr);
void otp_read(uint32_t *buf_data, uint8_t *buf_ecc, uint32_t otp_addr, uint32_t count, uint8_t flags);
uint8_t otp_calculate_ecc(uint32_t data);

#endif // _OTP_H_
