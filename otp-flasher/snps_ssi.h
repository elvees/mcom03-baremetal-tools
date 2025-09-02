// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#ifndef _SNPS_SSI_H_
#define _SNPS_SSI_H_

#include <stdint.h>
#include <regs.h>

#define SPI_OTP (SNPS_SSI_regs_t *)(TO_VIRT(SPI_OTP_BASE))

typedef struct {
	volatile uint32_t CTRLR0;
	volatile uint32_t CTRLR1;
	volatile uint32_t SSIENR;
	volatile uint32_t MWCR;
	volatile uint32_t SER;
	volatile uint32_t BAUDR;
	volatile uint32_t TXFTLR;
	volatile uint32_t RXFTLR;
	volatile uint32_t TXFLR;
	volatile uint32_t RXFLR;
	volatile uint32_t SR;
	volatile uint32_t IMR;
	volatile uint32_t ISR;
	volatile uint32_t RISR;
	volatile uint32_t TXOICR;
	volatile uint32_t RXOICR;
	volatile uint32_t RXUICR;
	volatile uint32_t MSTICR;
	volatile uint32_t ICR;
	volatile uint32_t DMACR;
	volatile uint32_t DMATDLR;
	volatile uint32_t DMARDLR;
	volatile uint32_t IDR;
	volatile uint32_t SSI_VERSION_ID;
	volatile uint32_t DR[36];
	volatile uint32_t RX_SAMPLE_DLY;
	volatile uint32_t SPI_CTRLR0;
} SNPS_SSI_regs_t;

typedef struct DW_apb_ssi_Parameters_s {
	uint32_t SSI_APBIF_TYPE;
	uint32_t SSI_APB3_ERR_RESP_EN;
	uint32_t APB_DATA_WIDTH;
	uint32_t SSI_IS_MASTER;
	uint32_t SSI_ENH_CLK_RATIO;
	uint32_t SSI_MAX_XFER_SIZE;
	uint32_t SSI_RX_FIFO_DEPTH;
	uint32_t SSI_TX_FIFO_DEPTH;
	uint32_t SSI_NUM_SLAVES;
	uint32_t SSI_HAS_RX_SAMPLE_DELAY;
	uint32_t SSI_RX_DLY_SR_DEPTH;
	uint32_t SSI_ID;
	uint32_t SSI_HAS_DMA;
	uint32_t SSI_INTR_IO;
	uint32_t SSI_INTR_POL;
	uint32_t SSI_SYNC_CLK;
	uint32_t SSI_CLK_EN_MODE;
	uint32_t SSI_HC_FRF;
	uint32_t SSI_DFLT_FRF;
	uint32_t SSI_DFLT_SCPOL;
	uint32_t SSI_DFLT_SCPH;
	uint32_t SSI_SCPH0_SSTOGGLE;
	uint32_t SSI_SPI_MODE;
	uint32_t SSI_IO_MAP_EN;
	uint32_t SSI_HAS_DDR;
	uint32_t SSI_HAS_RXDS;
	uint32_t SSI_SPI_DM_EN;
	uint32_t SSI_XIP_EN;
} SNPS_SSI_Parameters_s;

/*! @brief Data frame size encoding
 *  @ingroup Group_SNPS_SSI
 */
typedef enum {
	FRAME_04BITS = 0x3, ///<  4 bits data frame size
	FRAME_05BITS = 0x4, ///<  5 bits data frame size
	FRAME_06BITS = 0x5, ///<  6 bits data frame size
	FRAME_07BITS = 0x6, ///<  7 bits data frame size
	FRAME_08BITS = 0x7, ///<  8 bits data frame size
	FRAME_09BITS = 0x8, ///<  9 bits data frame size
	FRAME_10BITS = 0x9, ///< 10 bits data frame size
	FRAME_11BITS = 0xa, ///< 11 bits data frame size
	FRAME_12BITS = 0xb, ///< 12 bits data frame size
	FRAME_13BITS = 0xc, ///< 13 bits data frame size
	FRAME_14BITS = 0xd, ///< 14 bits data frame size
	FRAME_15BITS = 0xe, ///< 15 bits data frame size
	FRAME_16BITS = 0xf, ///< 16 bits data frame size
	FRAME_17BITS = 0x10, ///< 17 bits data frame size
	FRAME_18BITS = 0x11, ///< 18 bits data frame size
	FRAME_19BITS = 0x12, ///< 19 bits data frame size
	FRAME_20BITS = 0x13, ///< 20 bits data frame size
	FRAME_21BITS = 0x14, ///< 21 bits data frame size
	FRAME_22BITS = 0x15, ///< 22 bits data frame size
	FRAME_23BITS = 0x16, ///< 23 bits data frame size
	FRAME_24BITS = 0x17, ///< 24 bits data frame size
	FRAME_25BITS = 0x18, ///< 25 bits data frame size
	FRAME_26BITS = 0x19, ///< 26 bits data frame size
	FRAME_27BITS = 0x1a, ///< 27 bits data frame size
	FRAME_28BITS = 0x1b, ///< 28 bits data frame size
	FRAME_29BITS = 0x1c, ///< 29 bits data frame size
	FRAME_30BITS = 0x1d, ///< 30 bits data frame size
	FRAME_31BITS = 0x1e, ///< 31 bits data frame size
	FRAME_32BITS = 0x1f, ///< 32 bits data frame size
} SNPS_SSI_DataFrameSize_e;

/*!
 * @brief Motorola SPI Frame format. i.e. number of spi data lines
 *        for simultaneous data transmission
 * @ingroup Group_SNPS_SSI
 */
typedef enum {
	STD_SPI_FRF = 0x0, ///< 1 data line
	DUAL_SPI_FRF = 0x1, ///< 2 data lines
	QUAD_SPI_FRF = 0x2, ///< 4 data lines
	OCTAL_SPI_FRF = 0x3, ///< 8 data lines
} SNPS_SSI_SpiFrameFormat_e;

/*! @brief Frame Format.
 *         Selects which serial protocol transfers the data.
 *  @ingroup Group_SNPS_SSI
 */
typedef enum {
	MOTOROLA_SPI = 0x0, ///< Motorola SPI Frame Format
	TEXAS_SSP = 0x1, ///< Texas Instruments SSP Frame Format
	NS_MICROWIRE = 0x2, ///< National Microwire Frame Format
} SNPS_SSI_FrameFormat_e;

/*!
 * @brief Transfer mode
 *        Selects mode of transfer for serial communication
 * @ingroup Group_SNPS_SSI
 */
typedef enum {
	TX_AND_RX = 0x0, ///< Transmit and Receive (not for DUAL/QUAD/OCTAL SPI frame format)
	TX_ONLY = 0x1, ///< Transmit only
	RX_ONLY = 0x2, ///< Receive only
	EEPROM_READ = 0x3, ///< EEPROM Read (not for DUAL/QUAD/OCTAL SPI frame format)
} SNPS_SSI_TranferMode_e;

/*!
 * @brief Encodings for NS Microwire control frame size
 * @ingroup Group_SNPS_SSI
 */
typedef enum {
	SIZE_01_BIT = 0x0, ///< 1 Bit control frame
	SIZE_02_BIT = 0x1, ///< 2 Bit control frame
	SIZE_03_BIT = 0x2, ///< 3 Bit control frame
	SIZE_04_BIT = 0x3, ///< 4 Bit control frame
	SIZE_05_BIT = 0x4, ///< 5 Bit control frame
	SIZE_06_BIT = 0x5, ///< 6 Bit control frame
	SIZE_07_BIT = 0x6, ///< 7 Bit control frame
	SIZE_08_BIT = 0x7, ///< 8 Bit control frame
	SIZE_09_BIT = 0x8, ///< 9 Bit control frame
	SIZE_10_BIT = 0x9, ///< 10 Bit control frame
	SIZE_11_BIT = 0xa, ///< 11 Bit control frame
	SIZE_12_BIT = 0xb, ///< 12 Bit control frame
	SIZE_13_BIT = 0xc, ///< 13 Bit control frame
	SIZE_14_BIT = 0xd, ///< 14 Bit control frame
	SIZE_15_BIT = 0xe, ///< 15 Bit control frame
	SIZE_16_BIT = 0xf, ///< 16 Bit control frame
} SNPS_SSI_ControlFrameSize_e;

/*! @brief Serial clock phase
 *  @ingroup Group_SNPS_SSI
 *  @note Valid for Motorola SPI only
 */
typedef enum {
	SCPH_MIDDLE = 0x0, ///< Serial clock toggles in middle of first data bit
	SCPH_START = 0x1, ///< Serial clock toggles at start of first data bit
} SNPS_SSI_SpiClockPhase_e;

/*! @brief Inactive level for serial clock line.
 *  @ingroup Group_SNPS_SSI
 *  @note Valid for Motorola SPI only
 */
typedef enum {
	SCLK_LOW = 0x0, ///< Inactive state of serial clock is low
	SCLK_HIGH = 0x1, ///< Inactive state of serial clock is high
} SNPS_SSI_SpiClockPolarity_e;

typedef enum { non_sequential = 0x0,
	       sequential = 0x1 } SNPS_SSI_mw_transfer_mode;

typedef enum { receive = 0x0,
	       transmit = 0x1 } SNPS_SSI_mw_control;

typedef enum {
	txe = 0x1,
	txo = 0x2,
	rxu = 0x4,
	rxo = 0x8,
	rxf = 0x10,
	mst = 0x11,
	all_s = 0x1f,
	all_m = 0x3f
} SNPS_SSI_irq;

/*!
 * @brief Specify data direction allowed for the DMA channel
 */
typedef enum {
	DMA_TX = 0x0, ///< Direction is Transmit Only
	DMA_RX = 0x1, ///< Direction is Receive Only
} SNPS_SSI_DmaChannelDirection_e;

/*!
 * @brief Specify data direction allowed for the DMA channel
 */
typedef enum {
	ACTIVE_LOW = 0x0, ///< Handshake signal is active low
	///  Only possible if IP block has
	///  externally appliec glue logic that
	///  inverts HS signals
	ACTIVE_HIGH = 0x1, ///< Handshake signal is active high
	///  This is the default hardware defined value
} SNPS_SSI_DmaHandshakePolarity_e;

/*! @brief Set data frame size for serial communication
 *  @ingroup Group_SNPS_SSI
 *  @param size encoded data frame size
 *  @return 0 -- SUCCESS
 *          1 -- FAILURE
 *  @see SNPS_SSI_DataFrameSize_e
 */
int SNPS_SSI_setDataFrameSize(SNPS_SSI_regs_t *SSI, SNPS_SSI_DataFrameSize_e size);

/*! @brief Returns encoded data frame size
 *  @ingroup Group_SNPS_SSI
 *  @return encoded data frame size according to
 *          SNPS_SSI_DataFrameSize_e enum
 *  @see SNPS_SSI_DataFrameSize_e
 */
int SNPS_SSI_getDataFrameSize(SNPS_SSI_regs_t *SSI);

/*! @brief Set data frame format for Motorola SPI protocol
 *  @ingroup Group_SNPS_SSI
 *  @details Set data frame format for Motorola SPI protocol
 *           I.E. set number of lines used simultaneously for
 *           data transfers. Do not confuse with setSsiFrameFormat()
 *           method
 *  @param format -- encoded SPI Frame Format
 *  @see SNPS_SSI_SpiFrameFormat_e
 */
void SNPS_SSI_setSpiFrameFormat(SNPS_SSI_regs_t *SSI, SNPS_SSI_SpiFrameFormat_e format);

/*! @brief Returns data frame format for Motorola SPI protocol
 *  @ingroup Group_SNPS_SSI
 *  @details Returns data frame format for Motorola SPI protocol
 *           I.E. number of lines used simultaneously for data
 *           transfers. Do not confuse with getSsiFrameFormat()
 *           method
 *  @return encoded SPI Frame Format of SNPS_SSI_SpiFrameFormat_e type
 *  @see SNPS_SSI_SpiFrameFormat_e
 */
int SNPS_SSI_getSpiFrameFormat(SNPS_SSI_regs_t *SSI);

/*! @brief Set serial protocol to transfer data
 *  @ingroup Group_SNPS_SSI
 *  @param format -- format chosen
 *  @return 0 -- SUCCESS
 *          1 -- Failure (Unavailable format have been chosen
 *  @see SNPS_SSI_FrameFormat_e
 */
int SNPS_SSI_setSsiFrameFormat(SNPS_SSI_regs_t *SSI, SNPS_SSI_FrameFormat_e format);

/*! @brief return current frame format
 *  @ingroup Group_SNPS_SSI
 *  @return frame format of SNPS_SSI_FrameFormat_e type
 *  @see SNPS_SSI_FrameFormat_e
 */
int SNPS_SSI_getSsiFrameFormat(SNPS_SSI_regs_t *SSI);

// TODO: Actualize (NOTE that activity inactivity documented in enum)
/*!
 * @ingroup Group_SNPS_SSI
 * Устновить фазу тактового сигнала.
 * Функция используется только тогда, когда текущий формат кадра обмена - motorola.
 * @param phase = in_middle,
 *       at_start.
 */
void SNPS_SSI_setClockPhase(SNPS_SSI_regs_t *SSI, SNPS_SSI_SpiClockPhase_e phase);

// TODO: Actualize (NOTE that activity inactivity documented in enum)
/*!
 * @ingroup Group_SNPS_SSI
 * Текущая фаза тактового сигнала.
 * Функция используется только тогда, когда текущий формат кадра обмена - motorola.
 * @return 0x0 = in_middle,
 * @return 0x1 = at_start.
 */
int SNPS_SSI_getClockPhase(SNPS_SSI_regs_t *SSI);

// TODO: Actualize (NOTE that activity inactivity documented in enum)
/*!
 * @ingroup Group_SNPS_SSI
 * Устновить полярность тактового сигнала.
 * Функция используется только тогда, когда текущий формат кадра обмена - motorola.
 * @param polarity = inactive_low,
 *                   inactive_high.
 */
void SNPS_SSI_setClockPolarity(SNPS_SSI_regs_t *SSI, SNPS_SSI_SpiClockPolarity_e polarity);

// TODO: Actualize (NOTE that activity inactivity documented in enum)
/*!
 * @ingroup Group_SNPS_SSI
 * Текущая полярность тактового сигнала.
 * Функция используется только тогда, когда текущий формат кадра обмена - motorola.
 * @return 0x0 = inactive_low,
 * @return 0x1 = inactive_high.
 */
int SNPS_SSI_getClockPolarity(SNPS_SSI_regs_t *SSI);

/*!
 * @brief Set up serial line data transfer mode
 * @ingroup Group_SNPS_SSI
 * @param mode -- Chosen mode of data transfer via serial line
 * @see SNPS_SSI_TranferMode_e
 */
void SNPS_SSI_setTransferMode(SNPS_SSI_regs_t *SSI, SNPS_SSI_TranferMode_e mode);

// TODO: Actualize (NOTE that mode documented in enum)
/*!
 * @brief Returns current mode of transfer via serial line.
 * @ingroup Group_SNPS_SSI
 * @return Values according to SNPS_SSI_TranferMode_e enumeration, casted to \b int type
 * @see SNPS_SSI_TranferMode_e
 */
int SNPS_SSI_getTransferMode(SNPS_SSI_regs_t *SSI);

// TODO: Document
void SNPS_SSI_setRegisterLoop(SNPS_SSI_regs_t *SSI, int loop);

// TODO: Actualize
/*!
 * @ingroup Group_SNPS_SSI
 * @return true  - Тестовый режим выключен.
 * @return false - Тестовый режим выключен.
 */
int SNPS_SSI_getRegisterLoop(SNPS_SSI_regs_t *SSI);

// TODO: Actualize
/*!
 * @ingroup Group_SNPS_SSI
 * Включить тестовый режим. Ппередатчик SSI замыкается на приемник SSI.
 */
void SNPS_SSI_enableRegisterLoop(SNPS_SSI_regs_t *SSI);

// TODO: Actualize
/*!
 * @ingroup Group_SNPS_SSI
 * Выключить тестовый режим.
 */
void SNPS_SSI_disableRegisterLoop(SNPS_SSI_regs_t *SSI);

// TODO: Document
void SNPS_SSI_setSlaveOutputEnable(SNPS_SSI_regs_t *SSI, int oe);

// TODO: Document
int SNPS_SSI_getSlaveOutputEnable(SNPS_SSI_regs_t *SSI);

// TODO: Actualize
/*!
 * @ingroup Group_SNPS_SSI
 * Установить размер кадра контроля.
 * Функция используется только тогда, когда текущий формат кадра обмена - microwire.
 * @param size = control_bit_1,
 *      control_bit_2,
 *      control_bit_3,
 *      control_bit_4,
 *      control_bit_5,
 *      control_bit_6,
 *      control_bit_7,
 *      control_bit_8,
 *      control_bit_9,
 *      control_bit_10,
 *      control_bit_11,
 *      control_bit_12,
 *      control_bit_13,
 *      control_bit_14,
 *      control_bit_15,
 *      control_bit_16.
 */
void SNPS_SSI_setControlFrameSize(SNPS_SSI_regs_t *SSI, SNPS_SSI_ControlFrameSize_e size);

// TODO: Actualize
/*!
 * @ingroup Group_SNPS_SSI
 * Текущий размер кадра контроля.
 * Функция используется только тогда, когда текущий формат кадра обмена - microwire.
 * @return 0x0 = control_bit_1,
 * @return 0x1 = control_bit_2,
 * @return 0x2 = control_bit_3,
 * @return 0x3 = control_bit_4,
 * @return 0x4 = control_bit_5,
 * @return 0x5 = control_bit_6,
 * @return 0x6 = control_bit_7,
 * @return 0x7 = control_bit_8,
 * @return 0x8 = control_bit_9,
 * @return 0x9 = control_bit_10,
 * @return 0xa = control_bit_11,
 * @return 0xb = control_bit_12,
 * @return 0xc = control_bit_13,
 * @return 0xd = control_bit_14,
 * @return 0xe = control_bit_15,
 * @return 0xf = control_bit_16.
 */
int SNPS_SSI_getControlFrameSize(SNPS_SSI_regs_t *SSI);

/*!
 * @brief Set up number of data frames to be continouously received
 *        by DW_apb_ssi master from external slave
 * @ingroup Group_SNPS_SSI
 * @param number -- Number of data frames to be received [1..0x10000]
 */
void SNPS_SSI_setDataFramesNumberToReceive(SNPS_SSI_regs_t *SSI, int number);

/*!
 * @brief Show number of data frames to be continouously received
 *        by DW_apb_ssi master from external slave
 * @ingroup Group_SNPS_SSI
 * @return Number of data frames to be received [1..0x10000]
 */
int SNPS_SSI_getDataFramesNumberToReceive(SNPS_SSI_regs_t *SSI);

// TODO: Actualize
/*!
 * @ingroup Group_SNPS_SSI
 * Включить контроллер SSI.
 *
 */
void SNPS_SSI_enableSsi(SNPS_SSI_regs_t *SSI);

// TODO: Actualize
/*!
 * @ingroup Group_SNPS_SSI
 * Выключить контроллер SSI.
 *
 */
void SNPS_SSI_disableSsi(SNPS_SSI_regs_t *SSI);

// TODO: Actualize
/*!
 * @ingroup Group_SNPS_SSI
 * @return 0x0 - контроллер выключен.
 * @return 0x1 - контроллер включен.
 */
int SNPS_SSI_isSsiEnabled(SNPS_SSI_regs_t *SSI);

// TODO: Actualize
/*! @brief Indicates that a serial transfer is in progress
 *  @ingroup Group_SNPS_SSI
 *  @return false -- DW_apb_ssi is idle or disabled
 *          true  -- DW_apb_ssi is actively transferring data
 */
int SNPS_SSI_isSsiBusy(SNPS_SSI_regs_t *SSI);

int SNPS_SSI_isSsiTxEmpty(SNPS_SSI_regs_t *SSI);

/*!
 * @ingroup Group_SNPS_SSI
 * Устновить режим передачи данных.
 * Функция используется только тогда, когда текущий формат кадра обмена - microwire.
 * @param mode = non_sequential,
 *      sequential.
 */
void SNPS_SSI_set_mw_tramsfer_mode(SNPS_SSI_regs_t *SSI, SNPS_SSI_mw_transfer_mode mode);

/*!
 * @ingroup Group_SNPS_SSI
 * Текущий режим передачи данных.
 * Функция используется только тогда, когда текущий формат кадра обмена - microwire.
 * @return 0x0 - non_sequential,
 * @return 0x1- sequential.
 */
int SNPS_SSI_get_mw_transfer_mode(SNPS_SSI_regs_t *SSI);

/*!
 * @ingroup Group_SNPS_SSI
 * Установить направление потока данных.
 * Функция используется только тогда, когда текущий формат кадра обмена - microwire.
 * @param control = receive,
 *         transmit.
 */
void SNPS_SSI_set_mw_control(SNPS_SSI_regs_t *SSI, SNPS_SSI_mw_control control);

/*!
 * @ingroup Group_SNPS_SSI
 * Текущее направление потока данных.
 * Функция используется только тогда, когда текущий формат кадра обмена - microwire.
 * @return 0x0 - receive,
 * @return 0x1 - transmit.
 */
int SNPS_SSI_get_mw_control(SNPS_SSI_regs_t *SSI);

/*!
 * @ingroup Group_SNPS_SSI
 * Включить интерфейс квитирования.
 * Функция используется только тогда, когда текущий формат кадра обмена - microwire.
 */
void SNPS_SSI_enable_mw_handshaking(SNPS_SSI_regs_t *SSI);

/*!
 * @ingroup Group_SNPS_SSI
 * Выключить интерфейс квитирования.
 * Функция используется только тогда, когда текущий формат кадра обмена - microwire.
 */
void SNPS_SSI_disable_mw_handshaking(SNPS_SSI_regs_t *SSI);

/*!
 * @ingroup Group_SNPS_SSI
 * Функция используется только тогда, когда текущий формат кадра обмена - microwire.
 * @return true  - интерфейс квитирование включен.
 * @return false - интерфейс квитирование выключен.
 */
int SNPS_SSI_is_mw_handshaking_enabled(SNPS_SSI_regs_t *SSI);

/*!
 * @brief Set index for TX DMA handshake interface of this driver
 * @ingroup Group_SNPS_SSI
 * @param hsIndex -- Handshake index in the (sub)system
 */
void SNPS_SSI_setDmaTxHandshakeIndex(uint32_t hsIndex);

/*!
 * @brief Get index of the TX DMA handshake interface of this driver
 * @ingroup Group_SNPS_SSI
 * @return Handshake index in the (sub)system previously set by the
 *         setTxHandshakeIndex() method
 * @see void setTxHandshakeIndex(uint32_t)
 */
uint32_t SNPS_SSI_getDmaTxHandshakeIndex(void);

/*!
 * @brief Set index for RX DMA handshake interface of this driver
 * @ingroup Group_SNPS_SSI
 * @param hsIndex -- Handshake index in the (sub)system
 */
void SNPS_SSI_setDmaRxHandshakeIndex(uint32_t hsIndex);

/*!
 * @brief Get index of the RX DMA handshake interface of this driver
 * @ingroup Group_SNPS_SSI
 * @return Handshake index in the (sub)system previously set by the
 *         setRxHandshakeIndex() method
 * @see void setRxHandshakeIndex(uint32_t)
 */
uint32_t SNPS_SSI_getDmaRxHandshakeIndex(void);

/*!
 * @brief Set polarity for TX DMA handshake interface of this driver
 * @ingroup Group_SNPS_SSI
 * @param polarity -- Handshake polarity in the (sub)system
 */
void SNPS_SSI_setDmaTxHandshakePolarity(SNPS_SSI_DmaHandshakePolarity_e polarity);

/*!
 * @brief Get polarity of the TX DMA handshake interface of this driver
 * @ingroup Group_SNPS_SSI
 * @return Handshake polarity in the (sub)system previously set by the
 *         setTxHandshakePolarity() method
 * @see void setTxHandshakePolarity(SNPS_SSI_DmaHandshakePolarity_e)
 */
SNPS_SSI_DmaHandshakePolarity_e SNPS_SSI_getDmaTxHandshakePolarity(void);

/*!
 * @brief Set polarity for RX DMA handshake interface of this driver
 * @ingroup Group_SNPS_SSI
 * @param polarity -- Handshake polarity in the (sub)system
 */
void SNPS_SSI_setDmaRxHandshakePolarity(SNPS_SSI_DmaHandshakePolarity_e polarity);

/*!
 * @brief Get polarity of the RX DMA handshake interface of this driver
 * @ingroup Group_SNPS_SSI
 * @return Handshake polarity in the (sub)system previously set by the
 *         setRxHandshakePolarity() method
 * @see void setRxHandshakePolarity(SNPS_SSI_DmaHandshakePolarity_e)
 */
SNPS_SSI_DmaHandshakePolarity_e SNPS_SSI_getDmaRxHandshakePolarity(void);

/*! @brief Set pattern of nSS to be asserted during serial activity
 *  @ingroup Group_SNPS_SSI
 *  @param slavesField - bitfield, where 1 in corresponding bit select
 *                       nSS signal to be asserted
 */
void SNPS_SSI_setSlaveSelectsEnable(SNPS_SSI_regs_t *SSI, uint32_t slavesField);

/*! @brief Return pattern of nSS to be asserted during serial activity
 *  @ingroup Group_SNPS_SSI
 *  @return bitfield, where 1 in corresponding bit select nSS signal to be asserted
 */
uint32_t SNPS_SSI_getEnabledSlaveSelects(SNPS_SSI_regs_t *SSI);

/*! @brief Set clock divider to select SCLK frequency
 *  @ingroup Group_SNPS_SSI
 *  @param divider -- integer ssi_clk will be divided by
 *         2 <= divider <= 65534
 *  @note SCLK is disabled if divider == 0
 */
void SNPS_SSI_setBaudrateClkDivider(SNPS_SSI_regs_t *SSI, uint32_t divider);

/*! @brief Returns actual clock divider value
 *  @ingroup Group_SNPS_SSI
 *  @return actual baudrate clock divider value
 *  @note SCLK is disabled if divider == 0
 */
uint32_t SNPS_SSI_getBaudrateClkDivider(SNPS_SSI_regs_t *SSI);

// TODO: Actualize
/*!
 * @ingroup Group_SNPS_SSI
 * Устновить пороговый уровень заполнения FIFO передатчика.
 * Прерывание txe устанавливается, если кол-во записей в FIFO передатчика меньше этого значения.
 * @param level = 0x0..0x20.
 */
void SNPS_SSI_setTxFifoThrLevel(SNPS_SSI_regs_t *SSI, int level);

// TODO: Actualize
/*!
 * @ingroup Group_SNPS_SSI
 * Текущий пороговый уровень заполнения FIFO передачика.
 * Прерывание txe устанавливается, если кол-во записей в FIFO передатчика меньше этого значения.
 * @return 0x0..0x20.
 */
int SNPS_SSI_getTxFifoThrLevel(SNPS_SSI_regs_t *SSI);

// TODO: Actualize
/*!
 * @ingroup Group_SNPS_SSI
 * Устновить пороговый уровень заполнения FIFO приемника.
 * Прерывание rxf устанавливается, если кол-во записей в FIFO передатчика превышает это значение.
 * @param level = 0x0..0x20.
 */
void SNPS_SSI_setRxFifoThrLevel(SNPS_SSI_regs_t *SSI, int level);

// TODO: Actualize
/*!
 * @ingroup Group_SNPS_SSI
 * Текущий пороговый уровень заполнения FIFO приемника.
 * Прерывание rxf устанавливается, если кол-во записей в FIFO передатчика превышает это значение.
 * @return 0x0..0x20.
 */
int SNPS_SSI_getRxFifoThrLevel(SNPS_SSI_regs_t *SSI);

/*!
 * @ingroup Group_SNPS_SSI
 * Текущее колличество записей в FIFO передатчика.
 * @return 0x0..0x20. // TODO!!! Correct this
 */
int SNPS_SSI_getTxFifoLevel(SNPS_SSI_regs_t *SSI);

/*!
 * @ingroup Group_SNPS_SSI
 * Текущее колличество записей в FIFO приёмника
 * @return 0x0..0x20. // TODO!!! Correct this
 */
int SNPS_SSI_getRxFifoLevel(SNPS_SSI_regs_t *SSI);

/*!
 * @ingroup Group_SNPS_SSI
 * Текущее колличество записей в FIFO приемника.
 * @return 0x0..0x20.
 */
int SNPS_SSI_get_rx_fifo_level(SNPS_SSI_regs_t *SSI);

/*!
 * @ingroup Group_SNPS_SSI
 * Установить маску прерываний.
 * @param mask = txe,
 txo,
 rxf,
 rxo,
 rxu,
 mst,
 all.
 */
void SNPS_SSI_setIrqMask(SNPS_SSI_regs_t *SSI, SNPS_SSI_irq mask);
void SNPS_SSI_setIrqUnmask(SNPS_SSI_regs_t *SSI, SNPS_SSI_irq mask);

/*!
 * @ingroup Group_SNPS_SSI
 * Текущая маска прерываний.
 * @return 0x1  - txe,
 * @return 0x2  - txo,
 * @return 0x4  - rxf,
 * @return 0x8  - rxo,
 * @return 0x10 - rxu,
 * @return 0x11 - mst.
 */
int SNPS_SSI_getIrqMask(SNPS_SSI_regs_t *SSI);

/*!
 * @ingroup Group_SNPS_SSI
 * Текущий статус прерываний
 * @return 0x1  - txe,
 * @return 0x2  - txo,
 * @return 0x4  - rxf,
 * @return 0x8  - rxo,
 * @return 0x10 - rxu,
 * @return 0x11 - mst.
 */
int SNPS_SSI_getIrqStatus(SNPS_SSI_regs_t *SSI);

/*!
 * @ingroup Group_SNPS_SSI
 * Текущий статус немаскированных прерываний
 * @return 0x1  - txe,
 * @return 0x2  - txo,
 * @return 0x4  - rxf,
 * @return 0x8  - rxo,
 * @return 0x10 - rxu,
 * @return 0x11 - mst.
 */
int SNPS_SSI_getRawIrqStatus(SNPS_SSI_regs_t *SSI);

/*!
 * @ingroup Group_SNPS_SSI
 * Сбросить прерывания
 * If there are attempt to clear more than one flag at a time, or any value beside
 * listed in parameter section, all flags will be cleared.
 * TODO: refactor for more adequate behaviour
 * @param clear = txo, rxo, rxu, mst, all.
 */
void SNPS_SSI_clearInterrupts(SNPS_SSI_regs_t *SSI, SNPS_SSI_irq clear);

/*!
 * @brief Associate Irq index in this DW_apb_ssi instance and system Irq index
 *        i.e. associate local irq index to global irq index
 * @ingroup Group_SNPS_SSI
 * @param ipBlockIrqIndex -- Irq Source index
 * @param systemIrqIndex -- Global Irq Index
 * @return 0 -- SUCCESS
 * @return -1 -- FAILURE
 */
int SNPS_SSI_setIrqIndexInSystemVector(uint32_t ipBlockIrqIndex, uint32_t systemIrqIndex);

/*!
 * @brief Return global (system-level) Irq index that is associated
 *        to local (ip block-level) irq index provided as a parameter
 * @ingroup Group_SNPS_SSI
 * @param ipBlockIrqIndex -- Irq Source index
 * @return -1 -- FAILURE (no such local Irq)
 * @return 0 and greater -- global index of an Irq
 */
int SNPS_SSI_getIrqIndexInSystemVector(uint32_t ipBlockIrqIndex);

int SNPS_SSI_getIrqIndexInSystemVectorByCode(SNPS_SSI_irq ipBlockIrqCode);

/*!
 * @ingroup Group_SNPS_SSI
 * Функция считывает значение регистра TOGGLE, отвечающего за изменение уровня сигнала Slave Select в конце каждой передачи SPI.
 */
int SNPS_SSI_getToggleSS(SNPS_SSI_regs_t *SSI);

/*!
 * @ingroup Group_SNPS_SSI
 * Функция устанавливает значение регистра TOGGLE, отвечающего за изменение уровня сигнала Slave Select в конце каждой передачи SPI.
 */
void SNPS_SSI_setToggleSS(SNPS_SSI_regs_t *SSI, int value);

/*!
 * @ingroup Group_SNPS_SSI
 * Функция включает интерфейс квитирования для канала DMA приемника или передатчика SSI.
 */
void SNPS_SSI_dmaHsEnable(SNPS_SSI_regs_t *SSI, SNPS_SSI_DmaChannelDirection_e dir);

/*!
 * @ingroup Group_SNPS_SSI
 * Функция выключает интерфейс квитирования для канала DMA приемника или передатчика SSI.
 */
void SNPS_SSI_dmaHsDisable(SNPS_SSI_regs_t *SSI, SNPS_SSI_DmaChannelDirection_e dir);

/*!
 * @ingroup Group_SNPS_SSI
 * Функция определяет включен ли интерфейс квитирования для канала DMA приемника или передатчика SSI.
 */
int SNPS_SSI_isDmaHsEnabled(SNPS_SSI_regs_t *SSI, SNPS_SSI_DmaChannelDirection_e dir);

/*!
 * @brief Setup number of data items in FIFO that will trigger DMA request
 * @ingroup Group_SNPS_SSI
 * @details Setup number of data items in FIFO that will trigger DMA request\n
 *          dma_rx_req will be asserted when FIFO contains 'level' items or more\n
 *          dma_tx_req will be generated when FIFO contains 'level' items or less.
 * @param dir -- DMA Channel direction
 * @param level -- Watermark level
 */
void SNPS_SSI_setDmaRqLevel(SNPS_SSI_regs_t *SSI, SNPS_SSI_DmaChannelDirection_e dir, int level);

/*!
 * @ingroup Group_SNPS_SSI
 * Функция возвращает пороговое значение заполнения буфера FIFO приемника или передатчика SSI для интерфейса квитирования.
 */
int SNPS_SSI_getDmaRqLevel(SNPS_SSI_regs_t *SSI, SNPS_SSI_DmaChannelDirection_e dir);

// TODO: Document
void SNPS_SSI_writeData(SNPS_SSI_regs_t *SSI, uint32_t data);

// TODO: Document
uint32_t SNPS_SSI_readData(SNPS_SSI_regs_t *SSI);

/*!
 * @brief Decode data frame size in DW_apb_ssi format
 *        into the number of bits.
 * @ingroup Group_SNPS_SSI
 * @param encodedDFS -- Data Frame Size in the format it
 *        present in corresponding bitfield (SNPS_SSI_DataFrameSize_e)
 * @return Data frame size in number of bits
 * @see SNPS_SSI_DataFrameSize_e
 */
uint32_t SNPS_SSI_decodeDFS(SNPS_SSI_DataFrameSize_e encodedDFS);

/*!
 * @brief Encode frame size in number of bits
 *        to the format suitable for DW_apb_ssi driver
 * @ingroup Group_SNPS_SSI
 * @param DFS -- Data frame size in number of bits
 * @return encoded DFS value that can be used as bitfield value
 * @see SNPS_SSI_DataFrameSize_e
 */
SNPS_SSI_DataFrameSize_e SNPS_SSI_encodeDFS(uint32_t DFS);

/*!
 * @brief Return pointer to IP Block configuration Parameters
 * @ingroup Group_SNPS_SSI
 */
SNPS_SSI_Parameters_s *SNPS_SSI_getDwParams(void);

/*!
 * @brief Set up DW_apb_ssi IP Block parameters
 * @ingroup Group_SNPS_SSI
 */
void SNPS_SSI_setDwParams(SNPS_SSI_Parameters_s *pParams);

/*!
 * @brief Return number of Irq sources of this IP block
 * @ingroup Group_SNPS_SSI
 * @return Number of IRQ sources
 */
uint32_t SNPS_SSI_getIrqQty(void);

#endif // _SNSP_SSI_H_
