/**
  ******************************************************************************
  * @file           : stm32_boot_xip.h
  * @brief          : Header for stm32_boot_xip.c file.
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
#ifndef __XIPBOOT__H__
#define __XIPBOOT__H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Exported types ------------------------------------------------------------*/
/**
  * @brief list of error code of XIP
  */
typedef enum {
     BOOT_OK,
     BOOT_ERROR_UNSUPPORTED_MEMORY,
     BOOT_ERROR_NOBASEADDRESS,
     BOOT_ERROR_MAPPEDMODEFAIL,
     BOOT_ERROR_INCOMPATIBLEMEMORY,
     BOOT_ERROR_DRIVER,
} BOOTStatus_TypeDef;

/* Exported functions --------------------------------------------------------*/
/**
  * @brief Boot on the application: map the NOR flash and jump to the app.
  * @return @ref BOOTStatus_TypeDef
  */
BOOTStatus_TypeDef BOOT_Application(void);

#ifdef __cplusplus
}
#endif

#endif /* __XIPBOOT__H__ */
