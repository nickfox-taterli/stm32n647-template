#include "stm32n647xx.h"
#include "stm32n6xx_ll_utils.h"

#include "FreeRTOS.h"
#include "task.h"
#include "board_init.h"
#include "app_tasks.h"
#include "serial_console.h"
#include "shell_port.h"
#include "sdmmc_drv.h"
#include "ai_instance_segmentation.h"

int main(void)
{
  SCB_EnableICache();
  SCB_EnableDCache();

  NVIC_SetPriorityGrouping(3);

  SystemCoreClockUpdate();
  LL_Init1msTick(SystemCoreClock);

  MX_GPIO_Init();
  SystemIsolation_Config();
  serial_console_init();
  shell_port_start();

  xTaskCreate(SD_InitTask, "sd_init", 768, NULL, 2, NULL);
  xTaskCreate(CameraTask, "camera", 2048, NULL, 2, NULL);
  xTaskCreate(AIInstanceSegmentationTask, "ai_iseg", 2048, NULL, 3, NULL);

  BaseType_t ok = xTaskCreate(LedTask, "led", 256, NULL, 1, NULL);
  configASSERT(ok == pdPASS);

  vTaskStartScheduler();

  while (1)
  {
  }
}
