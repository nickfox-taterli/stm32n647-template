#include "app_tasks.h"
#include "stm32n6xx_ll_gpio.h"
#include "FreeRTOS.h"
#include "task.h"

void LedTask(void *argument)
{
  (void)argument;

  while (1)
  {
    LL_GPIO_ResetOutputPin(GPIOG, LL_GPIO_PIN_10);
    LL_GPIO_SetOutputPin(GPIOE, LL_GPIO_PIN_10);
    vTaskDelay(pdMS_TO_TICKS(500));
    LL_GPIO_SetOutputPin(GPIOG, LL_GPIO_PIN_10);
    LL_GPIO_ResetOutputPin(GPIOE, LL_GPIO_PIN_10);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
