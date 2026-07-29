#include "stm32n647xx.h"
#include "stm32n6xx_ll_utils.h"

#include "FreeRTOS.h"
#include "task.h"
#include "board_init.h"
#include "app_tasks.h"
#include "serial_console.h"
#include "shell_port.h"
#include "ai_instance_segmentation.h"
#include "sdmmc_drv.h"

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

  /* Initialize shared storage before starting users of SDMMC2. */
  BaseType_t sd_ok = xTaskCreate(SD_InitTask, "sd_init", 1024, NULL, 5, NULL);
  configASSERT(sd_ok == pdPASS);

  BaseType_t usb_ok = xTaskCreate(USBDeviceTask, "usb_msc", 1536, NULL, 4, NULL);
  configASSERT(usb_ok == pdPASS);

  /* Camera -> DCMIPP Pipe2 -> NPU -> LTDC test. */
  BaseType_t ai_ok = xTaskCreate(AIInstanceSegmentationTask,
                                 "ai_iseg", 2048, NULL, 3, NULL);
  BaseType_t camera_ok = xTaskCreate(CameraTask,
                                     "camera", 2048, NULL, 2, NULL);
  configASSERT(ai_ok == pdPASS);
  configASSERT(camera_ok == pdPASS);

  BaseType_t ok = xTaskCreate(LedTask, "led", 256, NULL, 1, NULL);
  configASSERT(ok == pdPASS);

  vTaskStartScheduler();

  while (1)
  {
  }
}
