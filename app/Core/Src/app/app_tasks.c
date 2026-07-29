#include "app_tasks.h"
#include "camera_demo.h"
#include "console_log.h"
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

void CameraTask(void *argument)
{
  uint16_t sensor_id = 0U;
  CameraDemoStatus status;

  (void)argument;

  RGB_LCD_Init();
  RGB_LCD_Fill(0x0000U);

  LOG_DEBUG("camera: init\r\n");
  status = CameraDemo_Init(&sensor_id);
  LOG_INFO("camera: init status=%u id=0x%04x\r\n", (unsigned int)status, sensor_id);

  while (status == CAMERA_DEMO_OK)
  {
    status = CameraDemo_CaptureToLcd();
    if (status != CAMERA_DEMO_OK)
    {
      LOG_WARN("camera: capture status=%u\r\n", (unsigned int)status);
      break;
    }
  }

  while (1)
  {
    LL_GPIO_TogglePin(GPIOE, LL_GPIO_PIN_10);
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}
