#include "app_tasks.h"
#include "camera_demo.h"
#include "rgb_lcd.h"
#include "serial_console.h"
#include "stm32n6xx_ll_gpio.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <string.h>

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

static void camera_log(const char *text)
{
  (void)serial_console_write(text, (unsigned short)strlen(text));
}

void CameraTask(void *argument)
{
  char line[96];
  uint16_t sensor_id = 0U;
  uint32_t frame_count = 0U;
  CameraDemoStatus status;

  (void)argument;

  RGB_LCD_Init();
  RGB_LCD_Fill(0x0000U);

  camera_log("camera: init\r\n");
  status = CameraDemo_Init(&sensor_id);
  (void)snprintf(line, sizeof(line), "camera: init status=%u id=0x%04x\r\n",
                 (unsigned int)status, sensor_id);
  camera_log(line);

  while (status == CAMERA_DEMO_OK)
  {
    status = CameraDemo_CaptureToLcd();
    if (status != CAMERA_DEMO_OK)
    {
      CameraDemoDebug debug;

      CameraDemo_GetDebug(&debug);
      (void)snprintf(line, sizeof(line), "camera: capture status=%u p1sr=0x%08lx p1dmcr=0x%08lx\r\n",
                     (unsigned int)status,
                     (unsigned long)debug.dcmipp_p1sr,
                     (unsigned long)debug.dcmipp_p1dmcr);
      camera_log(line);
      (void)snprintf(line, sizeof(line), "camera: csi sr0=0x%08lx err1=0x%08lx err2=0x%08lx\r\n",
                     (unsigned long)debug.csi_sr0,
                     (unsigned long)debug.csi_err1,
                     (unsigned long)debug.csi_err2);
      camera_log(line);
      (void)snprintf(line, sizeof(line), "camera: p1ppcr=0x%08lx p1decr=0x%08lx\r\n",
                     (unsigned long)debug.dcmipp_p1ppcr,
                     (unsigned long)debug.dcmipp_p1decr);
      camera_log(line);
      break;
    }
    frame_count++;
    if ((frame_count == 1U) || ((frame_count % 30U) == 0U))
    {
      CameraDemoDebug debug;

      CameraDemo_GetDebug(&debug);
      (void)snprintf(line, sizeof(line), "camera: frame=%lu p1sr=0x%08lx p1ppcr=0x%08lx fb=%04x..%04x\r\n",
                     (unsigned long)frame_count,
                     (unsigned long)debug.dcmipp_p1sr,
                     (unsigned long)debug.dcmipp_p1ppcr,
                     (unsigned int)debug.fb_min,
                     (unsigned int)debug.fb_max);
      camera_log(line);
    }
  }

  while (1)
  {
    LL_GPIO_TogglePin(GPIOE, LL_GPIO_PIN_10);
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}
