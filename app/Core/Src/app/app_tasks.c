#include "app_tasks.h"
#include "rgb_lcd.h"
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

void LcdColorTask(void *argument)
{
  static const uint16_t colors[] = {
    0xF800U, /* red */
    0x07E0U, /* green */
    0x001FU, /* blue */
    0xFFFFU, /* white */
    0x0000U, /* black */
  };
  uint32_t index = 0;

  (void)argument;

  RGB_LCD_Init();

  while (1)
  {
    RGB_LCD_Fill(colors[index]);
    index++;
    if (index >= (sizeof(colors) / sizeof(colors[0])))
    {
      index = 0;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
