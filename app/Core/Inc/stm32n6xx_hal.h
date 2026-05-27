#ifndef STM32N6xx_HAL_H
#define STM32N6xx_HAL_H

#include "stm32n6xx_hal_conf.h"
#include "stm32n6xx_hal_def.h"

#ifdef HAL_SD_MODULE_ENABLED
#include "stm32n6xx_hal_sd.h"
#endif

extern __IO uint32_t uwTick;
extern uint32_t uwTickPrio;
extern HAL_TickFreqTypeDef uwTickFreq;

uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t Delay);
void HAL_IncTick(void);
uint32_t HAL_RCCEx_GetPeriphCLKFreq(uint32_t PeriphClk);

#endif /* STM32N6xx_HAL_H */
