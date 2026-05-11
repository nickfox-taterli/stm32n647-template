/**
  ******************************************************************************
  * @file    stm32_boot_xip.c
  * @brief   Boot in execute-in-place mode using direct NORFlash calls.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32_boot_xip.h"
#include "norflash.h"

/** @addtogroup BOOT
  * @{
  */

/** @addtogroup BOOT_XIP
  * @{
  */

/* Private defines -----------------------------------------------------------*/
#define XIP_IMAGE_OFFSET  0x10000

/* Private variables ---------------------------------------------------------*/
extern NORFlash_ObjectTypeDef NORFlashObject;

/* Private function prototypes -----------------------------------------------*/
BOOTStatus_TypeDef JumpToApplication(void);

/**
  *  @addtogroup BOOT_XIP_Exported_Functions Boot XIP exported functions
  * @{
  */

BOOTStatus_TypeDef BOOT_Application(void)
{
  uint32_t BaseAddress;

  /* Enable memory-mapped mode on the NOR flash */
  if (NORFlash_EnableMemoryMappedMode(XSPI2, &NORFlashObject) != NORFlash_OK)
  {
    return BOOT_ERROR_MAPPEDMODEFAIL;
  }

  /* Get the mapped base address */
  if (NORFlash_GetMemoryMappedAddress(XSPI2, &BaseAddress) != NORFlash_OK)
  {
    return BOOT_ERROR_NOBASEADDRESS;
  }

  /* Jump to the application */
  return JumpToApplication();
}

/**
  * @}
  */

/**
  *  @defgroup BOOT_XIP_Private_Functions Boot XIP private functions
  * @{
  */

/**
  * @brief  This function jumps to the application through its vector table
  * @return @ref BOOTStatus_TypeDef
  */
BOOTStatus_TypeDef JumpToApplication(void)
{
  uint32_t primask_bit;
  typedef  void (*pFunction)(void);
  static pFunction JumpToApp;
  uint32_t Application_vector;

  if (NORFlash_GetMemoryMappedAddress(XSPI2, &Application_vector) != NORFlash_OK)
  {
      return BOOT_ERROR_INCOMPATIBLEMEMORY;
  }

#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1U)
  /* if I-Cache is enabled, disable I-Cache-----------------------------------*/
  if (SCB->CCR & SCB_CCR_IC_Msk)
  {
    SCB_DisableICache();
  }
#endif /* defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1U) */

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
  /* if D-Cache is enabled, disable D-Cache-----------------------------------*/
  if (SCB->CCR & SCB_CCR_DC_Msk)
  {
    SCB_DisableDCache();
  }
#endif /* defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U) */

  /* Initialize user application's Stack Pointer & Jump to user application  */
  primask_bit = __get_PRIMASK();
  __disable_irq();

  /* Apply offsets for image location and vector table offset */
  Application_vector += XIP_IMAGE_OFFSET;

  SCB->VTOR = (uint32_t)Application_vector;
  JumpToApp = (pFunction) (*(__IO uint32_t *)(Application_vector + 4u));

#if ((defined (__ARM_ARCH_8M_MAIN__ ) && (__ARM_ARCH_8M_MAIN__ == 1)) || \
     (defined (__ARM_ARCH_8_1M_MAIN__ ) && (__ARM_ARCH_8_1M_MAIN__ == 1)) || \
     (defined (__ARM_ARCH_8M_BASE__ ) && (__ARM_ARCH_8M_BASE__ == 1))    )
  /* on ARM v8m, set MSPLIM before setting MSP to avoid unwanted stack overflow faults */
  __set_MSPLIM(0x00000000);
#endif  /* __ARM_ARCH_8M_MAIN__ or __ARM_ARCH_8M_BASE__ */

  __set_MSP(*(__IO uint32_t*) Application_vector);

  /* Re-enable the interrupts */
  __set_PRIMASK(primask_bit);

  JumpToApp();
  return BOOT_OK;
}

/**
  * @}
  */

 /**
  * @}
  */

/**
  * @}
  */
