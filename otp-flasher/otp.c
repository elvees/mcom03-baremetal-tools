// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#include <otp.h>
#include <regs.h>

SNPS_SSI_regs_t *SSI;

void SBPI_getDefaultDwParams(SNPS_SSI_Parameters_s *pParams)
{
	if (pParams) {
		pParams->SSI_APBIF_TYPE = 1;
		pParams->SSI_APB3_ERR_RESP_EN = 0;
		pParams->APB_DATA_WIDTH = 32;
		pParams->SSI_IS_MASTER = 1;
		pParams->SSI_ENH_CLK_RATIO = 1;
		pParams->SSI_MAX_XFER_SIZE = 16;
		pParams->SSI_RX_FIFO_DEPTH = 8;
		pParams->SSI_TX_FIFO_DEPTH = 8;
		pParams->SSI_NUM_SLAVES = 1;
		pParams->SSI_HAS_RX_SAMPLE_DELAY = 0;
		pParams->SSI_RX_DLY_SR_DEPTH = 4;
		pParams->SSI_ID = 0xffffffff;
		pParams->SSI_HAS_DMA = 0;
		pParams->SSI_INTR_IO = 1;
		pParams->SSI_INTR_POL = 1;
		pParams->SSI_SYNC_CLK = 1;
		pParams->SSI_CLK_EN_MODE = 0;
		pParams->SSI_HC_FRF = 0;
		pParams->SSI_DFLT_FRF = 0;
		pParams->SSI_DFLT_SCPOL = 0;
		pParams->SSI_DFLT_SCPH = 0;
		pParams->SSI_SCPH0_SSTOGGLE = 1;
		pParams->SSI_SPI_MODE = 3;
		pParams->SSI_IO_MAP_EN = 0;
		pParams->SSI_HAS_DDR = 1;
		pParams->SSI_HAS_RXDS = 0;
		pParams->SSI_SPI_DM_EN = 0;
		pParams->SSI_XIP_EN = 0;
	}
}

/**
 * @brief функция читает флаг завершения выполнения команды Boot
 * @param timeOut
 * @return
 */
static int readFlagBootDone(uint32_t timeOut)
{
	uint32_t timer = 0;
	while (!GET_SERVICE_SUBS_URB_OTP_FLAG_BOOT_DONE(REG(SEVICE_URB_OTP_FLAG))) {
		if (timer++ >= timeOut) {
			return 1;
		}
	}
	return 0;
}

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

void SBPI_initMaster(SNPS_SSI_regs_t *_SSI, SNPS_SSI_Parameters_s *pParams)
{
	SSI = _SSI;
	SNPS_SSI_setDwParams(pParams);
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
			if (!GET_SERVICE_SUBS_URB_OTP_FLAG_FLAG(REG(SEVICE_URB_OTP_FLAG))) {
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
			if (!GET_SERVICE_SUBS_URB_OTP_FLAG_FLAG(REG(SEVICE_URB_OTP_FLAG))) {
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
static inline uint32_t wrf_cmd(uint8_t target, uint32_t address, uint8_t *data, uint32_t size)
{
	uint32_t errors = 0;
	uint8_t tmp = target;
	errors += writeData(INST_SS, tmp, NULL_PTR, NUL, TIMEOUT_OTP);
	tmp = WRF_CMD | (address & 0x3f);
	errors += writeData(DATA_SS, tmp, data, size, TIMEOUT_OTP);
	return errors;
}

/**
 * @brief выполняется команда считывания из slave SPBI
 * @param target PMC, DAP или DATAPATH
 * @param address адрес slave устройства на щине SBPI
 * @param data указатель на массив в памяти в который будут записываться данные считанные с  slave устройства
 * @param size размер отведенной для чтения памяти (максимально число байт которое можно считать)
 */
static void rdf_cmd(uint8_t target, uint32_t address, uint8_t *data, uint32_t size)
{
	uint32_t errors = 0;
	uint8_t tmp = target;
	errors += writeData(INST_SS, tmp, NULL_PTR, NUL, TIMEOUT_OTP);
	tmp = RDF_CMD | (address & 0x3f);
	errors += readData(DATA_SS, tmp, data, size, TIMEOUT_OTP);
}

static void gq_cmd()
{
	writeData(INST_SS, DAP_ID, NULL_PTR, NUL, TIMEOUT_OTP);
	writeDataContinue(DATA_SS, START_CMD, 0xffffff, TIMEOUT_OTP);
}

/**
 * @brief Функция проверяет область памяти на чистоту ( в данном случае на 0 )
 * @param startAddr начальный адрес проверяемой памяти
 * @param numBytes количество байт
 * @return 0 - Ok 1- Error
 */
uint32_t OTP_blankCheck(uint32_t startAddr, uint32_t numBytes)
{
	uint32_t numWords = numBytes / sizeof(uint32_t);
	uint32_t errors = 0;
	uint32_t *OTPaddr = (uint32_t *)(OTP_MEMORY + startAddr); // 0x1f030000 + startAddr;
	REG(SEVICE_URB_OTP_FLAG) = DCTRL_DIRECT;
	for (uint32_t i = 0; i < numWords; i++) {
		if (*OTPaddr != BLANK) {
			errors++;
		}
		OTPaddr++;
	}
	REG(SEVICE_URB_OTP_MODE) = 0; // отключение прямого чтения
	return errors;
}

/**
 * @brief Функция считывает память
 * @param startAddr начальный адрес считываемой памяти
 * @param numBytes количество байт
 *
 */
uint32_t OTP_read(uint32_t startAddr, uint32_t *ptrBuffer, uint32_t numBytes)
{
	uint32_t numWords = numBytes / sizeof(uint32_t);
	uint32_t *OTPaddr = (uint32_t *)(OTP_MEMORY + startAddr); // 0x1f030000 + startAddr;
	readFlagBootDone(TIMEOUT_OTP);
	REG(SEVICE_URB_OTP_MODE) = DCTRL_DIRECT;
	for (uint32_t i = 0; i < numWords; i++) {
		ptrBuffer[i] = *OTPaddr;
		OTPaddr++;
	}

	REG(SEVICE_URB_OTP_MODE) = 0; // отключение прямого чтения

	return OTP_OK;
}

uint32_t OTP_read_SBPI()
{
	uint32_t memQ = 0;

	// исполнение команды GQ
	gq_cmd();
	// чтение данных с DATAPATH
	rdf_cmd(DAP_ID, 0x0, (uint8_t *)&memQ, 4);

	return OTP_OK;
}

/**
 * @brief Функция вычисляет 6 бит кода коррекции ошибки для последующего сравнения с прочитанным значением
 * @param data исходные данные для которых необходимо вычислить код коррекции
 * @return возвращает вычисленный код коррекции ecc 6 bit
 */
static uint8_t calculateECC(uint32_t data)
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

/**
 * @brief Функция возращает значение ECC и BRP для ячейки по указанному адресу
 * @note [5:0] - ecc, [7:6] - brp
 * @param addr адрес ячейки OTP-пямяти
 * @return содержимое otp_ecc
 */
uint8_t OTP_readECC(uint32_t addr)
{
	uint32_t readECC, tmp;
	volatile uint32_t *OTPaddr = (volatile uint32_t *)(OTP_MEMORY + addr);
	REG(SEVICE_URB_OTP_MODE) = DCTRL_DIRECT;
	tmp = *OTPaddr;
	(void)tmp;
	readECC = REG(SEVICE_URB_OTP_ECC);
	REG(SEVICE_URB_OTP_MODE) = 0;

	return (uint8_t)readECC;
}

/**
 * @brief Функция сравнивает ячейки OTP памяти с массивом в оперативной памяти
 * @param startAddr начальная сравниваемая ячейка в OTP памяти
 * @param numBytes количество сравниваемых байт
 * @param ptrBuffer указатель на массив в оперативной памяти
 * @param verifECC проверять ли ECC
 * @retval OTP_VERIFY_ERR Проверка прошла успешно
 * @retval OTP_VERIFY_ERR Данные не совпали
 * @retval OTP_VERIFY_ECC_ERR Данные не совпадают по ECC
 */
uint32_t OTP_verify(uint32_t startAddr, uint32_t *ptrBuffer, uint32_t numBytes, uint32_t verifECC)
{
	uint32_t numWords = numBytes / sizeof(uint32_t);
	uint32_t tmp, calcECC, readECC, errors = 0, ecc_errors = 0;
	uint32_t *OTPaddr = (uint32_t *)(OTP_MEMORY + startAddr); // 0x1f030000 + startAddr;
	REG(SEVICE_URB_OTP_MODE) = DCTRL_DIRECT;
	for (uint32_t i = 0; i < numWords; i++) {
		tmp = *OTPaddr;
		if (tmp != ptrBuffer[i]) {
			errors++;
		}
		if (verifECC) {
			readECC = REG(SEVICE_URB_OTP_ECC);
			calcECC = calculateECC(tmp);
			if (calcECC != readECC) {
				ecc_errors++;
			}
		}
		OTPaddr++;
	}

	REG(SEVICE_URB_OTP_MODE) = 0;

	if (errors != 0) {
		return OTP_VERIFY_ERR;
	}
	if (ecc_errors != 0) {
		return OTP_VERIFY_ECC_ERR;
	}
	return OTP_OK;
}

/**
 * @brief Функция подготавливает PMC и DAP регистры к процедуре программирования
 * @return
 */
void OTP_initForProg()
{
	struct PMC dataPMC2;
	uint32_t tmp;

	// установка режима паралельной работы SBPI, запрещено прямое чтение OTP через AXI, подано питание на OTP
	REG(SEVICE_URB_OTP_MODE) = SP_PARALL;

	// 112'h4000_0254_1100_fc01_f70f_fa12_f801
	dataPMC2.PROG_MODE_0 = 0xf801;
	dataPMC2.PROG_MODE_1 = 0xfa12;
	dataPMC2.PROG_MODE_2 = 0xf70f;
	dataPMC2.PROG_MODE_3 = 0xfc01;

	// Long Read Pulse Width = 2*`0` + 1  clk
	// Short Read Pulse Width = `0` + 11  clk
	dataPMC2.PROG_TIMING_CTRL_0 = 0;
	// Programming Recovery Time = 64 clk
	// Short Read Pulse Width = 1 (long)
	// BrpCheckReadPulseWidth = 1 (long)
	dataPMC2.PROG_TIMING_CTRL_1 = 0x11;
	// Mode Setting Time = 256 clk
	// Soak Pulse Width = 2048 clk
	// Program Pulse Width = 1024 clk
	dataPMC2.PROG_TIMING_CTRL_2 = 0x54;

	dataPMC2.PROG_DAP_ADDR = 2;
	dataPMC2.PMC_CQ = 0x4000;

	// установка MODE и PROG_TIMING
	wrf_cmd(PMC_ID, ADDR_PROG_MODE_0, (uint8_t *)&dataPMC2, 14);
	tmp = PROG_INST; // функция программирования
	wrf_cmd(PMC_ID, ADDR_PROG_CTRL_STATUS, (uint8_t *)&tmp, 1);

	tmp = 0x048c7801;
	wrf_cmd(DAP_ID, 0x30, (uint8_t *)&tmp, 4);
	tmp = 0x01000000;
	wrf_cmd(DAP_ID, 0x34, (uint8_t *)&tmp, 4);
	tmp = 0x08;
	wrf_cmd(DAP_ID, 0x38, (uint8_t *)&tmp, 1);
	tmp = 0x00000400;
	wrf_cmd(DAP_ID, 0x3A, (uint8_t *)&tmp, 4);

	readFlagBootDone(TIMEOUT_OTP);
}

/**
 * @brief Конфигурация регистров PMC и DAP для исполнения процедуры BIST
 * @details необходимо вызывать до функции @ref OTP_BIST
 */
void OTP_initForBist()
{
	uint32_t tmp, i;
	const uint32_t init_size = 14;
	uint32_t rq_cq_dap_bist[4] = { 0x048c7801, 0x01000000, 0x00000008, 0x0000 };
	uint32_t rq_cq_pmc_bist[4] = { 0xf801f801, 0x0080fe01, 0x02004d08, 0x4000 };
	uint8_t *p_dap_bist = (uint8_t *)rq_cq_dap_bist;
	uint8_t *p_pmc_bist = (uint8_t *)rq_cq_pmc_bist;

	// установка режима паралельной работы SBPI запрещено прямое чтение  OTP через AXI
	REG(SEVICE_URB_OTP_MODE) = SP_PARALL;

	// установка MODE и PROG_TIMING
	for (i = 0; i < init_size; i++) {
		wrf_cmd(PMC_ID, 0x30 + i, p_pmc_bist + i, sizeof(uint8_t));
	}
	tmp = BIST_INST; // функция BIST
	wrf_cmd(PMC_ID, ADDR_BIST_CTRL_STATUS, (uint8_t *)&tmp, 1);

	for (i = 0; i < init_size; i++) {
		if (i == 0x9)
			continue;
		wrf_cmd(DAP_ID, 0x30 + i, p_dap_bist + i, sizeof(uint8_t));
	}

	readFlagBootDone(TIMEOUT_OTP);
}

void OTP_initForBoot()
{
	uint32_t tmp, i;
	const uint32_t init_size = 14;
	uint32_t rq_cq_dap_boot[4] = { 0x04097801, 0x00000000, 0x00000008, 0x0000 };
	uint32_t rq_cq_pmc_boot[4] = { 0x780f7801, 0x00007821, 0x0200a201, 0x8000 };
	uint8_t *p_dap_boot = (uint8_t *)rq_cq_dap_boot;
	uint8_t *p_pmc_boot = (uint8_t *)rq_cq_pmc_boot;

	// установка режим паралельной работы SBPI, запрещено прямое чтение  OTP через AXI
	REG(SEVICE_URB_OTP_MODE) = SP_PARALL;

	// BOOT_CQ = BOOT_READ_OTP
	p_pmc_boot[0xC] = ((BOOT_READ_OTP_EXIT & 0x7) << 5) | (BOOT_READ_OTP_ENTRY);
	p_pmc_boot[0xD] = BOOT_READ_OTP_EXIT >> 3;

	// установка MODE и PROG_TIMING
	for (i = 0; i < init_size; i++) {
		wrf_cmd(PMC_ID, 0x30 + i, p_pmc_boot + i, sizeof(uint8_t));
	}
	for (i = 0; i < init_size; i++) {
		if (i == 0x9)
			continue;
		wrf_cmd(DAP_ID, 0x30 + i, p_dap_boot + i, sizeof(uint8_t));
	}

	tmp = BOOT_INST; // функция BOOT
	wrf_cmd(PMC_ID, ADDR_PROG_CTRL_STATUS, (uint8_t *)&tmp, 1);

	readFlagBootDone(TIMEOUT_OTP);
}

/**
 * @brief процедура будет программировать слова в память OTP
 * @details корректно работает только если включен autoInc
 * @param startAddr начальный адрес OTP памяти
 * @param ptrBuffer указатель на область памяти содержащей данные которые необходимо запрограммировать в OTP
 * @param numBytes количество байт которое необходимо запрограммировать
 * @return 0 - Ok 1- ошибка программирования
 */
uint32_t OTP_program(uint32_t startAddr, uint32_t *ptrBuffer, unsigned long numBytes)
{
	uint32_t numWords = numBytes / sizeof(uint32_t);
	uint32_t wordAddr = startAddr / sizeof(uint32_t);
	uint32_t tmp = 0;

	// установка адреса, по которому проводится запись
	wrf_cmd(DAP_ID, ADDR_DAP_OAR, (uint8_t *)&wordAddr, 2);
	for (uint32_t i = 0; i < numWords; i++) {
		// отправка данных на запись
		wrf_cmd(DAP_ID, ADDR_DAP_DR, (uint8_t *)&ptrBuffer[i], 4);
		start_cmd(PMC_ID);
		//  чтение статуса
		do {
			rdf_cmd(PMC_ID, ADDR_PROG_CTRL_STATUS, (uint8_t *)&tmp, 1);
		} while ((tmp & 0x40) == 0);
		stop_cmd(PMC_ID);
		wordAddr++;
		if (tmp != 0x40) {
			uint32_t addr_err = 0;
			rdf_cmd(DAP_ID, ADDR_DAP_OAR, (uint8_t *)&addr_err, 2);
			return OTP_PROGRAM_ERR;
		}
	}
	return OTP_OK;
}

/**
 * @brief процедура выполняет операцию проверки массива памяти OTP (BIST)
 * @param startAddr начальный адрес OTP памяти
 * @param numBytes количество байт которое необходимо проверить
 * @return 0 - Ok 1- ошибка
 */
uint32_t OTP_bist(uint32_t startAddr, unsigned long numBytes)
{
	uint32_t numWords = numBytes / sizeof(uint32_t);
	uint32_t wordAddr = startAddr / sizeof(uint32_t);
	uint32_t tmp = 0;

	// начальный адрес, с которого будет проводиться проверка
	wrf_cmd(DAP_ID, ADDR_DAP_OAR, (uint8_t *)&wordAddr, 2);

	// количество слов для тестирования
	wrf_cmd(PMC_ID, ADDR_BIST_SIZE, (uint8_t *)&numWords, 2);
	start_cmd(PMC_ID);
	//  чтение статуса
	do {
		rdf_cmd(PMC_ID, ADDR_PROG_CTRL_STATUS, (uint8_t *)&tmp, 1);
	} while ((tmp & 0x40) == 0);
	stop_cmd(PMC_ID);
	if (tmp != 0x40) {
		return OTP_BIST_ERR;
	}

	return OTP_OK;
}

void OTP_setEccBrpFlag(uint32_t flag)
{
	uint8_t reg_value = 0;

	rdf_cmd(DAP_ID, 0x3A, &reg_value, 1);
	reg_value &= ~ECC_BRP_Msk;
	reg_value |= flag;
	wrf_cmd(DAP_ID, 0x3A, &reg_value, 1);
}

void test_rows_enable(uint32_t data)
{
	uint8_t reg_value = 0;

	rdf_cmd(DAP_ID, 0x35, &reg_value, 1);
	reg_value &= 0x7;
	reg_value |= data;
	wrf_cmd(DAP_ID, 0x35, &reg_value, 1);
}
