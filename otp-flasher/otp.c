// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#include <otp.h>
#include <regs.h>

#define PMC_MODE_0	  0x30
#define PMC_MODE_1	  0x32
#define PMC_MODE_2	  0x34
#define PMC_MODE_3	  0x36
#define PMC_TIMING_CTRL_0 0x38
#define PMC_TIMING_CTRL_1 0x39
#define PMC_TIMING_CTRL_2 0x3a
#define PMC_DAP_ADDR	  0x3b
#define PMC_CQ		  0x3c
#define PMC_DFSR	  0x3e
#define PMC_CTRL_STATUS	  0x3f

#define DAP_DR	  0x00
#define DAP_ER	  0x20
#define DAP_RFMR  0x30
#define DAP_VRMR  0x31
#define DAP_OVLR  0x32
#define DAP_IPCR  0x33
#define DAP_OSCR  0x34
#define DAP_ORCR  0x35
#define DAP_ODCR  0x36
#define DAP_IPCR2 0x37
#define DAP_OCER  0x38
#define DAP_DPCR  0x3a
#define DAP_DPCR2 0x3B
#define DAP_OAR	  0x3C
#define DAP_OAR_M 0x3D
#define DAP_FP	  0x3E
#define DAP_DCSR  0x3F

#define OTP_MODE_SP_SERIAL    BIT(0)
#define OTP_MODE_DCTRL_DIRECT BIT(1)
#define OTP_MODE_PD_OFF	      BIT(2)

#define INST_SS		 0x2
#define DATA_SS		 0x1
#define NUM_BYTE_SL_ADDR 0x1
#define PMC_ID		 0x3A
#define DAP_ID		 0x2

// Инструкции PMC
#define BOOT_INST 0x8
#define BIST_INST 0x9
#define PROG_INST 0xa

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

#define GET_SERVICE_SUBS_URB_OTP_FLAG_BOOT_DONE(x) ((x) & (1 << 1))
#define GET_SERVICE_SUBS_URB_OTP_FLAG_FLAG(x)	   ((x) & (1 << 0))

// Поля регистра SPI_CTRLR0
#define INST_L_0bit  (0 << 8) // No instructions
#define INST_L_4bit  (1 << 8) // 4 bit instructions
#define INST_L_8bit  (2 << 8) // 8 bit instructions
#define INST_L_16bit (3 << 8) // 16 bit instructions

// Флаги регистра SR status register SSI
#define BUSY (1 << 0)
#define TFNF (1 << 1)
#define TFE  (1 << 2)
#define RFNE (1 << 3)
#define RFF  (1 << 4)

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

union dap_regs {
	struct {
		struct {
			uint8_t iref : 5;
			uint8_t redund : 2;
			uint8_t ld_cp_en : 1;
		};
		struct {
			uint8_t vrr : 4;
			uint8_t vrrts : 2;
			uint8_t vrr_en : 1;
			uint8_t cp_en : 1;
		};
		struct {
			uint8_t vqq : 3;
			uint8_t vpp : 3;
			uint8_t ipsoscvpp : 1;
			uint8_t ipsoscvqq : 1;
		};
		struct {
			uint8_t vdd_det_dis : 1;
			uint8_t ext_ref_en : 1;
			uint8_t ips_en : 1;
			uint8_t osc_out : 2;
			uint8_t ext_ck_en : 1;
			uint8_t ref_bias_dis : 1;
			uint8_t vrrswc : 1;
		};
		struct {
			uint8_t b : 4;
			uint8_t m : 4;
		};
		struct {
			uint8_t romen : 1;
			uint8_t arom : 2;
			uint8_t otp_rtst : 1;
			uint8_t ta : 1;
			uint8_t : 3;
		};
		struct {
			uint8_t ckdel : 5;
			uint8_t : 3;
		};
		struct {
			uint8_t we_ck : 1;
			uint8_t vreflvl : 3;
			uint8_t : 4;
		};
		struct {
			uint8_t data_bank_all : 1;
			uint8_t power_down : 1;
			uint8_t all : 1;
			uint8_t sel : 1;
			uint8_t bank_addr : 3;
			uint8_t w : 1;
		};
		struct {
			uint8_t : 8;
		};
		union {
			struct {
				uint8_t ecc_dis : 1;
				uint8_t ecc_gen : 1;
				uint8_t ecc_tst : 1;
				uint8_t brp_dis : 1;
				uint8_t brp_gen : 1;
				uint8_t pass : 1;
				uint8_t muxq : 2;
			};
			uint8_t dpcr;
		};
		struct {
			uint8_t mbpc : 3;
			uint8_t : 5;
		};
		uint16_t oar;
		uint8_t fp;
		struct {
			uint8_t ctrl : 3;
			uint8_t ctrl_en : 1;
			uint8_t fadd : 3;
			uint8_t fadd_en : 1;
		};
	};
	struct {
		union {
			uint8_t rq_bytes[12];
			uint16_t rq_words[6];
		};
		union {
			uint8_t cq_bytes[2];
			uint16_t cq_word;
		};
		union {
			uint8_t ctrl_bytes[2];
			uint16_t ctrl_word;
		};
	};
	uint8_t bytes[16];
};

union pmc_mode {
	struct {
		uint16_t iref : 5;
		uint16_t ored : 2;
		uint16_t ld_cp_en : 1;
		uint16_t vrrl : 4;
		uint16_t vrrs : 2;
		uint16_t vrre : 1;
		uint16_t ipsoscen : 1;
	};
	uint16_t value;
};

union pmc_regs {
	struct {
		union pmc_mode mode0;
		union pmc_mode mode1;
		union pmc_mode mode2;
		union {
			union pmc_mode mode3;
			uint16_t bist_size;
			uint16_t boot_addr;
		};
		uint8_t timing_ctrl0;
		uint8_t timing_ctrl1;
		uint8_t timing_ctrl2;
		uint8_t dap_addr;
		union {
			uint16_t cq;
			struct {
				uint16_t entry : 5;
				uint16_t exitn : 5;
				uint16_t limitn : 4;
				uint16_t ainc : 1;
				uint16_t : 1;
			} cq_prog;
			struct {
				uint16_t entry : 4;
				uint16_t exitn : 4;
				uint16_t : 6;
				uint16_t program_enable : 1;
				uint16_t : 1;
			} cq_bist;
		};
		uint8_t dfsr;
		uint8_t ctrl_status;
	};
	uint8_t bytes[16];
};

SNPS_SSI_regs_t *SSI;

/**
 * @brief Функция первоначальной настройки SSI
 * @details Функция
 * @return 0 - Ok 1 - error
 */
static int setControlRegister0(void)
{
	SNPS_SSI_setToggleSS(SSI, 0);
	if (SNPS_SSI_setDataFrameSize(SSI, FRAME_08BITS)) {
		return 1;
	}
	SNPS_SSI_setRegisterLoop(SSI, 0);
	SNPS_SSI_setTransferMode(SSI, RX_ONLY);
	SNPS_SSI_setClockPolarity(SSI, SCLK_LOW);
	SNPS_SSI_setClockPhase(SSI, SCPH_MIDDLE);
	SNPS_SSI_setSpiFrameFormat(SSI, OCTAL_SPI_FRF);
	if (SNPS_SSI_setSsiFrameFormat(SSI, MOTOROLA_SPI)) {
		return 1;
	}
	return 0;
}

/*! @brief Функция настройки SSI контроллера как мастера
 *  @details Настраивает частоту клока, запрет всех прерываний
 *  @return
 */
static void SBPI_configureMaster()
{
	SNPS_SSI_disableSsi(SSI);
	if (setControlRegister0()) {
		SNPS_SSI_disableSsi(SSI);
		return;
	}
	SNPS_SSI_setBaudrateClkDivider(SSI, 2);
	SNPS_SSI_setTxFifoThrLevel(SSI, 7);
	SNPS_SSI_setRxFifoThrLevel(SSI, 7);

	/* IMR related */
	SNPS_SSI_setIrqMask(SSI, all_m);
	SNPS_SSI_clearInterrupts(SSI, all_m);
	return;
}

void SBPI_initMaster(SNPS_SSI_regs_t *_SSI)
{
	SNPS_SSI_Parameters_s params = {
		.SSI_APBIF_TYPE = 1,
		.SSI_APB3_ERR_RESP_EN = 0,
		.APB_DATA_WIDTH = 32,
		.SSI_IS_MASTER = 1,
		.SSI_ENH_CLK_RATIO = 1,
		.SSI_MAX_XFER_SIZE = 16,
		.SSI_RX_FIFO_DEPTH = 8,
		.SSI_TX_FIFO_DEPTH = 8,
		.SSI_NUM_SLAVES = 1,
		.SSI_HAS_RX_SAMPLE_DELAY = 0,
		.SSI_RX_DLY_SR_DEPTH = 4,
		.SSI_ID = 0xffffffff,
		.SSI_HAS_DMA = 0,
		.SSI_INTR_IO = 1,
		.SSI_INTR_POL = 1,
		.SSI_SYNC_CLK = 1,
		.SSI_CLK_EN_MODE = 0,
		.SSI_HC_FRF = 0,
		.SSI_DFLT_FRF = 0,
		.SSI_DFLT_SCPOL = 0,
		.SSI_DFLT_SCPH = 0,
		.SSI_SCPH0_SSTOGGLE = 1,
		.SSI_SPI_MODE = 3,
		.SSI_IO_MAP_EN = 0,
		.SSI_HAS_DDR = 1,
		.SSI_HAS_RXDS = 0,
		.SSI_SPI_DM_EN = 0,
		.SSI_XIP_EN = 0,
	};

	SSI = _SSI;
	SNPS_SSI_setDwParams(&params);
	SBPI_configureMaster();
	SSI->SPI_CTRLR0 = INST_L_8bit | ADDR_L_0bit | TRANS_TYPE_10;
}

/**
 * @brief Функция считывания данных из PVT, IPS, DAP,
 * @param ser выбор сигнала slave select
 * @param cmd  код команды
 * @param buff указатель на область памяти куда будут складываться считанные данные
 * @param size размер выделенной памяти
 * @param timeout время ожидания завершения операции
 * @return
 */
static uint32_t readData(uint8_t ser, uint8_t cmd, uint8_t *buff, unsigned long size,
			 uint32_t timeout)
{
	unsigned tmp1;
	uint32_t sr;

	// ожидание завершения передачи
	tmp1 = timeout;
	while ((SSI->SR & BUSY)) {
		if (!tmp1--) {
			return 1;
		}
	}
	// отключение контроллера и перенастройка
	// настройка формата данных
	SNPS_SSI_disableSsi(SSI);
	SSI->SPI_CTRLR0 = INST_L_8bit | ADDR_L_0bit | TRANS_TYPE_10;
	SNPS_SSI_setTransferMode(SSI, RX_ONLY);
	// установка количества байт на считывание
	SSI->CTRLR1 = size;
	SNPS_SSI_enableSsi(SSI);
	SSI->SER = ser;
	uint32_t i;
	for (i = 0; i < size + 1; i++) {
		tmp1 = timeout;
		// проверка: FIFO не полный
		do {
			sr = SSI->SR;
			if (!tmp1--) {
				return 1;
			}
		} while (!(sr & TFNF));
		// запись данных в буфер
		if (i == 0) {
			SSI->DR[0] = cmd;
			// ожидание освобождения буфера
			while (!(SSI->SR & TFE) || (SSI->SR & BUSY)) {
				if (!tmp1--) {
					return 1;
				}
			}
		} else {
			// ожидание нового байта
			tmp1 = timeout;
			while (!(SSI->SR & RFNE)) {
				if (!tmp1--) {
					return 1;
				}
			}
			buff[i - 1] = SNPS_SSI_readData(SSI);
			if (!GET_SERVICE_SUBS_URB_OTP_FLAG_FLAG(REG(SERVICE_URB_OTP_FLAG))) {
				break;
			}
		}
	}
	return 0;
}

/**
 * @brief процедура посылает код команды по SBPI и далее продолжает посылать по шине синхросигнал,
 * так как вся схема синхронизируется от тактового сигнала интерфеса SBPI
 * @param ser выбор сигнала slave select
 * @param cmd первый байт посылки
 * @param size количество dummy байт которые будут посланы после команды
 * @param timeout время ожидания завершения операции
 * @return 0-Ok >1-произошла ошибка
 */
static uint32_t writeDataContinue(uint8_t ser, uint8_t cmd, unsigned long size, uint32_t timeout)
{
	uint32_t errors = 0;
	uint32_t tmp1;
	uint32_t sr;

	// ожидание завершения передачи
	tmp1 = timeout;
	while ((SSI->SR & BUSY)) {
		if (!tmp1--) {
			return 1;
		}
	}

	// отключение контроллера и перенастройка
	// необходимо настроить формат данных
	SNPS_SSI_disableSsi(SSI);
	SSI->SPI_CTRLR0 = INST_L_16bit | ADDR_L_0bit | TRANS_TYPE_10;
	SNPS_SSI_setTransferMode(SSI, TX_ONLY);
	SSI->CTRLR1 = 0;
	SNPS_SSI_enableSsi(SSI);
	SSI->SER = ser;
	uint32_t i;
	for (i = 0; i < size; i++) {
		tmp1 = timeout;
		// проверка: FIFO не полный
		do {
			sr = SSI->SR;
			if (!tmp1--) {
				return 1;
			}
		} while (!(sr & TFNF));

		// запись данных в буфер
		if (i == 0) {
			SSI->DR[0] = (cmd << 8) &
				     0xff00; // так как выдвигается старшим разрядом вперед
		} else {
			SSI->DR[0] = NOP_CMD;
			if (!GET_SERVICE_SUBS_URB_OTP_FLAG_FLAG(REG(SERVICE_URB_OTP_FLAG))) {
				break;
			}
		}
	}
	tmp1 = timeout;
	// проверка окончания передачи
	do {
		sr = SSI->SR;
		if (!tmp1--) {
			return 1;
		}
	} while ((!(sr & TFE)) || (sr & BUSY));

	SSI->SER = 0;
	if (i == size) {
		errors++;
	}
	return errors;
}

/**
 * @brief процедура передает массив байт
 * @details
 * @param cmd первый байт в посылке
 * @param ser выбор сигнала slave select
 * @param data указатель на массив передаваемых данных
 * @param size количество байт в массиве в data которые необходимо передать
 * @param timeout время ожидания выполнения операции
 * @return возвращает 1 если произошел timeout  и 0 успешное завершение
 */
static int writeData(uint32_t ser, uint8_t cmd, uint8_t *data, uint32_t size, int timeout)
{
	unsigned tmp1;
	uint32_t sr;

	// ожидание завершения передачи
	tmp1 = timeout;
	while ((SSI->SR & BUSY)) {
		if (!tmp1--) {
			return 1;
		}
	}
	// отключение контроллера и перенастройка
	// необходимо настроить формат данных
	SNPS_SSI_disableSsi(SSI);
	SSI->SPI_CTRLR0 = INST_L_8bit | ADDR_L_0bit | TRANS_TYPE_10;
	SNPS_SSI_setTransferMode(SSI, TX_ONLY);
	SSI->CTRLR1 = 0;
	SNPS_SSI_enableSsi(SSI);
	SSI->SER = ser;
	for (uint32_t i = 0; i < size + 1; i++) {
		tmp1 = timeout;
		// проверка: FIFO не полный
		do {
			sr = SSI->SR;
			if (!tmp1--) {
				return 1;
			}
		} while (!(sr & TFNF));

		// запись данных в буфер
		if (i == 0) {
			SSI->DR[0] = cmd;
		} else {
			SSI->DR[0] = data[i - 1];
		}
	}
	tmp1 = timeout;
	// проверка окончания передачи
	do {
		sr = SSI->SR;
		if (!tmp1--) {
			return 1;
		}
	} while ((!(sr & TFE)) || (sr & BUSY));

	SSI->SER = 0;
	return 0;
}

/**
 * @brief Исполнение короткой команды
 * @param target PMC, DAP или DATAPATH
 * @param cmd код выполняемой команды
 * @return
 */
static inline void shortFrame(uint8_t target, uint8_t cmd)
{
	writeData(INST_SS, target, NULL_PTR, NUL, TIMEOUT_OTP);
	writeData(DATA_SS, cmd, NULL_PTR, NUL, TIMEOUT_OTP);
}

/**
 * @brief Запуск команды
 * @param target PMC, DAP или DATAPATH
 */
static inline void start_cmd(uint32_t target)
{
	writeData(INST_SS, target, NULL_PTR, NUL, TIMEOUT_OTP);
	writeDataContinue(DATA_SS, START_CMD, 0xffffff, TIMEOUT_OTP);
}

/**
 * @brief Остановка команды PMC
 * @param target PMC, DAP или DATAPATH
*/
static inline void stop_cmd(uint32_t target)
{
	shortFrame(target, STOP_CMD);
}

/**
 * @brief Исполнение команда записи в регистр в slave SPBI
 * @param target PMC, DAP или DATAPATH
 * @param address адрес slave устройства на шине SBPI
 * @param data указатель на массив данных который будет передан slave устройству
 * @param size размер массива который надо записать (передать)
 * @return
 */
static inline int wrf_cmd(uint8_t target, uint32_t address, uint8_t *data, uint32_t size)
{
	uint32_t errors = 0;
	uint8_t tmp = target;

	errors += writeData(INST_SS, tmp, NULL_PTR, NUL, TIMEOUT_OTP);
	tmp = WRF_CMD | (address & 0x3f);
	errors += writeData(DATA_SS, tmp, data, size, TIMEOUT_OTP);

	return errors ? OTP_ERR_BUS : 0;
}

/**
 * @brief выполняется команда считывания из slave SPBI
 * @param target PMC, DAP или DATAPATH
 * @param address адрес slave устройства на щине SBPI
 * @param data указатель на массив в памяти в который будут записываться данные считанные с  slave устройства
 * @param size размер отведенной для чтения памяти (максимально число байт которое можно считать)
 */
static int rdf_cmd(uint8_t target, uint32_t address, uint8_t *data, uint32_t size)
{
	uint32_t errors = 0;
	uint8_t tmp = target;

	errors += writeData(INST_SS, tmp, NULL_PTR, NUL, TIMEOUT_OTP);
	tmp = RDF_CMD | (address & 0x3f);
	errors += readData(DATA_SS, tmp, data, size, TIMEOUT_OTP);

	return errors ? OTP_ERR_BUS : 0;
}

/**
 * @brief Функция вычисляет 6 бит кода коррекции ошибки для последующего сравнения с прочитанным значением
 * @param data исходные данные для которых необходимо вычислить код коррекции
 * @return возвращает вычисленный код коррекции ecc 6 bit
 */
uint8_t otp_calculate_ecc(uint32_t data)
{
	uint32_t dataTmp;
	uint32_t ecc = 0;
	const uint32_t P[6] = { 0x56aaad5b, 0x9b33366d, 0xe3c3c78e,
				0x03fc07f0, 0x03fff800, 0xfc000000 };

	for (uint32_t i = 0; i < 6; i++) {
		dataTmp = data & P[i];
		dataTmp ^= (dataTmp >> 16);
		dataTmp ^= (dataTmp >> 8);
		dataTmp ^= (dataTmp >> 4);
		dataTmp &= 0xf;
		ecc |= ((0x6996 >> dataTmp) & 1) << i;
	}

	return ecc;
}

static inline void otp_set_eccbrp_flags(uint8_t flag)
{
	uint8_t reg_value = 0;

	rdf_cmd(DAP_ID, DAP_DPCR, &reg_value, 1);
	reg_value &= ~OTP_FLAG_MASK;
	reg_value |= flag;
	wrf_cmd(DAP_ID, DAP_DPCR, &reg_value, 1);
}

static inline void otp_set_mode_direct_read(void)
{
	REG(SERVICE_URB_OTP_MODE) = OTP_MODE_DCTRL_DIRECT;
	__sync_synchronize();
}

static inline void otp_set_mode_sbpi(void)
{
	// установка режима паралельной работы SBPI, запрещено прямое чтение OTP через AXI, подано питание на OTP
	REG(SERVICE_URB_OTP_MODE) = 0;
	__sync_synchronize();
}

void otp_read(uint32_t *buf_data, uint8_t *buf_ecc, uint32_t otp_addr, uint32_t count, uint8_t flags)
{
	otp_set_mode_sbpi();
	otp_set_eccbrp_flags(flags);
	otp_set_mode_direct_read();
	for (uint32_t i = 0; i < count; i++) {
		buf_data[i] = REG(SERVICE_OTP + ((otp_addr + i) * 4));
		buf_ecc[i] = REG(SERVICE_URB_OTP_ECC);
	}

	otp_set_mode_sbpi();
}

/**
 * @brief процедура будет программировать слова в память OTP
 * @details корректно работает только если включен autoInc
 * @param buffer указатель на область памяти содержащей данные которые необходимо запрограммировать в OTP
 * @param ecc указатель на область памяти содержащей ECC которые необходимо запрограммировать в OTP.
 *            Если NULL, то ECC будет рассчитываться автоматически.
 * @param otp_addr начальный адрес OTP памяти (в диапазоне 0..128)
 * @param count количество слов, которое необходимо запрограммировать (в диапазоне 1..128)
 * @param err_addr в случае ошибки сюда будет записан адрес слова OTP, на котором произошла ошибка
 * @return 0 - Ok, отрацательное значение - ошибка программирования
 */
int otp_program(uint32_t *buffer, uint8_t *ecc, uint16_t otp_addr, uint32_t count, uint16_t *err_addr)
{
	uint8_t status = 0;
	union dap_regs dap = {
		.iref = 1,
		.vrr = 8,
		.vrrts = 3,
		.vrr_en = 1,
		.vqq = 4,
		.vpp = 1,
		.ipsoscvqq = 1,
		.ips_en = 1,
		.we_ck = 1,
		.sel = 1,
		.mbpc = 4,
		.oar = otp_addr,
	};
	union pmc_regs pmc = {
		.mode0.iref = 1,
		.mode0.vrrl = 8,
		.mode0.vrrs = 3,
		.mode0.vrre = 1,
		.mode0.ipsoscen = 1,
		.mode1.iref = 18, // IREF soak settings
		.mode1.vrrl = 10,
		.mode1.vrrs = 3,
		.mode1.vrre = 1,
		.mode1.ipsoscen = 1,
		.mode2.iref = 15, // IREF verify settings
		.mode2.vrrl = 7,
		.mode2.vrrs = 3,
		.mode2.vrre = 1,
		.mode2.ipsoscen = 1,
		.mode3.iref = 1,
		.mode3.vrrl = 0xc,
		.mode3.vrrs = 3,
		.mode3.vrre = 1,
		.mode3.ipsoscen = 1,
		.timing_ctrl0 = 0,
		.timing_ctrl1 = 0x11,
		.timing_ctrl2 = 0x54,
		.dap_addr = 0x2,
		.cq_prog.ainc = 1,
		.cq_prog.entry = ecc ? 4 : 0,
		.ctrl_status = PROG_INST,
	};

	otp_set_mode_sbpi();

	if (wrf_cmd(PMC_ID, 0x30, pmc.bytes, sizeof(pmc)))
		return OTP_ERR_BUS;

	if (wrf_cmd(DAP_ID, 0x30, dap.bytes, sizeof(dap)))
		return OTP_ERR_BUS;

	for (uint32_t i = 0; i < count; i++) {
		if (wrf_cmd(DAP_ID, DAP_DR, (uint8_t *)&buffer[i], 4))
			return OTP_ERR_BUS;

		if (ecc) {
			if (wrf_cmd(DAP_ID, 0x20, &ecc[i], 1))
				return OTP_ERR_BUS;
		}

		start_cmd(PMC_ID);
		do {
			if (rdf_cmd(PMC_ID, PMC_CTRL_STATUS, (uint8_t *)&status, 1))
				return OTP_ERR_BUS;
		} while ((status & 0xc0) != 0x40);

		stop_cmd(PMC_ID);
		status &= 0x30;
		if (status) {
			if (err_addr) {
				if (rdf_cmd(DAP_ID, DAP_OAR, (uint8_t *)err_addr, 2))
					return OTP_ERR_BUS;
			}

			if (status == 0x10)
				return OTP_ERR_PROG_SOAK_LIMIT;
			else if (status == 0x20)
				return OTP_ERR_PROG_COMPARE;

			return OTP_ERR;
		}
	}

	return 0;
}

/**
 * @brief процедура выполняет операцию проверки/проверки с исправлением массива памяти OTP (BIST)
 * @param otp_addr начальный адрес OTP памяти (в диапазоне 0..128)
 * @param count количество слов, которое необходимо проверить (в диапазоне 1..128)
 * @param is_bisr если не равно нулю, то проверять с исправлением
 * @param err_addr в случае ошибки сюда будет записан адрес слова OTP, на котором произошла ошибка
 * @return 0 - Ok, отрицательное значение - ошибка
 */
int otp_bist(uint16_t otp_addr, uint16_t count, int is_bisr, uint16_t *err_addr)
{
	uint8_t status = 0;
	union dap_regs dap = {
		.iref = 1,
		.vrr = 8,
		.vrrts = 3,
		.vrr_en = 1,
		.vqq = 4,
		.vpp = 1,
		.ipsoscvqq = 1,
		.ips_en = 1,
		.we_ck = 1,
		.sel = 1,
		.oar = otp_addr,
	};
	union pmc_regs pmc = {
		.mode0.iref = 1,
		.mode0.vrrl = 8,
		.mode0.vrrs = 3,
		.mode0.vrre = 1,
		.mode0.ipsoscen = 1,
		.mode1.iref = 1,
		.mode1.vrrl = 8,
		.mode1.vrrs = 3,
		.mode1.vrre = 1,
		.mode1.ipsoscen = 1,
		.mode2.iref = 1,
		.mode2.vrrl = 0xe,
		.mode2.vrrs = 3,
		.mode2.vrre = 1,
		.mode2.ipsoscen = 1,
		.bist_size = 0x80,
		.timing_ctrl0 = 0x8,
		.timing_ctrl1 = 0x4d,
		.timing_ctrl2 = 0,
		.dap_addr = 0x2,
		.ctrl_status = BIST_INST,
		.bist_size = count,
		.cq_bist.program_enable = !!is_bisr,
	};

	otp_set_mode_sbpi();

	if (wrf_cmd(PMC_ID, 0x30, pmc.bytes, sizeof(pmc)))
		return -1;

	if (wrf_cmd(DAP_ID, 0x30, dap.bytes, sizeof(dap)))
		return -1;

	start_cmd(PMC_ID);
	do {
		if (rdf_cmd(PMC_ID, PMC_CTRL_STATUS, &status, 1))
			return OTP_ERR_BUS;
	} while ((status & 0xc0) != 0x40);

	stop_cmd(PMC_ID);
	if (status & 0x30) {
		if (err_addr) {
			if (rdf_cmd(DAP_ID, DAP_OAR, (uint8_t *)err_addr, 2))
				return OTP_ERR_BUS;
		}

		return OTP_ERR;
	}

	return 0;
}
