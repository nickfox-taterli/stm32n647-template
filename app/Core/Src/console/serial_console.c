#include "serial_console.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lwrb/lwrb.h"

#include "stm32n6xx_ll_bus.h"
#include "stm32n6xx_ll_gpio.h"
#include "stm32n6xx_ll_rcc.h"
#include "stm32n6xx_ll_usart.h"

#define SERIAL_USART USART1
#define SERIAL_IRQ USART1_IRQn
#define SERIAL_BAUDRATE 115200U
#define SERIAL_RX_BUFFER_SIZE 512U
#define SERIAL_TX_BUFFER_SIZE 512U
#define SERIAL_RX_NOTIFY_INDEX 0U
#define SERIAL_TX_NOTIFY_INDEX 1U

static lwrb_t rx_rb;
static lwrb_t tx_rb;
static uint8_t rx_rb_storage[SERIAL_RX_BUFFER_SIZE];
static uint8_t tx_rb_storage[SERIAL_TX_BUFFER_SIZE];
static TaskHandle_t rx_wait_task;
static TaskHandle_t tx_wait_task;

void serial_console_init(void)
{
  LL_USART_InitTypeDef usart_init = {0};
  LL_GPIO_InitTypeDef gpio_init = {0};

  configASSERT(lwrb_init(&rx_rb, rx_rb_storage, sizeof(rx_rb_storage)) == 1U);
  configASSERT(lwrb_init(&tx_rb, tx_rb_storage, sizeof(tx_rb_storage)) == 1U);

  /* The APP inherits the FSBL clock tree.  PCLK2 consequently is not a
   * stable console reference across XIP/debug starts; use the 64 MHz HSI
   * kernel clock instead so the host can always use 115200/8N1. */
  LL_RCC_HSI_Enable();
  while (LL_RCC_HSI_IsReady() == 0U) {}
  LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_HSI);
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
  LL_APB2_GRP1_ForceReset(LL_APB2_GRP1_PERIPH_USART1);
  LL_APB2_GRP1_ReleaseReset(LL_APB2_GRP1_PERIPH_USART1);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOE);

  gpio_init.Pin = LL_GPIO_PIN_5 | LL_GPIO_PIN_6;
  gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
  gpio_init.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio_init.Pull = LL_GPIO_PULL_UP;
  gpio_init.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOE, &gpio_init);

  usart_init.PrescalerValue = LL_USART_PRESCALER_DIV1;
  usart_init.BaudRate = SERIAL_BAUDRATE;
  usart_init.DataWidth = LL_USART_DATAWIDTH_8B;
  usart_init.StopBits = LL_USART_STOPBITS_1;
  usart_init.Parity = LL_USART_PARITY_NONE;
  usart_init.TransferDirection = LL_USART_DIRECTION_TX_RX;
  usart_init.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  usart_init.OverSampling = LL_USART_OVERSAMPLING_16;
  (void)LL_USART_Init(SERIAL_USART, &usart_init);

  LL_USART_ConfigAsyncMode(SERIAL_USART);
  LL_USART_ConfigFIFOsThreshold(SERIAL_USART,
                                LL_USART_FIFOTHRESHOLD_1_8,
                                LL_USART_FIFOTHRESHOLD_1_2);
  LL_USART_EnableFIFO(SERIAL_USART);
  LL_USART_EnableIT_RXFT(SERIAL_USART);
  LL_USART_EnableIT_IDLE(SERIAL_USART);
  LL_USART_EnableIT_ERROR(SERIAL_USART);
  LL_USART_Enable(SERIAL_USART);
  while ((LL_USART_IsActiveFlag_TEACK(SERIAL_USART) == 0U) ||
         (LL_USART_IsActiveFlag_REACK(SERIAL_USART) == 0U)) {}

  NVIC_SetPriority(SERIAL_IRQ, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 6U, 0U));
  NVIC_EnableIRQ(SERIAL_IRQ);
}

int serial_console_read(char *data, unsigned short len)
{
  lwrb_sz_t read_len;

  if ((data == NULL) || (len == 0U))
  {
    return 0;
  }

  do
  {
    taskENTER_CRITICAL();
    read_len = lwrb_read(&rx_rb, data, len);
    if (read_len == 0U)
    {
      rx_wait_task = xTaskGetCurrentTaskHandle();
    }
    taskEXIT_CRITICAL();

    if (read_len == 0U)
    {
      (void)ulTaskNotifyTakeIndexed(SERIAL_RX_NOTIFY_INDEX, pdTRUE, portMAX_DELAY);
    }
  } while (read_len == 0U);

  return (int)read_len;
}

int serial_console_write(const char *data, unsigned short len)
{
  unsigned short written = 0U;

  while (written < len)
  {
    lwrb_sz_t write_len;

    taskENTER_CRITICAL();
    write_len = lwrb_write(&tx_rb, &data[written], (lwrb_sz_t)(len - written));
    written = (unsigned short)(written + write_len);

    while ((LL_USART_IsActiveFlag_TXE_TXFNF(SERIAL_USART) != 0U) &&
           (lwrb_get_full(&tx_rb) > 0U))
    {
      uint8_t byte;

      (void)lwrb_read(&tx_rb, &byte, 1U);
      LL_USART_TransmitData8(SERIAL_USART, byte);
    }

    LL_USART_EnableIT_TXE_TXFNF(SERIAL_USART);
    if ((written < len) && (lwrb_get_free(&tx_rb) == 0U))
    {
      tx_wait_task = xTaskGetCurrentTaskHandle();
    }
    taskEXIT_CRITICAL();

    if (written < len)
    {
      (void)ulTaskNotifyTakeIndexed(SERIAL_TX_NOTIFY_INDEX, pdTRUE, portMAX_DELAY);
    }
  }

  return (int)written;
}

void serial_console_irq_handler(void)
{
  BaseType_t higher_priority_task_woken = pdFALSE;
  BaseType_t rx_has_data = pdFALSE;
  BaseType_t tx_has_space = pdFALSE;

  if (LL_USART_IsActiveFlag_IDLE(SERIAL_USART) != 0U)
  {
    LL_USART_ClearFlag_IDLE(SERIAL_USART);
  }

  if ((LL_USART_IsActiveFlag_RXFT(SERIAL_USART) != 0U) ||
      (LL_USART_IsActiveFlag_RXNE_RXFNE(SERIAL_USART) != 0U))
  {
    while (LL_USART_IsActiveFlag_RXNE_RXFNE(SERIAL_USART) != 0U)
    {
      uint8_t byte = LL_USART_ReceiveData8(SERIAL_USART);
      (void)lwrb_write(&rx_rb, &byte, 1U);
      rx_has_data = pdTRUE;
    }
  }

  if ((LL_USART_IsActiveFlag_ORE(SERIAL_USART) != 0U) ||
      (LL_USART_IsActiveFlag_NE(SERIAL_USART) != 0U) ||
      (LL_USART_IsActiveFlag_FE(SERIAL_USART) != 0U))
  {
    LL_USART_ClearFlag_ORE(SERIAL_USART);
    LL_USART_ClearFlag_NE(SERIAL_USART);
    LL_USART_ClearFlag_FE(SERIAL_USART);

    while (LL_USART_IsActiveFlag_RXNE_RXFNE(SERIAL_USART) != 0U)
    {
      uint8_t byte = LL_USART_ReceiveData8(SERIAL_USART);
      (void)lwrb_write(&rx_rb, &byte, 1U);
      rx_has_data = pdTRUE;
    }
  }

  if (LL_USART_IsEnabledIT_TXE_TXFNF(SERIAL_USART) &&
      (LL_USART_IsActiveFlag_TXE_TXFNF(SERIAL_USART) != 0U))
  {
    while ((LL_USART_IsActiveFlag_TXE_TXFNF(SERIAL_USART) != 0U) &&
           (lwrb_get_full(&tx_rb) > 0U))
    {
      uint8_t byte;

      (void)lwrb_read(&tx_rb, &byte, 1U);
      LL_USART_TransmitData8(SERIAL_USART, byte);
      tx_has_space = pdTRUE;
    }

    if (lwrb_get_full(&tx_rb) == 0U)
    {
      LL_USART_DisableIT_TXE_TXFNF(SERIAL_USART);
      tx_has_space = pdTRUE;
    }
  }

  if ((rx_has_data == pdTRUE) && (rx_wait_task != NULL))
  {
    vTaskNotifyGiveFromISR(rx_wait_task, &higher_priority_task_woken);
    rx_wait_task = NULL;
  }

  if ((tx_has_space == pdTRUE) && (tx_wait_task != NULL))
  {
    vTaskNotifyGiveIndexedFromISR(tx_wait_task, SERIAL_TX_NOTIFY_INDEX, &higher_priority_task_woken);
    tx_wait_task = NULL;
  }

  portYIELD_FROM_ISR(higher_priority_task_woken);
}
