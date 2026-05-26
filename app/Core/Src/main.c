#include "stm32n647xx.h"

#include "stm32n6xx_ll_bus.h"
#include "stm32n6xx_ll_cortex.h"
#include "stm32n6xx_ll_gpio.h"
#include "stm32n6xx_ll_pwr.h"
#include "stm32n6xx_ll_rcc.h"
#include "stm32n6xx_ll_system.h"
#include "stm32n6xx_ll_utils.h"

#include "FreeRTOS.h"
#include "task.h"
#include "serial_console.h"
#include "shell_port.h"

static void MX_GPIO_Init(void);
static void SystemIsolation_Config(void);
static void LedTask(void *argument);

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

  BaseType_t ok = xTaskCreate(LedTask, "led", 256, NULL, 1, NULL);
  configASSERT(ok == pdPASS);

  vTaskStartScheduler();

  while (1)
  {
  }
}

static void LedTask(void *argument)
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

static void SystemIsolation_Config(void)
{
  LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_RIFSC);

  LL_PWR_ConfigSecure(LL_PWR_SEC0_SEC);
  LL_PWR_ConfigPrivilege(0);

  LL_GPIO_EnablePinSecure(GPIOE, LL_GPIO_PIN_10);
  LL_GPIO_DisablePinPrivilege(GPIOE, LL_GPIO_PIN_10);

  LL_GPIO_EnablePinSecure(GPIOG, LL_GPIO_PIN_10);
  LL_GPIO_DisablePinPrivilege(GPIOG, LL_GPIO_PIN_10);

  LL_GPIO_EnablePinSecure(GPIOE, LL_GPIO_PIN_5 | LL_GPIO_PIN_6);
  LL_GPIO_DisablePinPrivilege(GPIOE, LL_GPIO_PIN_5 | LL_GPIO_PIN_6);
}

static void MX_GPIO_Init(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOE);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOG);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOO);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOP);

  LL_GPIO_SetOutputPin(GPIOE, LL_GPIO_PIN_10);
  LL_GPIO_SetOutputPin(GPIOG, LL_GPIO_PIN_10);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
  LL_GPIO_Init(GPIOG, &GPIO_InitStruct);
}
