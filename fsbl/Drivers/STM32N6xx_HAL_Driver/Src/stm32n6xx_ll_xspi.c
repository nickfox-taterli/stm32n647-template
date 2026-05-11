/**
  ******************************************************************************
  * @file    stm32n6xx_ll_xspi.c
  * @brief   XSPI LL module driver.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#if defined(USE_FULL_LL_DRIVER)

/* Includes ------------------------------------------------------------------*/
#include "stm32n6xx_ll_xspi.h"
#include "stm32n6xx_ll_xspi.h"
#ifdef  USE_FULL_ASSERT
#include "stm32n6xx_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif

/** @addtogroup STM32N6xx_LL_Driver
  * @{
  */

#if defined (XSPI1) || defined (XSPI2) || defined (XSPI3)

/** @addtogroup XSPI_LL
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/** @addtogroup XSPI_LL_Private_Macros
  * @{
  */

#define IS_LL_XSPI_MEMORY_MODE(__VALUE__)    (((__VALUE__) == LL_XSPI_SINGLE_MEM) || \
                                              ((__VALUE__) == LL_XSPI_DUAL_MEM))

#define IS_LL_XSPI_MEMORY_TYPE(__VALUE__)    (((__VALUE__) == LL_XSPI_MEMTYPE_MICRON)       || \
                                              ((__VALUE__) == LL_XSPI_MEMTYPE_MACRONIX)     || \
                                              ((__VALUE__) == LL_XSPI_MEMTYPE_APMEM)        || \
                                              ((__VALUE__) == LL_XSPI_MEMTYPE_MACRONIX_RAM) || \
                                              ((__VALUE__) == LL_XSPI_MEMTYPE_HYPERBUS)     || \
                                              ((__VALUE__) == LL_XSPI_MEMTYPE_APMEM_16BITS))

#define IS_LL_XSPI_MEMORY_SIZE(__VALUE__)    ((__VALUE__) <= LL_XSPI_SIZE_32GB)

#define IS_LL_XSPI_CS_HIGH_TIME(__VALUE__)   (((__VALUE__) >= 1U) && ((__VALUE__) <= 64U))

#define IS_LL_XSPI_FREE_RUN_CLK(__VALUE__)   (((__VALUE__) == LL_XSPI_FREERUNCLK_DISABLE) || \
                                              ((__VALUE__) == LL_XSPI_FREERUNCLK_ENABLE))

#define IS_LL_XSPI_CLOCK_MODE(__VALUE__)     (((__VALUE__) == LL_XSPI_CLOCK_MODE_0) || \
                                              ((__VALUE__) == LL_XSPI_CLOCK_MODE_3))

#define IS_LL_XSPI_WRAP_SIZE(__VALUE__)      (((__VALUE__) == LL_XSPI_WRAP_NOT_SUPPORTED) || \
                                              ((__VALUE__) == LL_XSPI_WRAP_16_BYTES)      || \
                                              ((__VALUE__) == LL_XSPI_WRAP_32_BYTES)      || \
                                              ((__VALUE__) == LL_XSPI_WRAP_64_BYTES)      || \
                                              ((__VALUE__) == LL_XSPI_WRAP_128_BYTES))

#define IS_LL_XSPI_CLK_PRESCALER(__VALUE__)  ((__VALUE__) <= 255U)

#define IS_LL_XSPI_SAMPLE_SHIFT(__VALUE__)   (((__VALUE__) == LL_XSPI_SAMPLE_SHIFT_NONE) || \
                                              ((__VALUE__) == LL_XSPI_SAMPLE_SHIFT_HALFCYCLE))

#define IS_LL_XSPI_CS_BOUNDARY(__VALUE__)    ((__VALUE__) <= LL_XSPI_CSBOUND_16GB)

#define IS_LL_XSPI_FIFO_THRESHOLD(__VALUE__) (((__VALUE__) >= 1U) && ((__VALUE__) <= 64U))

#define IS_LL_XSPI_MAXTRAN(__VALUE__)        ((__VALUE__) <= 255U)

#define IS_LL_XSPI_MEMORY_SELECT(__VALUE__)  (((__VALUE__) == LL_XSPI_CSSEL_NCS1) || \
                                              ((__VALUE__) == LL_XSPI_CSSEL_NCS2))

#define IS_LL_XSPI_MEMORY_EXTENDED(__VALUE__) (((__VALUE__) == LL_XSPI_MEMEXT_SW) || \
                                               ((__VALUE__) == LL_XSPI_MEMEXT_HW))

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup XSPI_LL_EF_Init
  * @{
  */

/**
  * @brief  Initialize the XSPI peripheral according to the specified parameters.
  * @param  XSPIx XSPI instance
  * @param  XSPI_InitStruct pointer to a @ref LL_XSPI_InitTypeDef structure
  * @retval An ErrorStatus (SUCCESS)
  */
ErrorStatus LL_XSPI_Init(XSPI_TypeDef *XSPIx, const LL_XSPI_InitTypeDef *XSPI_InitStruct)
{
  /* Check the parameters */
  assert_param(IS_LL_XSPI_MEMORY_MODE(XSPI_InitStruct->MemoryMode));
  assert_param(IS_LL_XSPI_MEMORY_TYPE(XSPI_InitStruct->MemoryType));
  assert_param(IS_LL_XSPI_MEMORY_SIZE(XSPI_InitStruct->MemorySize));
  assert_param(IS_LL_XSPI_CS_HIGH_TIME(XSPI_InitStruct->ChipSelectHighTimeCycle));
  assert_param(IS_LL_XSPI_FREE_RUN_CLK(XSPI_InitStruct->FreeRunningClock));
  assert_param(IS_LL_XSPI_CLOCK_MODE(XSPI_InitStruct->ClockMode));
  assert_param(IS_LL_XSPI_WRAP_SIZE(XSPI_InitStruct->WrapSize));
  assert_param(IS_LL_XSPI_CLK_PRESCALER(XSPI_InitStruct->ClockPrescaler));
  assert_param(IS_LL_XSPI_SAMPLE_SHIFT(XSPI_InitStruct->SampleShifting));
  assert_param(IS_LL_XSPI_CS_BOUNDARY(XSPI_InitStruct->ChipSelectBoundary));
  assert_param(IS_LL_XSPI_FIFO_THRESHOLD(XSPI_InitStruct->FifoThresholdByte));
  assert_param(IS_LL_XSPI_MAXTRAN(XSPI_InitStruct->MaxTran));
  assert_param(IS_LL_XSPI_MEMORY_SELECT(XSPI_InitStruct->MemorySelect));
  assert_param(IS_LL_XSPI_MEMORY_EXTENDED(XSPI_InitStruct->MemoryExtended));

  /* Configure DCR1: MemoryType, MemorySize, ChipSelectHighTime, ClockMode */
  MODIFY_REG(XSPIx->DCR1,
             (XSPI_DCR1_MTYP | XSPI_DCR1_DEVSIZE | XSPI_DCR1_CSHT | XSPI_DCR1_CKMODE),
             (XSPI_InitStruct->MemoryType |
              (XSPI_InitStruct->MemorySize << XSPI_DCR1_DEVSIZE_Pos) |
              ((XSPI_InitStruct->ChipSelectHighTimeCycle - 1U) << XSPI_DCR1_CSHT_Pos) |
              XSPI_InitStruct->ClockMode));

  /* Configure DCR2: WrapSize */
  MODIFY_REG(XSPIx->DCR2, XSPI_DCR2_WRAPSIZE, XSPI_InitStruct->WrapSize);

  /* Configure DCR3: ChipSelectBoundary and MaxTran */
  MODIFY_REG(XSPIx->DCR3, XSPI_DCR3_CSBOUND,
             (XSPI_InitStruct->ChipSelectBoundary << XSPI_DCR3_CSBOUND_Pos));
  MODIFY_REG(XSPIx->DCR3, XSPI_DCR3_MAXTRAN,
             (XSPI_InitStruct->MaxTran << XSPI_DCR3_MAXTRAN_Pos));

  /* Configure DCR4: Refresh */
  WRITE_REG(XSPIx->DCR4, XSPI_InitStruct->Refresh);

  /* Configure CR: FifoThreshold */
  MODIFY_REG(XSPIx->CR, XSPI_CR_FTHRES,
             ((XSPI_InitStruct->FifoThresholdByte - 1U) << XSPI_CR_FTHRES_Pos));

  /* Wait for BUSY flag to clear */
  while (LL_XSPI_IsActiveFlag_BUSY(XSPIx) != 0U)
  {
  }

  /* Configure DCR2: ClockPrescaler (triggers automatic calibration) */
  MODIFY_REG(XSPIx->DCR2, XSPI_DCR2_PRESCALER,
             (XSPI_InitStruct->ClockPrescaler << XSPI_DCR2_PRESCALER_Pos));

  /* Wait for calibration complete */
  while (LL_XSPI_IsActiveFlag_BUSY(XSPIx) != 0U)
  {
  }

  /* Configure CR: MemoryMode and MemorySelect */
  MODIFY_REG(XSPIx->CR, (XSPI_CR_DMM | XSPI_CR_CSSEL),
             (XSPI_InitStruct->MemoryMode | XSPI_InitStruct->MemorySelect));

  /* Configure TCR: SampleShifting */
  MODIFY_REG(XSPIx->TCR, XSPI_TCR_SSHIFT, XSPI_InitStruct->SampleShifting);

  /* Enable XSPI */
  LL_XSPI_Enable(XSPIx);

  /* Enable free running clock if needed (must be after XSPI enable) */
  if (XSPI_InitStruct->FreeRunningClock == LL_XSPI_FREERUNCLK_ENABLE)
  {
    SET_BIT(XSPIx->DCR1, XSPI_DCR1_FRCK);
  }

  /* Configure extended memory if needed */
  if (XSPI_InitStruct->MemoryExtended == LL_XSPI_MEMEXT_HW)
  {
    SET_BIT(XSPIx->DCR1, XSPI_DCR1_EXTENDMEM);
  }

  return SUCCESS;
}

/**
  * @brief  Set each @ref LL_XSPI_InitTypeDef field to default value.
  * @param  XSPI_InitStruct pointer to a @ref LL_XSPI_InitTypeDef structure
  * @retval None
  */
void LL_XSPI_StructInit(LL_XSPI_InitTypeDef *XSPI_InitStruct)
{
  XSPI_InitStruct->FifoThresholdByte       = 1U;
  XSPI_InitStruct->MemoryMode              = LL_XSPI_SINGLE_MEM;
  XSPI_InitStruct->MemoryType              = LL_XSPI_MEMTYPE_MICRON;
  XSPI_InitStruct->MemorySize              = LL_XSPI_SIZE_16B;
  XSPI_InitStruct->ChipSelectHighTimeCycle = 1U;
  XSPI_InitStruct->FreeRunningClock        = LL_XSPI_FREERUNCLK_DISABLE;
  XSPI_InitStruct->ClockMode               = LL_XSPI_CLOCK_MODE_0;
  XSPI_InitStruct->WrapSize                = LL_XSPI_WRAP_NOT_SUPPORTED;
  XSPI_InitStruct->ClockPrescaler          = 0U;
  XSPI_InitStruct->SampleShifting          = LL_XSPI_SAMPLE_SHIFT_NONE;
  XSPI_InitStruct->ChipSelectBoundary      = LL_XSPI_CSBOUND_NONE;
  XSPI_InitStruct->MaxTran                 = 0U;
  XSPI_InitStruct->Refresh                 = 0U;
  XSPI_InitStruct->MemorySelect            = LL_XSPI_CSSEL_NCS1;
  XSPI_InitStruct->MemoryExtended          = LL_XSPI_MEMEXT_SW;
}

/**
  * @brief  Configure the XSPI HyperBus latency and timing parameters.
  * @param  XSPIx XSPI instance
  * @param  pCfg pointer to a @ref LL_XSPI_HyperbusCfgTypeDef structure
  * @retval None
  */
void LL_XSPI_HyperbusCfg(XSPI_TypeDef *XSPIx, const LL_XSPI_HyperbusCfgTypeDef *pCfg)
{
  /* Wait until BUSY flag is cleared */
  while (LL_XSPI_IsActiveFlag_BUSY(XSPIx) != 0U)
  {
  }

  /* Configure HyperBus Latency register */
  WRITE_REG(XSPIx->HLCR,
            (pCfg->RWRecoveryTimeCycle << XSPI_HLCR_TRWR_Pos) |
            (pCfg->AccessTimeCycle << XSPI_HLCR_TACC_Pos)     |
            pCfg->WriteZeroLatency | pCfg->LatencyMode);
}

/**
  * @brief  Abort any ongoing XSPI transfer and return to indirect mode.
  * @param  XSPIx XSPI instance
  * @retval None
  */
void LL_XSPI_Abort(XSPI_TypeDef *XSPIx)
{
  /* Disable DMA (harmless if not enabled) */
  CLEAR_BIT(XSPIx->CR, XSPI_CR_DMAEN);

  if (READ_BIT(XSPIx->SR, XSPI_SR_BUSY) != 0U)
  {
    /* Request abort */
    SET_BIT(XSPIx->CR, XSPI_CR_ABORT);

    /* Wait for transfer complete flag */
    while (READ_BIT(XSPIx->SR, XSPI_SR_TCF) == 0U)
    {
    }

    /* Clear transfer complete flag */
    WRITE_REG(XSPIx->FCR, XSPI_FCR_CTCF);

    /* Wait until busy flag clears */
    while (READ_BIT(XSPIx->SR, XSPI_SR_BUSY) != 0U)
    {
    }
  }

  /* Return to indirect mode */
  CLEAR_BIT(XSPIx->CR, XSPI_CR_FMODE);
}

/**
  * @brief  Configure HyperBus command registers.
  */
void LL_XSPI_HyperbusCmd(XSPI_TypeDef *XSPIx, uint32_t AddressSpace,
                          uint32_t Address, uint32_t DataLength,
                          uint32_t DQSMode, uint32_t DataMode,
                          uint32_t AddressWidth)
{
  while (READ_BIT(XSPIx->SR, XSPI_SR_BUSY) != 0U) {}

  CLEAR_BIT(XSPIx->CR, XSPI_CR_FMODE);
  MODIFY_REG(XSPIx->DCR1, XSPI_DCR1_MTYP_0, AddressSpace);

  WRITE_REG(XSPIx->CCR, DQSMode | XSPI_CCR_DDTR | DataMode |
            AddressWidth | XSPI_CCR_ADDTR | XSPI_CCR_ADMODE_2);
  WRITE_REG(XSPIx->WCCR, DQSMode | XSPI_WCCR_DDTR | DataMode |
             AddressWidth | XSPI_WCCR_ADDTR | XSPI_WCCR_ADMODE_2);

  WRITE_REG(XSPIx->DLR, DataLength - 1U);
  WRITE_REG(XSPIx->AR, Address);
}

/**
  * @brief  Transmit data via XSPI indirect write mode.
  */
void LL_XSPI_Transmit(XSPI_TypeDef *XSPIx, const uint8_t *pData)
{
  uint32_t count = READ_REG(XSPIx->DLR) + 1U;

  MODIFY_REG(XSPIx->CR, XSPI_CR_FMODE, LL_XSPI_MODE_INDIRECT_WRITE);

  while (count > 0U)
  {
    while (READ_BIT(XSPIx->SR, XSPI_SR_FTF) == 0U) {}
    *((__IO uint8_t *)&XSPIx->DR) = *pData;
    pData++;
    count--;
  }

  while (READ_BIT(XSPIx->SR, XSPI_SR_TCF) == 0U) {}
  WRITE_REG(XSPIx->FCR, XSPI_FCR_CTCF);
}

/**
  * @brief  Receive data via XSPI indirect read mode.
  */
void LL_XSPI_Receive(XSPI_TypeDef *XSPIx, uint8_t *pData, uint32_t MemoryType)
{
  uint32_t count = READ_REG(XSPIx->DLR) + 1U;
  uint32_t addr_reg = READ_REG(XSPIx->AR);
  uint32_t ir_reg   = READ_REG(XSPIx->IR);

  MODIFY_REG(XSPIx->CR, XSPI_CR_FMODE, LL_XSPI_MODE_INDIRECT_READ);

  if (MemoryType == LL_XSPI_MEMTYPE_HYPERBUS)
    WRITE_REG(XSPIx->AR, addr_reg);
  else if (READ_BIT(XSPIx->CCR, XSPI_CCR_ADMODE) != 0U)
    WRITE_REG(XSPIx->AR, addr_reg);
  else
    WRITE_REG(XSPIx->IR, ir_reg);

  while (count > 0U)
  {
    while (READ_BIT(XSPIx->SR, (XSPI_SR_FTF | XSPI_SR_TCF)) == 0U) {}
    *pData = *((__IO uint8_t *)&XSPIx->DR);
    pData++;
    count--;
  }

  while (READ_BIT(XSPIx->SR, XSPI_SR_TCF) == 0U) {}
  WRITE_REG(XSPIx->FCR, XSPI_FCR_CTCF);
}

/**
  * @brief  Enable XSPI memory-mapped mode.
  */
void LL_XSPI_EnableMemoryMappedMode(XSPI_TypeDef *XSPIx, uint32_t TimeOutActivation,
                                     uint32_t TimeoutPeriod, uint32_t NoPrefetchData,
                                     uint32_t NoPrefetchAXI)
{
  while (READ_BIT(XSPIx->SR, XSPI_SR_BUSY) != 0U) {}

  MODIFY_REG(XSPIx->CR, (XSPI_CR_NOPREF | XSPI_CR_NOPREF_AXI),
             (NoPrefetchData | NoPrefetchAXI));

  if (TimeOutActivation != 0U)
  {
    WRITE_REG(XSPIx->LPTR, TimeoutPeriod);
    WRITE_REG(XSPIx->FCR, XSPI_FCR_CTOF);
    SET_BIT(XSPIx->CR, XSPI_CR_TOIE);
  }

  MODIFY_REG(XSPIx->CR, (XSPI_CR_TCEN | XSPI_CR_FMODE),
             (TimeOutActivation | LL_XSPI_MODE_MEMORY_MAPPED));
}

/**
  * @brief  Perform auto-polling on XSPI status register.
  */
void LL_XSPI_AutoPolling(XSPI_TypeDef *XSPIx, uint32_t MatchValue,
                          uint32_t MatchMask, uint32_t IntervalTime,
                          uint32_t MatchMode, uint32_t AutomaticStop,
                          uint32_t MemoryType)
{
  uint32_t addr_reg = READ_REG(XSPIx->AR);
  uint32_t ir_reg   = READ_REG(XSPIx->IR);

  while (READ_BIT(XSPIx->SR, XSPI_SR_BUSY) != 0U) {}

  WRITE_REG(XSPIx->PSMAR, MatchValue);
  WRITE_REG(XSPIx->PSMKR, MatchMask);
  WRITE_REG(XSPIx->PIR, IntervalTime);

  MODIFY_REG(XSPIx->CR, (XSPI_CR_PMM | XSPI_CR_APMS | XSPI_CR_FMODE),
             (MatchMode | AutomaticStop | LL_XSPI_MODE_AUTO_POLLING));

  if (MemoryType == LL_XSPI_MEMTYPE_HYPERBUS)
    WRITE_REG(XSPIx->AR, addr_reg);
  else if (READ_BIT(XSPIx->CCR, XSPI_CCR_ADMODE) != 0U)
    WRITE_REG(XSPIx->AR, addr_reg);
  else
    WRITE_REG(XSPIx->IR, ir_reg);

  while (READ_BIT(XSPIx->SR, XSPI_SR_SMF) == 0U) {}
  WRITE_REG(XSPIx->FCR, XSPI_FCR_CSMF);
}

/**
  * @brief  Configure XSPI regular command registers.
  *         Accepts the same XSPI_RegularCmdTypeDef used by HAL for minimal
  *         caller changes.
  * @param  XSPIx XSPI instance
  * @param  MemoryMode LL_XSPI_SINGLE_MEM or LL_XSPI_DUAL_MEM
  * @param  SampleShifting LL_XSPI_SAMPLE_SHIFT_NONE or LL_XSPI_SAMPLE_SHIFT_HALFCYCLE
  * @param  pCmd Pointer to command configuration structure
  * @retval SUCCESS or ERROR
  */
ErrorStatus LL_XSPI_ConfigRegularCmd(XSPI_TypeDef *XSPIx, uint32_t MemoryMode,
                                      uint32_t SampleShifting,
                                      const XSPI_RegularCmdTypeDef *pCmd)
{
  __IO uint32_t *ccr_reg;
  __IO uint32_t *tcr_reg;
  __IO uint32_t *ir_reg;
  __IO uint32_t *abr_reg;

  /* Wait for BUSY to clear */
  while (READ_BIT(XSPIx->SR, XSPI_SR_BUSY) != 0U) {}

  /* Clear functional mode */
  CLEAR_BIT(XSPIx->CR, XSPI_CR_FMODE);

  /* Set IO select for single memory mode */
  if (MemoryMode == LL_XSPI_SINGLE_MEM)
  {
    MODIFY_REG(XSPIx->CR, XSPI_CR_MSEL, pCmd->IOSelect);
  }

  /* Select register set based on operation type */
  if (pCmd->OperationType == LL_XSPI_OPTYPE_WRITE_CFG)
  {
    ccr_reg = &XSPIx->WCCR;
    tcr_reg = &XSPIx->WTCR;
    ir_reg  = &XSPIx->WIR;
    abr_reg = &XSPIx->WABR;
  }
  else if (pCmd->OperationType == LL_XSPI_OPTYPE_WRAP_CFG)
  {
    ccr_reg = &XSPIx->WPCCR;
    tcr_reg = &XSPIx->WPTCR;
    ir_reg  = &XSPIx->WPIR;
    abr_reg = &XSPIx->WPABR;
  }
  else
  {
    ccr_reg = &XSPIx->CCR;
    tcr_reg = &XSPIx->TCR;
    ir_reg  = &XSPIx->IR;
    abr_reg = &XSPIx->ABR;
  }

  /* Configure CCR with DQS mode */
  *ccr_reg = pCmd->DQSMode;

  /* Configure alternate bytes */
  if (pCmd->AlternateBytesMode != LL_XSPI_ALT_BYTES_NONE)
  {
    *abr_reg = pCmd->AlternateBytes;
    MODIFY_REG(*ccr_reg, (XSPI_CCR_ABMODE | XSPI_CCR_ABDTR | XSPI_CCR_ABSIZE),
               (pCmd->AlternateBytesMode | pCmd->AlternateBytesDTRMode | pCmd->AlternateBytesWidth));
  }

  /* Configure dummy cycles */
  MODIFY_REG(*tcr_reg, XSPI_TCR_DCYC, pCmd->DummyCycles);

  /* Configure data length for common operations with data */
  if (pCmd->DataMode != LL_XSPI_DATA_NONE)
  {
    if (pCmd->OperationType == LL_XSPI_OPTYPE_COMMON_CFG)
    {
      WRITE_REG(XSPIx->DLR, pCmd->DataLength - 1U);
    }
  }

  /* Configure SSHIFT for SDR/DTR */
  if (pCmd->DataMode != LL_XSPI_DATA_NONE)
  {
    if (pCmd->DataDTRMode == LL_XSPI_DATA_DTR_ENABLE)
    {
      CLEAR_BIT(XSPIx->TCR, XSPI_TCR_SSHIFT);
    }
    else if (SampleShifting == LL_XSPI_SAMPLE_SHIFT_HALFCYCLE)
    {
      SET_BIT(XSPIx->TCR, XSPI_TCR_SSHIFT);
    }
  }

  /* Configure CCR with instruction/address/data mode parameters */
  if (pCmd->InstructionMode != LL_XSPI_INSTRUCTION_NONE)
  {
    if (pCmd->AddressMode != LL_XSPI_ADDRESS_NONE)
    {
      if (pCmd->DataMode != LL_XSPI_DATA_NONE)
      {
        MODIFY_REG(*ccr_reg, (XSPI_CCR_IMODE  | XSPI_CCR_IDTR  | XSPI_CCR_ISIZE  |
                                XSPI_CCR_ADMODE | XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE |
                                XSPI_CCR_DMODE  | XSPI_CCR_DDTR),
                   (pCmd->InstructionMode | pCmd->InstructionDTRMode | pCmd->InstructionWidth |
                    pCmd->AddressMode     | pCmd->AddressDTRMode     | pCmd->AddressWidth     |
                    pCmd->DataMode        | pCmd->DataDTRMode));
      }
      else
      {
        MODIFY_REG(*ccr_reg, (XSPI_CCR_IMODE  | XSPI_CCR_IDTR  | XSPI_CCR_ISIZE  |
                                XSPI_CCR_ADMODE | XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE),
                   (pCmd->InstructionMode | pCmd->InstructionDTRMode | pCmd->InstructionWidth |
                    pCmd->AddressMode     | pCmd->AddressDTRMode     | pCmd->AddressWidth));
      }
      *ir_reg = pCmd->Instruction;
      WRITE_REG(XSPIx->AR, pCmd->Address);
    }
    else
    {
      if (pCmd->DataMode != LL_XSPI_DATA_NONE)
      {
        MODIFY_REG(*ccr_reg, (XSPI_CCR_IMODE | XSPI_CCR_IDTR | XSPI_CCR_ISIZE |
                                XSPI_CCR_DMODE | XSPI_CCR_DDTR),
                   (pCmd->InstructionMode | pCmd->InstructionDTRMode | pCmd->InstructionWidth |
                    pCmd->DataMode        | pCmd->DataDTRMode));
      }
      else
      {
        MODIFY_REG(*ccr_reg, (XSPI_CCR_IMODE | XSPI_CCR_IDTR | XSPI_CCR_ISIZE),
                   (pCmd->InstructionMode | pCmd->InstructionDTRMode | pCmd->InstructionWidth));
      }
      *ir_reg = pCmd->Instruction;
    }
  }
  else
  {
    if (pCmd->AddressMode != LL_XSPI_ADDRESS_NONE)
    {
      if (pCmd->DataMode != LL_XSPI_DATA_NONE)
      {
        MODIFY_REG(*ccr_reg, (XSPI_CCR_ADMODE | XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE |
                                XSPI_CCR_DMODE  | XSPI_CCR_DDTR),
                   (pCmd->AddressMode | pCmd->AddressDTRMode | pCmd->AddressWidth     |
                    pCmd->DataMode    | pCmd->DataDTRMode));
      }
      else
      {
        MODIFY_REG(*ccr_reg, (XSPI_CCR_ADMODE | XSPI_CCR_ADDTR | XSPI_CCR_ADSIZE),
                   (pCmd->AddressMode | pCmd->AddressDTRMode | pCmd->AddressWidth));
      }
      WRITE_REG(XSPIx->AR, pCmd->Address);
    }
    else
    {
      return ERROR;
    }
  }

  /* For commands with no data phase, wait for completion */
  if (pCmd->DataMode == LL_XSPI_DATA_NONE)
  {
    while (READ_BIT(XSPIx->SR, XSPI_SR_BUSY) != 0U) {}
    WRITE_REG(XSPIx->FCR, XSPI_FCR_CTCF);
  }

  return SUCCESS;
}

/**
  * @}
  */

/**
  * @}
  */

#endif /* XSPI1 || XSPI2 || XSPI3 */

/**
  * @}
  */

#endif /* USE_FULL_LL_DRIVER */
