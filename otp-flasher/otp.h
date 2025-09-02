// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#ifndef _OTP_H_
#define _OTP_H_

#include <stdint.h>
#include <regs.h>
#include <snps_ssi.h>

#define UCG1  1
#define BLANK 0

#define SP_SERIAL    1 // high
#define DCTRL_DIRECT 2 // high
#define PD_OFF	     4 // high

#define SP_PARALL 0 // low
#define DCTRL_SSI 0 // low
#define PD_ON	  0 // low

#define INST_SS		 0x2
#define DATA_SS		 0x1
#define NUM_BYTE_SL_ADDR 0x1
#define PMC_ID		 0x3A
#define DAP_ID		 0x2

// Инструкции PMC
#define START_CMD 0x1
#define STOP_CMD  0x2

// BOOT_CQ Register ENTRY and EXIT codes
#define BOOT_START_ENTRY    0x00
#define BOOT_READ_OTP_ENTRY 0x0E
#define BOOT_DONE_ENTRY	    0x1F
#define BOOT_START_EXIT	    0x00
#define BOOT_READ_OTP_EXIT  0x09
#define BOOT_DONE_EXIT	    0x00

// Инструкции PMC & DAP
#define NOP_CMD	 0
#define RDF_CMD	 0x80
#define WRF_CMD	 0xC0
#define NULL_PTR (uint8_t *)0
#define NUL	 0

// Инструкции DAP
#define GQ_CMD 0x2

#define TIMEOUT_OTP 10000

#define OTP_MEMORY 0xbf030000

#define GET_SERVICE_SUBS_URB_OTP_FLAG_BOOT_DONE(x) ((x) & (1 << 1))
#define GET_SERVICE_SUBS_URB_OTP_FLAG_FLAG(x)	   ((x) & (1 << 0))

#define OTP_SIZE 512

// Поля регистра SPI_CTRLR0
#define INST_L_0bit  (0 << 8) // No instructions
#define INST_L_4bit  (1 << 8) // 4 bit instructions
#define INST_L_8bit  (2 << 8) // 8 bit instructions
#define INST_L_16bit (3 << 8) // 16 bit instructions

// Length Address to be transmitted
// 0x - 0xf 4 bits
#define ADDR_L(a) ((a & 0xf) << 2)

#define ADDR_L_0bit  ((0) << 2)
#define ADDR_L_4bit  ((1) << 2)
#define ADDR_L_8bit  ((2) << 2)
#define ADDR_L_12bit ((3) << 2)
#define ADDR_L_16bit ((4) << 2)
#define ADDR_L_20bit ((5) << 2)
#define ADDR_L_24bit ((6) << 2)
#define ADDR_L_28bit ((7) << 2)
#define ADDR_L_32bit ((8) << 2)
#define ADDR_L_36bit ((9) << 2)
#define ADDR_L_40bit ((0xa) << 2)
#define ADDR_L_44bit ((0xb) << 2)
#define ADDR_L_48bit ((0xc) << 2)
#define ADDR_L_52bit ((0xd) << 2)
#define ADDR_L_56bit ((0xe) << 2)
#define ADDR_L_60bit ((0xf) << 2)

// Address and instruction transfer format
// Instructions will be sent in standard SPI mode, Address will de sent in mode specified by CTRLR0.SPI_FRF.
#define TRANS_TYPE_01 0x1
// Both Instruction and Address will be sent in the mode specified by SPI_FRF.
#define TRANS_TYPE_10 0x2

// Addresses PMC registers PROG function
enum addr_pmc_prog {
	ADDR_PROG_MODE_0 = 0x30,
	ADDR_PROG_MODE_1 = 0x32,
	ADDR_PROG_MODE_2 = 0x34,
	ADDR_PROG_MODE_3 = 0x36,
	ADDR_PROG_TIMING_CTRL_0 = 0x38,
	ADDR_PROG_TIMING_CTRL_1 = 0x39,
	ADDR_PROG_TIMING_CTRL_2 = 0x3a,
	ADDR_PROG_DAP_ADDR = 0x3b,
	ADDR_PROG_CQ = 0x3c,
	ADDR_PROG_DFSR = 0x3e,
	ADDR_PROG_CTRL_STATUS = 0x3f
};

// Addresses PMC registers BIST function
enum addr_pmc_bist {
	ADDR_BIST_MODE_0 = 0x30,
	ADDR_BIST_MODE_1 = 0x32,
	ADDR_BIST_MODE_2 = 0x34,
	ADDR_BIST_SIZE = 0x36,
	ADDR_BIST_TIMING_CTRL_0 = 0x38,
	ADDR_BIST_TIMING_CTRL_1 = 0x39,
	ADDR_BIST_DAP_ADDR = 0x3b,
	ADDR_BIST_CQ = 0x3c,
	ADDR_BIST_DFSR = 0x3e,
	ADDR_BIST_CTRL_STATUS = 0x3f
};

// DAP registers addresses
enum addr_dap {
	ADDR_DAP_DR = 0x00,
	ADDR_DAP_ER = 0x20,
	ADDR_DAP_RFMR = 0x30,
	ADDR_DAP_VRMR = 0x31,
	ADDR_DAP_OVLR = 0x32,
	ADDR_DAP_IPCR = 0x33,
	ADDR_DAP_OSCR = 0x34,
	ADDR_DAP_ORCR = 0x35,
	ADDR_DAP_ODCR = 0x36,
	ADDR_DAP_IPCR2 = 0x37,
	ADDR_DAP_OCER = 0x38,
	ADDR_DAP_DPCR = 0x3a,
	ADDR_DAP_DPCR2 = 0x3B,
	ADDR_DAP_OAR = 0x3C,
	ADDR_DAP_OAR_M = 0x3D,
	ADDR_DAP_FP = 0x3E,
	ADDR_DAP_DCSR = 0x3F
};

typedef enum {
	BIST_CLEAN_CHECK = 0,
	BIST_BIT_REPAIR = 1,
} bist_mode_t;

// Поля регистров prog_mode
// PMC регистр рассматривается как 16 разрядный
#define IREF(a)	    (a << 0)
#define ORED(a)	    (a << 5)
#define LD_CP_EN(a) (a << 7)
#define VRRL(a)	    (a << 8)
#define VRRS(a)	    (a << 12)
#define VRRE(a)	    (a << 14)
#define IPSOSCEN(a) (a << 15)

// PROG_TIMING_CTRL_0
#define LRPW(a) (a << 4) //  Long  Read Pulse  Width Control[3:0]
#define SRPW(a) (a << 0) //  Short Read Pulse  Width

// PROG_TIMING_CTRL_1
#define PRC(a)	      (a << 6) // Programming Recovery Control
#define RRC(a)	      (a << 5) // Read Recovery Control
#define CRPWC(a)      (a << 4) // Compare Read Pulse Width Control
#define PSRPWC(a)     (a << 3) // PostSoakReadPulseWidthControl
#define POST_PRPWC(a) (a << 2) // PostProgrammingReadPulseWidthContro
#define PRE_PRPWC(a)  (a << 1) // PreProgrammingReadPulseWidthControl
#define CRPWC_(a)     (a << 0) // BrpCheckReadPulseWidthControl

// PROG_TIMING_CTRL_2
#define MODE_SETTING_TIME_CONTROL(a) (a << 6) // Mode Setting Time
#define SOAK_PULSE_WIDTH_CONTROL(a)  (a << 3) // Soak Pulse Width
#define PROG_PULSE_WIDTH_CONTROL(a)  (a << 0) // Program Pulse Width

// PROG_CQ Register
#define ENTRY(a) (a << 0) // Entry code
#define EXIT(a)	 (a << 5) // Exit code
#define LIMIT(a) (a << 9) // soak cycle limit
#define AINC(a)	 (a << 14) // Use Address Increment

// Поля регистров DAP
#define IREF_(a)    (a << 0)
#define REDUND(a)   (a << 5)
#define LP_CP_EN(a) (a << 7)

#define VPP_LEVEL_Msk(a) (a & 0x7)
#define VPP_LEVEL_Pos(a) (a << 3)
#define VQQ_LEVEL_Msk(a) (a & 0x7)
#define VQQ_LEVEL_Pos(a) (a << 0)
#define OSC_VPP_Pos(a)	 (a << 6)
#define OSC_VQQ_Pos(a)	 (a << 7)

#define VRR_LEVEL(a)		(a << 0)
#define VRR_TEMPERATURE(a)	(a << 4)
#define VRR_REGULATOR_ENABLE(a) (a << 6)
#define CHARGE_PUMP_ENABLE(a)	(a << 7)

// Инструкции PMC
#define BOOT_INST 0x8
#define BIST_INST 0x9
#define PROG_INST 0xa

// Флаги регистра SR status register SSI
#define BUSY (1 << 0)
#define TFNF (1 << 1)
#define TFE  (1 << 2)
#define RFNE (1 << 3)
#define RFF  (1 << 4)

#define NO_VERIFE_ECC 0
#define VERIFE_ECC    1

// Structure programming DAP registers
struct DAP {
	uint32_t DAP_DR; //  Data for program;
	uint8_t DAP_ER; //  ECC protection bits
	uint8_t DAP_RFMR; // Read Mode Control, Charge Pump Control
	uint8_t DAP_VRMR; // Read Voltage Control (VRR), CP enable
	uint8_t DAP_OVLR; // IPS VQQ and VPP control
	uint8_t DAP_IPCR; // VDD detect, Ext. Ref. enable, ISP enable, OSC. Output Mode, Ext Ck enable, Ref Bias Disable
	uint8_t DAP_OSCR; // reserved for Test
	uint8_t DAP_ORCR; // OTP ROM control, Test Mode Controls
	uint8_t DAP_ODCR; // ReadTimerControl;
	uint8_t DAP_IPCR2; // IPS CP sync. Input Control, IPS reserved Control
	uint8_t DAP_OCER; // OTPbankSelection_PDcontrol
	uint8_t Reserved;
	uint8_t DAP_DPCR; //  DATAPATH control;
	uint8_t DAP_DPCR2; // DATAPATH control2;
	unsigned short DAP_OAR; // OTP address;
} __attribute__((packed));

// Structure programming PMC registers
struct PMC {
	unsigned short PROG_MODE_0; // defines for user reads
	unsigned short PROG_MODE_1; // defines for program
	unsigned short PROG_MODE_2; // defines for array clean read
	unsigned short PROG_MODE_3; // not used
	uint8_t PROG_TIMING_CTRL_0; // timing register 0
	uint8_t PROG_TIMING_CTRL_1; // timing register 1
	uint8_t PROG_TIMING_CTRL_2; // timing register 2
	uint8_t PROG_DAP_ADDR; // DAP address = 2
	unsigned short PMC_CQ;
	uint8_t PMC_DFSR;
	uint8_t PMC_CTRL_STATUS; // control / status register PMC
} __attribute__((packed));

typedef enum {
	OTP_OK = 0,
	OTP_ARGS_SIZE_ERR,
	OTP_ARGS_BIN_ERR,
	OTP_ARGS_ADDR_ERR,
	OTP_ARGS_TEST_MODE_ERR,
	OTP_ARGS_ECC_BRP_FLAG_ERR,
	OTP_VERIFY_ERR,
	OTP_VERIFY_ECC_ERR,
	OTP_PROGRAM_ERR,
	OTP_BIST_ERR,
	OTP_WRONG_OP_ERR
} OTP_error_t;

typedef enum {
	OTP_TEST_ROW_DISABLE = 0x0,
	OTP_BIT_LINE_TEST = 0x7, // MR[0:2] = b'111 write NOT allowed
	OTP_BOOT_ROM_TEST = 0x1, // MR[0:2] = b'001 write NOT allowed
	OTP_TEST_ROW_0 = 0x3, // MR[0:2] = b'011 write allowed
	OTP_TEST_ROW_1 = 0x5 // MR[0:2] = b'101 write allowed
} OTP_test_mode_t;

#define ECC_BRP_Msk 0x1B

typedef enum {
	OTP_ECC_BRP_DEFAULT = 0x0, // Оставить значения флагов по умолчанию
	OTP_ECC_DIS = (1 << 0), // Отключение коррекции ECC
	OTP_ECC_GEN = (1 << 1), // Включение генерации ECC
	OTP_BRP_DIS = (1 << 3), // Отключение инвертирования при чтении
	OTP_BRP_GEN = (1 << 4), // Включение инвертирования при записи
} OTP_ecc_brp_flag_t;

void SBPI_getDefaultDwParams(SNPS_SSI_Parameters_s *pParams);
void SBPI_initMaster(SNPS_SSI_regs_t *SSI, SNPS_SSI_Parameters_s *pParams);

void OTP_initForProg();
uint32_t OTP_blankCheck(uint32_t startAddr, uint32_t numBytes);
uint32_t OTP_read(uint32_t startAddr, uint32_t *ptrBuffer, uint32_t numBytes);
uint8_t OTP_readECC(uint32_t addr);
uint32_t OTP_verify(uint32_t startAddr, uint32_t *ptrBuffer, uint32_t numBytes, uint32_t verifECC);
uint32_t OTP_program(uint32_t startAddr, uint32_t *ptrBuffer, unsigned long numBytes);
uint32_t OTP_program(uint32_t startAddr, uint32_t *ptrBuffer, unsigned long numBytes);
uint32_t OTP_bist(uint32_t startAddr, unsigned long numBytes);

void test_rows_enable(uint32_t data);
uint32_t OTP_read_SBPI();
void OTP_initForBoot();
void OTP_initForBist();
void OTP_setEccBrpFlag(uint32_t flag);
#endif // _OTP_H_
