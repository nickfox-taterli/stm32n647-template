#include "stm32n6xx_it.h"
#include "camera_demo.h"
#include "serial_console.h"
#include "tusb.h"

void NMI_Handler(void)
{
  while (1)
  {
  }
}

void HardFault_Handler(void)
{
  while (1)
  {
  }
}

void MemManage_Handler(void)
{
  while (1)
  {
  }
}

void BusFault_Handler(void)
{
  while (1)
  {
  }
}

void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

void SecureFault_Handler(void)
{
  while (1)
  {
  }
}

void DebugMon_Handler(void)
{
}

void USART1_IRQHandler(void)
{
  serial_console_irq_handler();
}

void DCMIPP_IRQHandler(void)
{
  CameraDemo_DCMIPP_IRQHandler();
}

void USB1_OTG_HS_IRQHandler(void)
{
  tud_int_handler(0);
}
