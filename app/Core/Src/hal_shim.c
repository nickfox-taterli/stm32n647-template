#include "stm32n6xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32n6xx_ll_rcc.h"

__IO uint32_t uwTick = 0;
uint32_t uwTickPrio = 0;
HAL_TickFreqTypeDef uwTickFreq = HAL_TICK_FREQ_DEFAULT;

uint32_t HAL_GetTick(void)
{
    return (uint32_t)xTaskGetTickCount();
}

void HAL_Delay(uint32_t Delay)
{
    vTaskDelay(pdMS_TO_TICKS(Delay));
}

void HAL_IncTick(void)
{
    uwTick += (uint32_t)uwTickFreq;
}

uint32_t HAL_RCCEx_GetPeriphCLKFreq(uint32_t PeriphClk)
{
    if (PeriphClk == RCC_PERIPHCLK_SDMMC1 || PeriphClk == RCC_PERIPHCLK_SDMMC2) {
        return SystemCoreClock >> 1; /* HCLK = 300MHz */
    }
    return 0;
}
