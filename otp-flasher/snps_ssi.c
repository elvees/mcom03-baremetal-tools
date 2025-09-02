// SPDX-License-Identifier: MIT
// Copyright 2025 RnD Center "ELVEES", JSC

#include "snps_ssi.h"
#include "ssi0_dw_apb_ssi_reg_fields.h"

SNPS_SSI_Parameters_s dwParameters; ///< DW_abp_ssi block parameters
SNPS_SSI_DmaHandshakePolarity_e dmaTxHsPolarity =
	ACTIVE_HIGH; // Transmit DMA channel handshake interface polarity
SNPS_SSI_DmaHandshakePolarity_e dmaRxHsPolarity =
	ACTIVE_HIGH; // Receive DMA channel handshake interface polarity

/*!
 * @brief array of system-level (global) irq indices. Index in this array
 *        corresponds to a local (ib-block-level) irq index.
 */
uint32_t pIrqIndices[6];

/*!
 * @brief Ip Block irq sources quantity. Also a size of the pIrqIndices array.
 */
uint32_t irqSourcesQty;

/*!
 * @brief Transmit DMA channel handshake interface index in system
 */
uint32_t dmaTxHsIndex;

/*!
 * @brief Receive DMA channel handshake interface index in system
 */
uint32_t dmaRxHsIndex;

/*!
 * @brief Flag showing that dwParameters have been initialized
 */
int paramsSet;

void SNPS_SSI_setToggleSS(SNPS_SSI_regs_t *SSI, int value)
{
	SSI->CTRLR0 = SET_SSI0_DW_APB_SSI_CTRLR0_SSTE(SSI->CTRLR0, value);
}

int SNPS_SSI_getToggleSS(SNPS_SSI_regs_t *SSI)
{
	return GET_SSI0_DW_APB_SSI_CTRLR0_SSTE(SSI->CTRLR0);
}

void SNPS_SSI_setSpiFrameFormat(SNPS_SSI_regs_t *SSI, SNPS_SSI_SpiFrameFormat_e format)
{
	SSI->CTRLR0 = SET_SSI0_DW_APB_SSI_CTRLR0_SPI_FRF(SSI->CTRLR0, format);
}

int SNPS_SSI_getSpiFrameFormat(SNPS_SSI_regs_t *SSI)
{
	return GET_SSI0_DW_APB_SSI_CTRLR0_SPI_FRF(SSI->CTRLR0);
}

int SNPS_SSI_setDataFrameSize(SNPS_SSI_regs_t *SSI, SNPS_SSI_DataFrameSize_e size)
{
	uint32_t maxTransferSize = SNPS_SSI_getDwParams()->SSI_MAX_XFER_SIZE;
	if (SNPS_SSI_decodeDFS(size) > maxTransferSize) {
		return 1;
	} else if (maxTransferSize == 16) {
		SSI->CTRLR0 = SET_SSI0_DW_APB_SSI_CTRLR0_DFS(SSI->CTRLR0, size);
	} else if (maxTransferSize == 32) {
		SSI->CTRLR0 = SET_SSI0_DW_APB_SSI_CTRLR0_DFS_32(SSI->CTRLR0, size);
	}
	return 0;
}

int SNPS_SSI_getDataFrameSize(SNPS_SSI_regs_t *SSI)
{
	int maxTransferSize = SNPS_SSI_getDwParams()->SSI_MAX_XFER_SIZE;
	if (maxTransferSize == 16) {
		return GET_SSI0_DW_APB_SSI_CTRLR0_DFS(SSI->CTRLR0);
	} else if (maxTransferSize == 32) {
		return GET_SSI0_DW_APB_SSI_CTRLR0_DFS_32(SSI->CTRLR0);
	} else {
		return -1;
	}
}

int SNPS_SSI_setSsiFrameFormat(SNPS_SSI_regs_t *SSI, SNPS_SSI_FrameFormat_e format)
{
	if (SNPS_SSI_getDwParams()->SSI_HC_FRF == 1) {
		int formatHardcoded = SNPS_SSI_getSsiFrameFormat(SSI);
		if (format != formatHardcoded) {
			return 1;
		} else {
			return 0;
		}
	} else {
		SSI->CTRLR0 = SET_SSI0_DW_APB_SSI_CTRLR0_FRF(SSI->CTRLR0, format);
	}
	return 0;
}

int SNPS_SSI_getSsiFrameFormat(SNPS_SSI_regs_t *SSI)
{
	return GET_SSI0_DW_APB_SSI_CTRLR0_FRF(SSI->CTRLR0);
}

void SNPS_SSI_setClockPhase(SNPS_SSI_regs_t *SSI, SNPS_SSI_SpiClockPhase_e phase)
{
	SSI->CTRLR0 = SET_SSI0_DW_APB_SSI_CTRLR0_SCPH(SSI->CTRLR0, phase);
}

int SNPS_SSI_getClockPhase(SNPS_SSI_regs_t *SSI)
{
	return GET_SSI0_DW_APB_SSI_CTRLR0_SCPH(SSI->CTRLR0);
}

void SNPS_SSI_setClockPolarity(SNPS_SSI_regs_t *SSI, SNPS_SSI_SpiClockPolarity_e polarity)
{
	SSI->CTRLR0 = SET_SSI0_DW_APB_SSI_CTRLR0_SCPOL(SSI->CTRLR0, polarity);
}

int SNPS_SSI_getClockPolarity(SNPS_SSI_regs_t *SSI)
{
	return GET_SSI0_DW_APB_SSI_CTRLR0_SCPOL(SSI->CTRLR0);
}

void SNPS_SSI_setTransferMode(SNPS_SSI_regs_t *SSI, SNPS_SSI_TranferMode_e mode)
{
	SSI->CTRLR0 = SET_SSI0_DW_APB_SSI_CTRLR0_TMOD(SSI->CTRLR0, mode);
}

int SNPS_SSI_getTransferMode(SNPS_SSI_regs_t *SSI)
{
	return GET_SSI0_DW_APB_SSI_CTRLR0_TMOD(SSI->CTRLR0);
}

void SNPS_SSI_setRegisterLoop(SNPS_SSI_regs_t *SSI, int loop)
{
	SSI->CTRLR0 = SET_SSI0_DW_APB_SSI_CTRLR0_SRL(SSI->CTRLR0, loop);
}

int SNPS_SSI_getRegisterLoop(SNPS_SSI_regs_t *SSI)
{
	return GET_SSI0_DW_APB_SSI_CTRLR0_SRL(SSI->CTRLR0);
}
void SNPS_SSI_enableRegisterLoop(SNPS_SSI_regs_t *SSI)
{
	SNPS_SSI_setRegisterLoop(SSI, 1);
}

void SNPS_SSI_disableRegisterLoop(SNPS_SSI_regs_t *SSI)
{
	SNPS_SSI_setRegisterLoop(SSI, 0);
}

void SNPS_SSI_setSlaveOutputEnable(SNPS_SSI_regs_t *SSI, int oe)
{
	// TODO: Shall not be RSVD
	SSI->CTRLR0 = SET_SSI0_DW_APB_SSI_CTRLR0_RSVD_SLV_OE(SSI->CTRLR0, oe);
}

int SNPS_SSI_getSlaveOutputEnable(SNPS_SSI_regs_t *SSI)
{
	// TODO: Shall not be RSVD
	return GET_SSI0_DW_APB_SSI_CTRLR0_RSVD_SLV_OE(SSI->CTRLR0);
}

void SNPS_SSI_setControlFrameSize(SNPS_SSI_regs_t *SSI, SNPS_SSI_ControlFrameSize_e size)
{
	SSI->CTRLR0 = SET_SSI0_DW_APB_SSI_CTRLR0_CFS(SSI->CTRLR0, size);
}

int SNPS_SSI_getControlFrameSize(SNPS_SSI_regs_t *SSI)
{
	return GET_SSI0_DW_APB_SSI_CTRLR0_CFS(SSI->CTRLR0);
}

void SNPS_SSI_setDataFramesNumberToReceive(SNPS_SSI_regs_t *SSI, int number)
{
	SSI->CTRLR1 = number;
}

int SNPS_SSI_getDataFramesNumberToReceive(SNPS_SSI_regs_t *SSI)
{
	return SSI->CTRLR1 + 1;
}

void SNPS_SSI_enableSsi(SNPS_SSI_regs_t *SSI)
{
	SSI->SSIENR = 1;
}

void SNPS_SSI_disableSsi(SNPS_SSI_regs_t *SSI)
{
	SSI->SSIENR = 0;
}

int SNPS_SSI_isSsiEnabled(SNPS_SSI_regs_t *SSI)
{
	return SSI->SSIENR;
}

int SNPS_SSI_isSsiBusy(SNPS_SSI_regs_t *SSI)
{
	return GET_SSI0_DW_APB_SSI_SR_BUSY(SSI->SR);
}

int SNPS_SSI_isSsiTxEmpty(SNPS_SSI_regs_t *SSI)
{
	return GET_SSI0_DW_APB_SSI_SR_TFE(SSI->SR);
}

void SNPS_SSI_set_mw_tramsfer_mode(SNPS_SSI_regs_t *SSI, SNPS_SSI_mw_transfer_mode mode)
{
	SSI->MWCR = SET_SSI0_DW_APB_SSI_MWCR_MWMOD(SSI->SSIENR, mode);
}

int SNPS_SSI_get_mw_transfer_mode(SNPS_SSI_regs_t *SSI)
{
	return GET_SSI0_DW_APB_SSI_MWCR_MWMOD(SSI->MWCR);
}

void SNPS_SSI_set_mw_control(SNPS_SSI_regs_t *SSI, SNPS_SSI_mw_control control)
{
	SSI->MWCR = SET_SSI0_DW_APB_SSI_MWCR_MDD(SSI->SSIENR, control);
}

int SNPS_SSI_get_mw_control(SNPS_SSI_regs_t *SSI)
{
	return GET_SSI0_DW_APB_SSI_MWCR_MDD(SSI->MWCR);
}

void SNPS_SSI_enable_mw_handshaking(SNPS_SSI_regs_t *SSI)
{
	SSI->MWCR = 1;
}

void SNPS_SSI_disable_mw_handshaking(SNPS_SSI_regs_t *SSI)
{
	SSI->MWCR = 0;
}

int SNPS_SSI_is_mw_handshaking_enabled(SNPS_SSI_regs_t *SSI)
{
	return GET_SSI0_DW_APB_SSI_MWCR_MHS(SSI->SSIENR);
}

void SNPS_SSI_setDmaTxHandshakeIndex(uint32_t hsIndex)
{
	dmaTxHsIndex = hsIndex;
}

uint32_t SNPS_SSI_getDmaTxHandshakeIndex(void)
{
	return dmaTxHsIndex;
}

void SNPS_SSI_setDmaRxHandshakeIndex(uint32_t hsIndex)
{
	dmaRxHsIndex = hsIndex;
}

uint32_t SNPS_SSI_getDmaRxHandshakeIndex(void)
{
	return dmaRxHsIndex;
}

void SNPS_SSI_setDmaTxHandshakePolarity(SNPS_SSI_DmaHandshakePolarity_e polarity)
{
	dmaTxHsPolarity = polarity;
}

SNPS_SSI_DmaHandshakePolarity_e SNPS_SSI_getDmaTxHandshakePolarity(void)
{
	return dmaTxHsPolarity;
}

void SNPS_SSI_setDmaRxHandshakePolarity(SNPS_SSI_DmaHandshakePolarity_e polarity)
{
	dmaRxHsPolarity = polarity;
}

SNPS_SSI_DmaHandshakePolarity_e SNPS_SSI_getDmaRxHandshakePolarity(void)
{
	return dmaRxHsPolarity;
}

void SNPS_SSI_setSlaveSelectsEnable(SNPS_SSI_regs_t *SSI, uint32_t slavesField)
{
	SSI->SER = slavesField;
}

uint32_t SNPS_SSI_getEnabledSlaveSelects(SNPS_SSI_regs_t *SSI)
{
	return SSI->SER;
}

void SNPS_SSI_setBaudrateClkDivider(SNPS_SSI_regs_t *SSI, uint32_t divider)
{
	SSI->BAUDR = divider;
}

uint32_t SNPS_SSI_getBaudrateClkDivider(SNPS_SSI_regs_t *SSI)
{
	return SSI->BAUDR;
}

void SNPS_SSI_setTxFifoThrLevel(SNPS_SSI_regs_t *SSI, int level)
{
	SSI->TXFTLR = level;
}

int SNPS_SSI_getTxFifoThrLevel(SNPS_SSI_regs_t *SSI)
{
	return SSI->TXFTLR;
}

void SNPS_SSI_setRxFifoThrLevel(SNPS_SSI_regs_t *SSI, int level)
{
	SSI->RXFTLR = level;
}

int SNPS_SSI_getRxFifoThrLevel(SNPS_SSI_regs_t *SSI)
{
	return SSI->RXFTLR;
}

int SNPS_SSI_getTxFifoLevel(SNPS_SSI_regs_t *SSI)
{
	return SSI->TXFLR;
}

int SNPS_SSI_getRxFifoLevel(SNPS_SSI_regs_t *SSI)
{
	return SSI->RXFLR;
}

int SNPS_SSI_get_rx_fifo_level(SNPS_SSI_regs_t *SSI)
{
	return SSI->RXFLR;
}

void SNPS_SSI_setIrqMask(SNPS_SSI_regs_t *SSI, SNPS_SSI_irq mask)
{
	SSI->IMR = SSI->IMR & ~mask;
}

void SNPS_SSI_setIrqUnmask(SNPS_SSI_regs_t *SSI, SNPS_SSI_irq mask)
{
	SSI->IMR = SSI->IMR | mask;
}

int SNPS_SSI_getIrqMask(SNPS_SSI_regs_t *SSI)
{
	return SSI->IMR;
}

int SNPS_SSI_getIrqStatus(SNPS_SSI_regs_t *SSI)
{
	return SSI->ISR;
}

int SNPS_SSI_getRawIrqStatus(SNPS_SSI_regs_t *SSI)
{
	return SSI->RISR;
}

void SNPS_SSI_clearInterrupts(SNPS_SSI_regs_t *SSI, SNPS_SSI_irq clear)
{
	switch (clear) {
	case txo:
		(void)GET_SSI0_DW_APB_SSI_TXOICR_TXOICR(SSI->TXOICR);
		break;
	case rxf:
		(void)GET_SSI0_DW_APB_SSI_RXOICR_RXOICR(SSI->RXOICR);
		break;
	case rxu:
		(void)GET_SSI0_DW_APB_SSI_RXUICR_RXUICR(SSI->RXUICR);
		break;
	case mst:
		(void)GET_SSI0_DW_APB_SSI_MSTICR_MSTICR(SSI->MSTICR);
		break;
	default:
		(void)GET_SSI0_DW_APB_SSI_ICR_ICR(SSI->ICR);
		break;
	}
}

int SNPS_SSI_setIrqIndexInSystemVector(uint32_t ipBlockIrqIndex, uint32_t systemIrqIndex)
{
	if (ipBlockIrqIndex < irqSourcesQty) {
		pIrqIndices[ipBlockIrqIndex] = systemIrqIndex;
		return 0;
	} else {
		return -1;
	}
}

int SNPS_SSI_getIrqIndexInSystemVector(uint32_t ipBlockIrqIndex)
{
	if (ipBlockIrqIndex < irqSourcesQty) {
		return pIrqIndices[ipBlockIrqIndex];
	}
	return -1;
}

int SNPS_SSI_getIrqIndexInSystemVectorByCode(SNPS_SSI_irq ipBlockIrqCode)
{
	for (uint32_t i = 0; i < irqSourcesQty; i++) {
		if (ipBlockIrqCode >> i & 1) {
			return pIrqIndices[i];
		}
	}

	return -1;
}

void SNPS_SSI_dmaHsEnable(SNPS_SSI_regs_t *SSI, SNPS_SSI_DmaChannelDirection_e dir)
{
	if (dir == DMA_TX) {
		SSI->DMACR = SET_SSI0_DW_APB_SSI_DMACR_TDMAE(SSI->DMACR, 0x1);
	} else if (dir == DMA_RX) {
		SSI->DMACR = SET_SSI0_DW_APB_SSI_DMACR_RDMAE(SSI->DMACR, 0x1);
	}
}

void SNPS_SSI_dmaHsDisable(SNPS_SSI_regs_t *SSI, SNPS_SSI_DmaChannelDirection_e dir)
{
	if (dir == DMA_TX) {
		SSI->DMACR = SET_SSI0_DW_APB_SSI_DMACR_TDMAE(SSI->DMACR, 0x0);
	} else if (dir == DMA_RX) {
		SSI->DMACR = SET_SSI0_DW_APB_SSI_DMACR_RDMAE(SSI->DMACR, 0x0);
	}
}

int SNPS_SSI_isDmaHsEnabled(SNPS_SSI_regs_t *SSI, SNPS_SSI_DmaChannelDirection_e dir)
{
	int retval = 0;

	if (dir == DMA_TX) {
		retval = GET_SSI0_DW_APB_SSI_DMACR_TDMAE(SSI->DMACR);
	} else if (dir == DMA_RX) {
		retval = GET_SSI0_DW_APB_SSI_DMACR_RDMAE(SSI->DMACR);
	}

	return retval;
}

void SNPS_SSI_setDmaRqLevel(SNPS_SSI_regs_t *SSI, SNPS_SSI_DmaChannelDirection_e dir, int level)
{
	if (dir == DMA_TX) {
		SSI->DMATDLR = SET_SSI0_DW_APB_SSI_DMATDLR_DMATDL(SSI->DMATDLR, level);
	} else if (dir == DMA_RX) {
		SSI->DMARDLR = SET_SSI0_DW_APB_SSI_DMARDLR_DMARDL(SSI->DMARDLR, level - 1);
	}
}

int SNPS_SSI_getDmaRqLevel(SNPS_SSI_regs_t *SSI, SNPS_SSI_DmaChannelDirection_e dir)
{
	int retval = 0;

	if (dir == DMA_TX) {
		retval = GET_SSI0_DW_APB_SSI_DMATDLR_DMATDL(SSI->DMATDLR);
	} else if (dir == DMA_RX) {
		retval = GET_SSI0_DW_APB_SSI_DMARDLR_DMARDL(SSI->DMARDLR) + 1;
	}

	return retval;
}

void SNPS_SSI_writeData(SNPS_SSI_regs_t *SSI, uint32_t data)
{
	SSI->DR[0] = data;
}

uint32_t SNPS_SSI_readData(SNPS_SSI_regs_t *SSI)
{
	return SSI->DR[0];
}

uint32_t SNPS_SSI_decodeDFS(SNPS_SSI_DataFrameSize_e encodedDFS)
{
	return (encodedDFS + 1);
}

SNPS_SSI_DataFrameSize_e SNPS_SSI_encodeDFS(uint32_t DFS)
{
	return (SNPS_SSI_DataFrameSize_e)(DFS - 1);
}

SNPS_SSI_Parameters_s *SNPS_SSI_getDwParams(void)
{
	return &dwParameters;
}

void SNPS_SSI_setDwParams(SNPS_SSI_Parameters_s *pParams)
{
	if (pParams) {
		dwParameters.SSI_APBIF_TYPE = pParams->SSI_APBIF_TYPE;
		dwParameters.SSI_APB3_ERR_RESP_EN = pParams->SSI_APB3_ERR_RESP_EN;
		dwParameters.APB_DATA_WIDTH = pParams->APB_DATA_WIDTH;
		dwParameters.SSI_IS_MASTER = pParams->SSI_IS_MASTER;
		dwParameters.SSI_ENH_CLK_RATIO = pParams->SSI_ENH_CLK_RATIO;
		dwParameters.SSI_MAX_XFER_SIZE = pParams->SSI_MAX_XFER_SIZE;
		dwParameters.SSI_RX_FIFO_DEPTH = pParams->SSI_RX_FIFO_DEPTH;
		dwParameters.SSI_TX_FIFO_DEPTH = pParams->SSI_TX_FIFO_DEPTH;
		dwParameters.SSI_NUM_SLAVES = pParams->SSI_NUM_SLAVES;
		dwParameters.SSI_HAS_RX_SAMPLE_DELAY = pParams->SSI_HAS_RX_SAMPLE_DELAY;
		dwParameters.SSI_RX_DLY_SR_DEPTH = pParams->SSI_RX_DLY_SR_DEPTH;
		dwParameters.SSI_ID = pParams->SSI_ID;
		dwParameters.SSI_HAS_DMA = pParams->SSI_HAS_DMA;
		dwParameters.SSI_INTR_IO = pParams->SSI_INTR_IO;
		dwParameters.SSI_INTR_POL = pParams->SSI_INTR_POL;
		dwParameters.SSI_SYNC_CLK = pParams->SSI_SYNC_CLK;
		dwParameters.SSI_CLK_EN_MODE = pParams->SSI_CLK_EN_MODE;
		dwParameters.SSI_HC_FRF = pParams->SSI_HC_FRF;
		dwParameters.SSI_DFLT_FRF = pParams->SSI_DFLT_FRF;
		dwParameters.SSI_DFLT_SCPOL = pParams->SSI_DFLT_SCPOL;
		dwParameters.SSI_DFLT_SCPH = pParams->SSI_DFLT_SCPH;
		dwParameters.SSI_SCPH0_SSTOGGLE = pParams->SSI_SCPH0_SSTOGGLE;
		dwParameters.SSI_SPI_MODE = pParams->SSI_SPI_MODE;
		dwParameters.SSI_IO_MAP_EN = pParams->SSI_IO_MAP_EN;
		dwParameters.SSI_HAS_DDR = pParams->SSI_HAS_DDR;
		dwParameters.SSI_HAS_RXDS = pParams->SSI_HAS_RXDS;
		dwParameters.SSI_SPI_DM_EN = pParams->SSI_SPI_DM_EN;
		dwParameters.SSI_XIP_EN = pParams->SSI_XIP_EN;
	}

	if (pParams->SSI_IS_MASTER) {
		irqSourcesQty = 6;
	} else {
		irqSourcesQty = 5;
	}

	paramsSet = 1;
}

uint32_t SNPS_SSI_getIrqQty(void)
{
	return irqSourcesQty;
}
