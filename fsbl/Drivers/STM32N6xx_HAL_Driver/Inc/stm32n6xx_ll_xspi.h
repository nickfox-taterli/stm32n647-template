/**
  ******************************************************************************
  * @file    stm32n6xx_ll_xspi.h
  * @brief   Header file of XSPI LL module.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32N6xx_LL_XSPI_H
#define STM32N6xx_LL_XSPI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32n6xx.h"


/* ========================================================================== */
/* XSPI type definitions for BSP drivers                                       */
/* ========================================================================== */

/* XSPI Regular Command structure */
typedef struct
{
  uint32_t OperationType;
  uint32_t IOSelect;
  uint32_t Instruction;
  uint32_t InstructionMode;
  uint32_t InstructionWidth;
  uint32_t InstructionDTRMode;
  uint32_t Address;
  uint32_t AddressMode;
  uint32_t AddressWidth;
  uint32_t AddressDTRMode;
  uint32_t AlternateBytes;
  uint32_t AlternateBytesMode;
  uint32_t AlternateBytesWidth;
  uint32_t AlternateBytesDTRMode;
  uint32_t DataMode;
  uint32_t DataLength;
  uint32_t DataDTRMode;
  uint32_t DummyCycles;
  uint32_t DQSMode;
} XSPI_RegularCmdTypeDef;

/* XSPI Hyperbus Configuration structure */
typedef struct
{
  uint32_t RWRecoveryTimeCycle;
  uint32_t AccessTimeCycle;
  uint32_t WriteZeroLatency;
  uint32_t LatencyMode;
} XSPI_HyperbusCfgTypeDef;

/* XSPI Hyperbus Command structure */
typedef struct
{
  uint32_t AddressSpace;
  uint32_t Address;
  uint32_t AddressWidth;
  uint32_t DataLength;
  uint32_t DQSMode;
  uint32_t DataMode;
} XSPI_HyperbusCmdTypeDef;

/* XSPI Memory Mapped mode structure */
typedef struct
{
  uint32_t TimeOutActivation;
  uint32_t TimeoutPeriodClock;
  uint32_t NoPrefetchData;
  uint32_t NoPrefetchAXI;
} XSPI_MemoryMappedTypeDef;

/* --- LL XSPI Operation Type --- */
#define LL_XSPI_OPTYPE_COMMON_CFG           (0x00000000U)
#define LL_XSPI_OPTYPE_READ_CFG             (0x00000001U)
#define LL_XSPI_OPTYPE_WRITE_CFG            (0x00000002U)
#define LL_XSPI_OPTYPE_WRAP_CFG             (0x00000003U)

/* --- LL XSPI IO Select --- */
#define LL_XSPI_SELECT_IO_3_0               (0x00000000U)
#define LL_XSPI_SELECT_IO_7_4               ((uint32_t)XSPI_CR_MSEL_0)
#define LL_XSPI_SELECT_IO_11_8              ((uint32_t)XSPI_CR_MSEL_1)
#define LL_XSPI_SELECT_IO_15_12             ((uint32_t)XSPI_CR_MSEL)
#define LL_XSPI_SELECT_IO_7_0               (0x00000000U)
#define LL_XSPI_SELECT_IO_15_8              ((uint32_t)XSPI_CR_MSEL_1)

/* --- LL XSPI Instruction Mode --- */
#define LL_XSPI_INSTRUCTION_NONE            (0x00000000U)
#define LL_XSPI_INSTRUCTION_1_LINE          ((uint32_t)XSPI_CCR_IMODE_0)
#define LL_XSPI_INSTRUCTION_2_LINES         ((uint32_t)XSPI_CCR_IMODE_1)
#define LL_XSPI_INSTRUCTION_4_LINES         ((uint32_t)(XSPI_CCR_IMODE_0 | XSPI_CCR_IMODE_1))
#define LL_XSPI_INSTRUCTION_8_LINES         ((uint32_t)XSPI_CCR_IMODE_2)

/* --- LL XSPI Instruction Width --- */
#define LL_XSPI_INSTRUCTION_8_BITS          (0x00000000U)
#define LL_XSPI_INSTRUCTION_16_BITS         ((uint32_t)XSPI_CCR_ISIZE_0)
#define LL_XSPI_INSTRUCTION_24_BITS         ((uint32_t)XSPI_CCR_ISIZE_1)
#define LL_XSPI_INSTRUCTION_32_BITS         ((uint32_t)XSPI_CCR_ISIZE)

/* --- LL XSPI Instruction DTR Mode --- */
#define LL_XSPI_INSTRUCTION_DTR_DISABLE     (0x00000000U)
#define LL_XSPI_INSTRUCTION_DTR_ENABLE      ((uint32_t)XSPI_CCR_IDTR)

/* --- LL XSPI Address Mode --- */
#define LL_XSPI_ADDRESS_NONE                (0x00000000U)
#define LL_XSPI_ADDRESS_1_LINE              ((uint32_t)XSPI_CCR_ADMODE_0)
#define LL_XSPI_ADDRESS_2_LINES             ((uint32_t)XSPI_CCR_ADMODE_1)
#define LL_XSPI_ADDRESS_4_LINES             ((uint32_t)(XSPI_CCR_ADMODE_0 | XSPI_CCR_ADMODE_1))
#define LL_XSPI_ADDRESS_8_LINES             ((uint32_t)XSPI_CCR_ADMODE_2)

/* --- LL XSPI Address Width --- */
#define LL_XSPI_ADDRESS_8_BITS              (0x00000000U)
#define LL_XSPI_ADDRESS_16_BITS             ((uint32_t)XSPI_CCR_ADSIZE_0)
#define LL_XSPI_ADDRESS_24_BITS             ((uint32_t)XSPI_CCR_ADSIZE_1)
#define LL_XSPI_ADDRESS_32_BITS             ((uint32_t)XSPI_CCR_ADSIZE)

/* --- LL XSPI Address DTR Mode --- */
#define LL_XSPI_ADDRESS_DTR_DISABLE         (0x00000000U)
#define LL_XSPI_ADDRESS_DTR_ENABLE          ((uint32_t)XSPI_CCR_ADDTR)

/* --- LL XSPI Alternate Bytes Mode --- */
#define LL_XSPI_ALT_BYTES_NONE              (0x00000000U)
#define LL_XSPI_ALT_BYTES_1_LINE            ((uint32_t)XSPI_CCR_ABMODE_0)
#define LL_XSPI_ALT_BYTES_2_LINES           ((uint32_t)XSPI_CCR_ABMODE_1)
#define LL_XSPI_ALT_BYTES_4_LINES           ((uint32_t)(XSPI_CCR_ABMODE_0 | XSPI_CCR_ABMODE_1))
#define LL_XSPI_ALT_BYTES_8_LINES           ((uint32_t)XSPI_CCR_ABMODE_2)

/* --- LL XSPI Alternate Bytes Width --- */
#define LL_XSPI_ALT_BYTES_8_BITS            (0x00000000U)
#define LL_XSPI_ALT_BYTES_16_BITS           ((uint32_t)XSPI_CCR_ABSIZE_0)
#define LL_XSPI_ALT_BYTES_24_BITS           ((uint32_t)XSPI_CCR_ABSIZE_1)
#define LL_XSPI_ALT_BYTES_32_BITS           ((uint32_t)XSPI_CCR_ABSIZE)

/* --- LL XSPI Alternate Bytes DTR Mode --- */
#define LL_XSPI_ALT_BYTES_DTR_DISABLE       (0x00000000U)
#define LL_XSPI_ALT_BYTES_DTR_ENABLE        ((uint32_t)XSPI_CCR_ABDTR)

/* --- LL XSPI Data Mode --- */
#define LL_XSPI_DATA_NONE                   (0x00000000U)
#define LL_XSPI_DATA_1_LINE                 ((uint32_t)XSPI_CCR_DMODE_0)
#define LL_XSPI_DATA_2_LINES                ((uint32_t)XSPI_CCR_DMODE_1)
#define LL_XSPI_DATA_4_LINES                ((uint32_t)(XSPI_CCR_DMODE_0 | XSPI_CCR_DMODE_1))
#define LL_XSPI_DATA_8_LINES                ((uint32_t)XSPI_CCR_DMODE_2)
#define LL_XSPI_DATA_16_LINES               ((uint32_t)(XSPI_CCR_DMODE_0 | XSPI_CCR_DMODE_2))

/* --- LL XSPI Data DTR Mode --- */
#define LL_XSPI_DATA_DTR_DISABLE            (0x00000000U)
#define LL_XSPI_DATA_DTR_ENABLE             ((uint32_t)XSPI_CCR_DDTR)

/* --- LL XSPI DQS Mode --- */
#define LL_XSPI_DQS_DISABLE                 (0x00000000U)
#define LL_XSPI_DQS_ENABLE                  ((uint32_t)XSPI_CCR_DQSE)

/* --- LL XSPI Hyperbus Address Space --- */
#define LL_XSPI_MEMORY_ADDRESS_SPACE        (0x00000000U)
#define LL_XSPI_REGISTER_ADDRESS_SPACE      ((uint32_t)XSPI_DCR1_MTYP_0)

/* --- LL XSPI Delay Hold Quarter Cycle --- */
#define LL_XSPI_DHQC_DISABLE                (0x00000000U)
#define LL_XSPI_DHQC_ENABLE                 ((uint32_t)XSPI_TCR_DHQC)

/* End of XSPI type definitions */


/** @addtogroup STM32N6xx_LL_Driver
  * @{
  */

#if defined (XSPI1) || defined (XSPI2) || defined (XSPI3)

/** @defgroup XSPI_LL XSPI
  * @{
  */

/* Exported types ------------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup XSPI_LL_ES_INIT XSPI LL Exported Init structure
  * @{
  */

/**
  * @brief LL XSPI Init structure definition
  */
typedef struct
{
  uint32_t FifoThresholdByte;       /*!< Threshold for FIFO (1..64) */
  uint32_t MemoryMode;              /*!< @ref XSPI_LL_EC_MEMORY_MODE */
  uint32_t MemoryType;              /*!< @ref XSPI_LL_EC_MEMORY_TYPE */
  uint32_t MemorySize;              /*!< @ref XSPI_LL_EC_MEMORY_SIZE */
  uint32_t ChipSelectHighTimeCycle; /*!< CS high time between commands (1..64) */
  uint32_t FreeRunningClock;        /*!< @ref XSPI_LL_EC_FREE_RUNNING_CLOCK */
  uint32_t ClockMode;               /*!< @ref XSPI_LL_EC_CLOCK_MODE */
  uint32_t WrapSize;                /*!< @ref XSPI_LL_EC_WRAP_SIZE */
  uint32_t ClockPrescaler;          /*!< Clock prescaler (0..255) */
  uint32_t SampleShifting;          /*!< @ref XSPI_LL_EC_SAMPLE_SHIFTING */
  uint32_t ChipSelectBoundary;      /*!< @ref XSPI_LL_EC_CS_BOUNDARY */
  uint32_t MaxTran;                 /*!< Max transfer for regulation (0..255) */
  uint32_t Refresh;                 /*!< Refresh rate (0..0xFFFFFFFF) */
  uint32_t MemorySelect;            /*!< @ref XSPI_LL_EC_MEMORY_SELECT */
  uint32_t MemoryExtended;          /*!< @ref XSPI_LL_EC_MEMORY_EXTENDED */
} LL_XSPI_InitTypeDef;

/**
  * @brief LL XSPIM IO Manager configuration structure
  */
typedef struct
{
  uint32_t nCSOverride;    /*!< CS override: @ref XSPIM_LL_EC_CS_OVERRIDE */
  uint32_t IOPort;         /*!< IO port: @ref XSPIM_LL_EC_IO_PORT */
  uint32_t Req2AckTime;    /*!< REQ2ACK time (1..256) */
} LL_XSPIM_CfgTypeDef;

/**
  * @brief LL XSPI HyperBus configuration structure
  */
typedef struct
{
  uint32_t RWRecoveryTimeCycle;  /*!< R/W recovery time (0..255) */
  uint32_t AccessTimeCycle;      /*!< Access time (0..255) */
  uint32_t WriteZeroLatency;     /*!< @ref XSPI_LL_EC_HYPERBUS_WRITE_LATENCY */
  uint32_t LatencyMode;          /*!< @ref XSPI_LL_EC_HYPERBUS_LATENCY_MODE */
} LL_XSPI_HyperbusCfgTypeDef;

/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/* Exported constants --------------------------------------------------------*/
/** @defgroup XSPI_LL_EC_MEMORY_MODE Memory Mode
  * @{
  */
#define LL_XSPI_SINGLE_MEM           (0x00000000U)
#define LL_XSPI_DUAL_MEM             (XSPI_CR_DMM)
/**
  * @}
  */

/** @defgroup XSPI_LL_EC_MEMORY_TYPE Memory Type
  * @{
  */
#define LL_XSPI_MEMTYPE_MICRON       (0x00000000U)
#define LL_XSPI_MEMTYPE_MACRONIX     (XSPI_DCR1_MTYP_0)
#define LL_XSPI_MEMTYPE_APMEM        (XSPI_DCR1_MTYP_1)
#define LL_XSPI_MEMTYPE_MACRONIX_RAM ((XSPI_DCR1_MTYP_1 | XSPI_DCR1_MTYP_0))
#define LL_XSPI_MEMTYPE_HYPERBUS     (XSPI_DCR1_MTYP_2)
#define LL_XSPI_MEMTYPE_APMEM_16BITS ((XSPI_DCR1_MTYP_2 | XSPI_DCR1_MTYP_1))
/**
  * @}
  */

/** @defgroup XSPI_LL_EC_MEMORY_SIZE Memory Size
  * @{
  */
#define LL_XSPI_SIZE_16B             (0x00000000U)
#define LL_XSPI_SIZE_32B             (0x00000001U)
#define LL_XSPI_SIZE_64B             (0x00000002U)
#define LL_XSPI_SIZE_128B            (0x00000003U)
#define LL_XSPI_SIZE_256B            (0x00000004U)
#define LL_XSPI_SIZE_512B            (0x00000005U)
#define LL_XSPI_SIZE_1KB             (0x00000006U)
#define LL_XSPI_SIZE_2KB             (0x00000007U)
#define LL_XSPI_SIZE_4KB             (0x00000008U)
#define LL_XSPI_SIZE_8KB             (0x00000009U)
#define LL_XSPI_SIZE_16KB            (0x0000000AU)
#define LL_XSPI_SIZE_32KB            (0x0000000BU)
#define LL_XSPI_SIZE_64KB            (0x0000000CU)
#define LL_XSPI_SIZE_128KB           (0x0000000DU)
#define LL_XSPI_SIZE_256KB           (0x0000000EU)
#define LL_XSPI_SIZE_512KB           (0x0000000FU)
#define LL_XSPI_SIZE_1MB             (0x00000010U)
#define LL_XSPI_SIZE_2MB             (0x00000011U)
#define LL_XSPI_SIZE_4MB             (0x00000012U)
#define LL_XSPI_SIZE_8MB             (0x00000013U)
#define LL_XSPI_SIZE_16MB            (0x00000014U)
#define LL_XSPI_SIZE_32MB            (0x00000015U)
#define LL_XSPI_SIZE_64MB            (0x00000016U)
#define LL_XSPI_SIZE_128MB           (0x00000017U)
#define LL_XSPI_SIZE_256MB           (0x00000018U)
#define LL_XSPI_SIZE_512MB           (0x00000019U)
#define LL_XSPI_SIZE_1GB             (0x0000001AU)
#define LL_XSPI_SIZE_2GB             (0x0000001BU)
#define LL_XSPI_SIZE_4GB             (0x0000001CU)
#define LL_XSPI_SIZE_8GB             (0x0000001DU)
#define LL_XSPI_SIZE_16GB            (0x0000001EU)
#define LL_XSPI_SIZE_32GB            (0x0000001FU)
/**
  * @}
  */

/** @defgroup XSPI_LL_EC_FREE_RUNNING_CLOCK Free Running Clock
  * @{
  */
#define LL_XSPI_FREERUNCLK_DISABLE   (0x00000000U)
#define LL_XSPI_FREERUNCLK_ENABLE    (XSPI_DCR1_FRCK)
/**
  * @}
  */

/** @defgroup XSPI_LL_EC_CLOCK_MODE Clock Mode
  * @{
  */
#define LL_XSPI_CLOCK_MODE_0         (0x00000000U)
#define LL_XSPI_CLOCK_MODE_3         (XSPI_DCR1_CKMODE)
/**
  * @}
  */

/** @defgroup XSPI_LL_EC_WRAP_SIZE Wrap Size
  * @{
  */
#define LL_XSPI_WRAP_NOT_SUPPORTED   (0x00000000U)
#define LL_XSPI_WRAP_16_BYTES        (XSPI_DCR2_WRAPSIZE_1)
#define LL_XSPI_WRAP_32_BYTES        ((uint32_t)(XSPI_DCR2_WRAPSIZE_0 | XSPI_DCR2_WRAPSIZE_1))
#define LL_XSPI_WRAP_64_BYTES        (XSPI_DCR2_WRAPSIZE_2)
#define LL_XSPI_WRAP_128_BYTES       ((uint32_t)(XSPI_DCR2_WRAPSIZE_0 | XSPI_DCR2_WRAPSIZE_2))
/**
  * @}
  */

/** @defgroup XSPI_LL_EC_SAMPLE_SHIFTING Sample Shifting
  * @{
  */
#define LL_XSPI_SAMPLE_SHIFT_NONE    (0x00000000U)
#define LL_XSPI_SAMPLE_SHIFT_HALFCYCLE (XSPI_TCR_SSHIFT)
/**
  * @}
  */

/** @defgroup XSPI_LL_EC_CS_BOUNDARY Chip Select Boundary
  * @{
  */
#define LL_XSPI_CSBOUND_NONE         (0x00000000U)
#define LL_XSPI_CSBOUND_16B          (0x00000001U)
#define LL_XSPI_CSBOUND_32B          (0x00000002U)
#define LL_XSPI_CSBOUND_64B          (0x00000003U)
#define LL_XSPI_CSBOUND_128B         (0x00000004U)
#define LL_XSPI_CSBOUND_256B         (0x00000005U)
#define LL_XSPI_CSBOUND_512B         (0x00000006U)
#define LL_XSPI_CSBOUND_1KB          (0x00000007U)
#define LL_XSPI_CSBOUND_2KB          (0x00000008U)
#define LL_XSPI_CSBOUND_4KB          (0x00000009U)
#define LL_XSPI_CSBOUND_8KB          (0x0000000AU)
#define LL_XSPI_CSBOUND_16KB         (0x0000000BU)
#define LL_XSPI_CSBOUND_32KB         (0x0000000CU)
#define LL_XSPI_CSBOUND_64KB         (0x0000000DU)
#define LL_XSPI_CSBOUND_128KB        (0x0000000EU)
#define LL_XSPI_CSBOUND_256KB        (0x0000000FU)
#define LL_XSPI_CSBOUND_512KB        (0x00000010U)
#define LL_XSPI_CSBOUND_1MB          (0x00000011U)
#define LL_XSPI_CSBOUND_2MB          (0x00000012U)
#define LL_XSPI_CSBOUND_4MB          (0x00000013U)
#define LL_XSPI_CSBOUND_8MB          (0x00000014U)
#define LL_XSPI_CSBOUND_16MB         (0x00000015U)
#define LL_XSPI_CSBOUND_32MB         (0x00000016U)
#define LL_XSPI_CSBOUND_64MB         (0x00000017U)
#define LL_XSPI_CSBOUND_128MB        (0x00000018U)
#define LL_XSPI_CSBOUND_256MB        (0x00000019U)
#define LL_XSPI_CSBOUND_512MB        (0x0000001AU)
#define LL_XSPI_CSBOUND_1GB          (0x0000001BU)
#define LL_XSPI_CSBOUND_2GB          (0x0000001CU)
#define LL_XSPI_CSBOUND_4GB          (0x0000001DU)
#define LL_XSPI_CSBOUND_8GB          (0x0000001EU)
#define LL_XSPI_CSBOUND_16GB         (0x0000001FU)
/**
  * @}
  */

/** @defgroup XSPI_LL_EC_MEMORY_SELECT Memory Select
  * @{
  */
#define LL_XSPI_CSSEL_NCS1           (0x00000000U)
#define LL_XSPI_CSSEL_NCS2           (XSPI_CR_CSSEL)
/**
  * @}
  */

/** @defgroup XSPI_LL_EC_MEMORY_EXTENDED Memory Extended
  * @{
  */
#define LL_XSPI_MEMEXT_SW            (0x00000000U)
#define LL_XSPI_MEMEXT_HW            (XSPI_DCR1_EXTENDMEM)
/**
  * @}
  */

/** @defgroup XSPIM_LL_EC_IO_PORT XSPIM IO Port Selection
  * @{
  */
#define LL_XSPIM_IOPORT_1              (0x00000000U)
#define LL_XSPIM_IOPORT_2              (0x00000001U)
/**
  * @}
  */

/** @defgroup XSPIM_LL_EC_CS_OVERRIDE XSPIM Chip Select Override
  * @{
  */
#define LL_XSPIM_CSSEL_OVR_DISABLED    (0x00000000U)
#define LL_XSPIM_CSSEL_OVR_NCS1        (XSPIM_CR_CSSEL_OVR_EN)
#define LL_XSPIM_CSSEL_OVR_NCS2        (XSPIM_CR_CSSEL_OVR_EN | XSPIM_CR_CSSEL_OVR_O1 | XSPIM_CR_CSSEL_OVR_O2)
/**
  * @}
  */

/** @defgroup XSPI_LL_EC_HYPERBUS_WRITE_LATENCY HyperBus Write Latency
  * @{
  */
#define LL_XSPI_LATENCY_ON_WRITE       (0x00000000U)
#define LL_XSPI_NO_LATENCY_ON_WRITE    (XSPI_HLCR_WZL)
/**
  * @}
  */

/** @defgroup XSPI_LL_EC_HYPERBUS_LATENCY_MODE HyperBus Latency Mode
  * @{
  */
#define LL_XSPI_VARIABLE_LATENCY       (0x00000000U)
#define LL_XSPI_FIXED_LATENCY          (XSPI_HLCR_LM)
/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup XSPI_LL_EM_MISC Miscellaneous
  * @{
  */

/** @brief  Enable XSPI peripheral.
  * @param  __INSTANCE__ XSPI instance
  * @retval None
  */
#define LL_XSPI_Enable(__INSTANCE__)       SET_BIT((__INSTANCE__)->CR, XSPI_CR_EN)

/** @brief  Disable XSPI peripheral.
  * @param  __INSTANCE__ XSPI instance
  * @retval None
  */
#define LL_XSPI_Disable(__INSTANCE__)      CLEAR_BIT((__INSTANCE__)->CR, XSPI_CR_EN)

/** @brief  Check if XSPI is enabled.
  * @param  __INSTANCE__ XSPI instance
  * @retval State (0 or 1)
  */
#define LL_XSPI_IsEnabled(__INSTANCE__)    (READ_BIT((__INSTANCE__)->CR, XSPI_CR_EN) == XSPI_CR_EN)

/** @brief  Check if XSPI busy flag is set.
  * @param  __INSTANCE__ XSPI instance
  * @retval State (0 or 1)
  */
#define LL_XSPI_IsActiveFlag_BUSY(__INSTANCE__) \
  (READ_BIT((__INSTANCE__)->SR, XSPI_SR_BUSY) == XSPI_SR_BUSY)

/** @brief  Set XSPIM REQ2ACK_TIME.
  * @param  __TIME__ Value (0..255) to write to REQ2ACK_TIME field
  */
#define LL_XSPIM_SetReq2AckTime(__TIME__) \
  MODIFY_REG(XSPIM->CR, XSPIM_CR_REQ2ACK_TIME, \
             ((__TIME__) << XSPIM_CR_REQ2ACK_TIME_Pos))

/** @brief  Configure XSPI1 CS Override (OVR_O1 + OVR_EN bits).
  * @param  __OVR__ CS override value (@ref XSPIM_LL_EC_CS_OVERRIDE)
  */
#define LL_XSPIM_SetCSOverride_XSPI1(__OVR__) \
  MODIFY_REG(XSPIM->CR, (XSPIM_CR_CSSEL_OVR_O1 | XSPIM_CR_CSSEL_OVR_EN), (__OVR__))

/** @brief  Configure XSPI2 CS Override (OVR_O2 + OVR_EN bits).
  * @param  __OVR__ CS override value (@ref XSPIM_LL_EC_CS_OVERRIDE)
  */
#define LL_XSPIM_SetCSOverride_XSPI2(__OVR__) \
  MODIFY_REG(XSPIM->CR, (XSPIM_CR_CSSEL_OVR_O2 | XSPIM_CR_CSSEL_OVR_EN), (__OVR__))

/** @brief  Enable XSPIM multiplexed mode (dual instance sharing same IO port).
  */
#define LL_XSPIM_EnableMuxMode()   SET_BIT(XSPIM->CR, XSPIM_CR_MUXEN)

/** @brief  Disable XSPIM multiplexed mode.
  */
#define LL_XSPIM_DisableMuxMode()  CLEAR_BIT(XSPIM->CR, XSPIM_CR_MUXEN)

/** @brief  Set XSPIM mode (IO port routing).
  */
#define LL_XSPIM_SetMode()         SET_BIT(XSPIM->CR, XSPIM_CR_MODE)

/** @brief  Clear XSPIM mode.
  */
#define LL_XSPIM_ClearMode()       CLEAR_BIT(XSPIM->CR, XSPIM_CR_MODE)

/** @brief  Set clock prescaler.
  * @param  __INSTANCE__ XSPI instance
  * @param  __PRESCALER__ Value (0..255) for DCR2 PRESCALER field
  */
#define LL_XSPI_SetClockPrescaler(__INSTANCE__, __PRESCALER__) \
  MODIFY_REG((__INSTANCE__)->DCR2, XSPI_DCR2_PRESCALER, \
             ((__PRESCALER__) << XSPI_DCR2_PRESCALER_Pos))

/* === Status flag checks === */
#define LL_XSPI_IsActiveFlag_TC(__INSTANCE__)   (READ_BIT((__INSTANCE__)->SR, XSPI_SR_TCF) != 0U)
#define LL_XSPI_IsActiveFlag_FT(__INSTANCE__)   (READ_BIT((__INSTANCE__)->SR, XSPI_SR_FTF) != 0U)
#define LL_XSPI_IsActiveFlag_SM(__INSTANCE__)   (READ_BIT((__INSTANCE__)->SR, XSPI_SR_SMF) != 0U)
#define LL_XSPI_IsActiveFlag_TE(__INSTANCE__)   (READ_BIT((__INSTANCE__)->SR, XSPI_SR_TEF) != 0U)
#define LL_XSPI_IsActiveFlag_TO(__INSTANCE__)   (READ_BIT((__INSTANCE__)->SR, XSPI_SR_TOF) != 0U)

/* === Status flag clear === */
#define LL_XSPI_ClearFlag_TC(__INSTANCE__)      WRITE_REG((__INSTANCE__)->FCR, XSPI_FCR_CTCF)
#define LL_XSPI_ClearFlag_SM(__INSTANCE__)      WRITE_REG((__INSTANCE__)->FCR, XSPI_FCR_CSMF)
#define LL_XSPI_ClearFlag_TE(__INSTANCE__)      WRITE_REG((__INSTANCE__)->FCR, XSPI_FCR_CTEF)
#define LL_XSPI_ClearFlag_TO(__INSTANCE__)      WRITE_REG((__INSTANCE__)->FCR, XSPI_FCR_CTOF)

/* === Functional mode constants === */
#define LL_XSPI_MODE_INDIRECT_WRITE             (0x00000000U)
#define LL_XSPI_MODE_INDIRECT_READ              (XSPI_CR_FMODE_0)
#define LL_XSPI_MODE_AUTO_POLLING               (XSPI_CR_FMODE_1)
#define LL_XSPI_MODE_MEMORY_MAPPED              (XSPI_CR_FMODE)

/* === Functional mode operations === */
#define LL_XSPI_SetFunctionalMode(__INSTANCE__, __MODE__) \
  MODIFY_REG((__INSTANCE__)->CR, XSPI_CR_FMODE, (__MODE__))

#define LL_XSPI_GetFunctionalMode(__INSTANCE__) \
  READ_BIT((__INSTANCE__)->CR, XSPI_CR_FMODE)

/* === Data length === */
#define LL_XSPI_SetDataLength(__INSTANCE__, __LENGTH__) \
  WRITE_REG((__INSTANCE__)->DLR, (__LENGTH__))

/* === Data register (8-bit access) === */
#define LL_XSPI_WriteData8(__INSTANCE__, __DATA__) \
  (*((__IO uint8_t *)&(__INSTANCE__)->DR) = (__DATA__))

#define LL_XSPI_ReadData8(__INSTANCE__) \
  (*((__IO uint8_t *)&(__INSTANCE__)->DR))

/* === Address / Instruction registers === */
#define LL_XSPI_SetAddress(__INSTANCE__, __ADDR__) \
  WRITE_REG((__INSTANCE__)->AR, (__ADDR__))

#define LL_XSPI_SetInstruction(__INSTANCE__, __INSTR__) \
  WRITE_REG((__INSTANCE__)->IR, (__INSTR__))

/** @brief  Configure HyperBus Latency register in one write.
  * @param  __INSTANCE__ XSPI instance
  * @param  __TRWR__ R/W recovery time (0..255)
  * @param  __TACC__ Access time (0..255)
  * @param  __WZL__  Write zero latency (@ref XSPI_LL_EC_HYPERBUS_WRITE_LATENCY)
  * @param  __LM__   Latency mode (@ref XSPI_LL_EC_HYPERBUS_LATENCY_MODE)
  */
#define LL_XSPI_SetHyperbusConfig(__INSTANCE__, __TRWR__, __TACC__, __WZL__, __LM__) \
  WRITE_REG((__INSTANCE__)->HLCR, \
            ((__TRWR__) << XSPI_HLCR_TRWR_Pos) | \
            ((__TACC__) << XSPI_HLCR_TACC_Pos) | (__WZL__) | (__LM__))

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup XSPI_LL_EF_Init Initialization and de-initialization functions
  * @{
  */

ErrorStatus LL_XSPI_Init(XSPI_TypeDef *XSPIx, const LL_XSPI_InitTypeDef *XSPI_InitStruct);
ErrorStatus LL_XSPI_ConfigRegularCmd(XSPI_TypeDef *XSPIx, uint32_t MemoryMode,
                                       uint32_t SampleShifting,
                                       const XSPI_RegularCmdTypeDef *pCmd);
void        LL_XSPI_StructInit(LL_XSPI_InitTypeDef *XSPI_InitStruct);
void        LL_XSPI_HyperbusCfg(XSPI_TypeDef *XSPIx, const LL_XSPI_HyperbusCfgTypeDef *pCfg);
void        LL_XSPI_Abort(XSPI_TypeDef *XSPIx);
void        LL_XSPI_HyperbusCmd(XSPI_TypeDef *XSPIx, uint32_t AddressSpace,
                                 uint32_t Address, uint32_t DataLength,
                                 uint32_t DQSMode, uint32_t DataMode,
                                 uint32_t AddressWidth);
void        LL_XSPI_Transmit(XSPI_TypeDef *XSPIx, const uint8_t *pData);
void        LL_XSPI_Receive(XSPI_TypeDef *XSPIx, uint8_t *pData, uint32_t MemoryType);
void        LL_XSPI_EnableMemoryMappedMode(XSPI_TypeDef *XSPIx, uint32_t TimeOutActivation,
                                            uint32_t TimeoutPeriod, uint32_t NoPrefetchData,
                                            uint32_t NoPrefetchAXI);
void        LL_XSPI_AutoPolling(XSPI_TypeDef *XSPIx, uint32_t MatchValue,
                                 uint32_t MatchMask, uint32_t IntervalTime,
                                 uint32_t MatchMode, uint32_t AutomaticStop,
                                 uint32_t MemoryType);

/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

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

#ifdef __cplusplus
}
#endif

#endif /* STM32N6xx_LL_XSPI_H */
