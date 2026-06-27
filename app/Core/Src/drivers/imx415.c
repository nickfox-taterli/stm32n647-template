#include "imx415.h"

#include "FreeRTOS.h"
#include "task.h"

#include "stm32n647xx.h"
#include "stm32n6xx_ll_bus.h"
#include "stm32n6xx_ll_gpio.h"
#include "stm32n6xx_ll_i2c.h"
#include "stm32n6xx_ll_rcc.h"

#define IMX415_I2C                         I2C2
#define IMX415_I2C_ADDR                    0x34U
#define IMX415_I2C_TIMING                  0x10707DBCU
#define IMX415_I2C_TIMEOUT_MS              100U

#define IMX415_RST_GPIO                    GPIOG
#define IMX415_RST_PIN                     LL_GPIO_PIN_4
#define IMX415_PWDN_GPIO                   GPIOG
#define IMX415_PWDN_PIN                    LL_GPIO_PIN_6

#define IMX415_MODE                        0x3000U
#define IMX415_MODE_OPERATING              0x00U
#define IMX415_MODE_STANDBY                0x01U
#define IMX415_XMSTA                       0x3002U
#define IMX415_XMSTA_START                 0x00U
#define IMX415_XMSTA_STOP                  0x01U
#define IMX415_VMAX                        0x3024U
#define IMX415_SHR0                        0x3050U
#define IMX415_GAIN_PCG_0                  0x3090U
#define IMX415_BLKLEVEL                    0x30E2U
#define IMX415_TPG_EN_DUOUT                0x30E4U
#define IMX415_TPG_PATSEL_DUOUT            0x30E6U
#define IMX415_TPG_COLORWIDTH              0x30E8U
#define IMX415_TESTCLKEN_MIPI              0x3110U
#define IMX415_DIG_CLP_MODE                0x32C8U
#define IMX415_WRJ_OPEN                    0x3390U
#define IMX415_SENSOR_INFO                 0x3F12U
#define IMX415_SENSOR_INFO_MASK            0x0FFFU
#define IMX415_DEMO_VMAX_LINES             2250U
#define IMX415_DEMO_EXPOSURE_LINES         1800U
#define IMX415_DEMO_ANALOG_GAIN            80U

typedef struct
{
  uint16_t reg;
  uint32_t value;
  uint8_t width;
} IMX415_Reg;

static const IMX415_Reg imx415_init_regs[] = {
  {0x301CU, 0x00U, 1U},
  {0x3022U, 0x00U, 1U},
  {0x3030U, 0x00U, 1U},
  {0x3031U, 0x00U, 1U},
  {0x3032U, 0x00U, 1U},
  {0x30C0U, 0x22U, 1U},
  {0x30C1U, 0x00U, 1U},
  {0x32D4U, 0x21U, 1U},
  {0x32ECU, 0xA1U, 1U},
  {0x3452U, 0x7FU, 1U},
  {0x3453U, 0x03U, 1U},
  {0x358AU, 0x04U, 1U},
  {0x35A1U, 0x02U, 1U},
  {0x36BCU, 0x0CU, 1U},
  {0x36CCU, 0x53U, 1U},
  {0x36CDU, 0x00U, 1U},
  {0x36CEU, 0x3CU, 1U},
  {0x36D0U, 0x8CU, 1U},
  {0x36D1U, 0x00U, 1U},
  {0x36D2U, 0x71U, 1U},
  {0x36D4U, 0x3CU, 1U},
  {0x36D6U, 0x53U, 1U},
  {0x36D7U, 0x00U, 1U},
  {0x36D8U, 0x71U, 1U},
  {0x36DAU, 0x8CU, 1U},
  {0x36DBU, 0x00U, 1U},
  {0x3724U, 0x02U, 1U},
  {0x3726U, 0x02U, 1U},
  {0x3732U, 0x02U, 1U},
  {0x3734U, 0x03U, 1U},
  {0x3736U, 0x03U, 1U},
  {0x3742U, 0x03U, 1U},
  {0x3862U, 0xE0U, 1U},
  {0x38CCU, 0x30U, 1U},
  {0x38CDU, 0x2FU, 1U},
  {0x395CU, 0x0CU, 1U},
  {0x3A42U, 0xD1U, 1U},
  {0x3A4CU, 0x77U, 1U},
  {0x3AE0U, 0x02U, 1U},
  {0x3AECU, 0x0CU, 1U},
  {0x3B00U, 0x2EU, 1U},
  {0x3B06U, 0x29U, 1U},
  {0x3B98U, 0x25U, 1U},
  {0x3B99U, 0x21U, 1U},
  {0x3B9BU, 0x13U, 1U},
  {0x3B9CU, 0x13U, 1U},
  {0x3B9DU, 0x13U, 1U},
  {0x3B9EU, 0x13U, 1U},
  {0x3BA1U, 0x00U, 1U},
  {0x3BA2U, 0x06U, 1U},
  {0x3BA3U, 0x0BU, 1U},
  {0x3BA4U, 0x10U, 1U},
  {0x3BA5U, 0x14U, 1U},
  {0x3BA6U, 0x18U, 1U},
  {0x3BA7U, 0x1AU, 1U},
  {0x3BA8U, 0x1AU, 1U},
  {0x3BA9U, 0x1AU, 1U},
  {0x3BACU, 0xEDU, 1U},
  {0x3BADU, 0x01U, 1U},
  {0x3BAEU, 0xF6U, 1U},
  {0x3BAFU, 0x02U, 1U},
  {0x3BB0U, 0xA2U, 1U},
  {0x3BB1U, 0x03U, 1U},
  {0x3BB2U, 0xE0U, 1U},
  {0x3BB3U, 0x03U, 1U},
  {0x3BB4U, 0xE0U, 1U},
  {0x3BB5U, 0x03U, 1U},
  {0x3BB6U, 0xE0U, 1U},
  {0x3BB7U, 0x03U, 1U},
  {0x3BB8U, 0xE0U, 1U},
  {0x3BBAU, 0xE0U, 1U},
  {0x3BBCU, 0xDAU, 1U},
  {0x3BBEU, 0x88U, 1U},
  {0x3BC0U, 0x44U, 1U},
  {0x3BC2U, 0x7BU, 1U},
  {0x3BC4U, 0xA2U, 1U},
  {0x3BC8U, 0xBDU, 1U},
  {0x3BCAU, 0xBDU, 1U},

  {0x3024U, 0x0008CAU, 3U}, /* VMAX */
  {0x3028U, 0x000898U, 2U}, /* HMAX: 2-lane 891 Mbps */
  {0x3050U, 0x000008U, 3U}, /* SHR0 */

  /* 891 Mbps/lane, 37.125 MHz INCK, 2-lane RAW10. */
  {0x3008U, 0x007FU, 2U},
  {0x300AU, 0x005BU, 2U},
  {0x3033U, 0x05U, 1U},
  {0x3115U, 0x00U, 1U},
  {0x3116U, 0x24U, 1U},
  {0x3118U, 0x00C0U, 2U},
  {0x311AU, 0x00E0U, 2U},
  {0x311EU, 0x24U, 1U},
  {0x400CU, 0x00U, 1U},
  {0x4074U, 0x01U, 1U},
  {0x4004U, 0x0948U, 2U},
  {0x4018U, 0x007FU, 2U},
  {0x401AU, 0x0037U, 2U},
  {0x401CU, 0x0037U, 2U},
  {0x401EU, 0x00F7U, 2U},
  {0x4020U, 0x003FU, 2U},
  {0x4022U, 0x006FU, 2U},
  {0x4024U, 0x003FU, 2U},
  {0x4026U, 0x005FU, 2U},
  {0x4028U, 0x002FU, 2U},
  {0x4001U, 0x0001U, 2U},

  {0x30E2U, 0x0032U, 2U},
  {0x30E4U, 0x00U, 1U},
  {0x3110U, 0x00U, 1U},
  {0x32C8U, 0x01U, 1U},
  {0x3390U, 0x01U, 1U},
};

static IMX415_Status IMX415_WaitFlag(uint32_t (*flag)(const I2C_TypeDef *))
{
  TickType_t start = xTaskGetTickCount();

  while (flag(IMX415_I2C) == 0U)
  {
    if ((LL_I2C_IsActiveFlag_NACK(IMX415_I2C) != 0U) ||
        (LL_I2C_IsActiveFlag_BERR(IMX415_I2C) != 0U) ||
        (LL_I2C_IsActiveFlag_ARLO(IMX415_I2C) != 0U))
    {
      return IMX415_ERROR;
    }
    if ((xTaskGetTickCount() - start) > pdMS_TO_TICKS(IMX415_I2C_TIMEOUT_MS))
    {
      return IMX415_ERROR;
    }
  }

  return IMX415_OK;
}

static void IMX415_ClearI2CFlags(void)
{
  LL_I2C_ClearFlag_NACK(IMX415_I2C);
  LL_I2C_ClearFlag_STOP(IMX415_I2C);
  LL_I2C_ClearFlag_BERR(IMX415_I2C);
  LL_I2C_ClearFlag_ARLO(IMX415_I2C);
  LL_I2C_ClearFlag_OVR(IMX415_I2C);
}

static IMX415_Status IMX415_WaitStop(void)
{
  IMX415_Status status = IMX415_WaitFlag(LL_I2C_IsActiveFlag_STOP);

  LL_I2C_ClearFlag_STOP(IMX415_I2C);
  return status;
}

static IMX415_Status IMX415_WriteBytes(uint16_t reg, const uint8_t *data, uint8_t length)
{
  if ((data == 0) || (length == 0U))
  {
    return IMX415_ERROR;
  }

  TickType_t start = xTaskGetTickCount();
  while (LL_I2C_IsActiveFlag_BUSY(IMX415_I2C) != 0U)
  {
    if ((xTaskGetTickCount() - start) > pdMS_TO_TICKS(IMX415_I2C_TIMEOUT_MS))
    {
      return IMX415_ERROR;
    }
  }

  IMX415_ClearI2CFlags();
  LL_I2C_HandleTransfer(IMX415_I2C, IMX415_I2C_ADDR, LL_I2C_ADDRSLAVE_7BIT,
                        (uint32_t)length + 2U, LL_I2C_MODE_AUTOEND,
                        LL_I2C_GENERATE_START_WRITE);

  if (IMX415_WaitFlag(LL_I2C_IsActiveFlag_TXIS) != IMX415_OK)
  {
    return IMX415_ERROR;
  }
  LL_I2C_TransmitData8(IMX415_I2C, (uint8_t)(reg >> 8));

  if (IMX415_WaitFlag(LL_I2C_IsActiveFlag_TXIS) != IMX415_OK)
  {
    return IMX415_ERROR;
  }
  LL_I2C_TransmitData8(IMX415_I2C, (uint8_t)reg);

  for (uint8_t i = 0; i < length; i++)
  {
    if (IMX415_WaitFlag(LL_I2C_IsActiveFlag_TXIS) != IMX415_OK)
    {
      return IMX415_ERROR;
    }
    LL_I2C_TransmitData8(IMX415_I2C, data[i]);
  }

  return IMX415_WaitStop();
}

static IMX415_Status IMX415_WriteLe(uint16_t reg, uint32_t value, uint8_t width)
{
  uint8_t data[3];

  if ((width == 0U) || (width > sizeof(data)))
  {
    return IMX415_ERROR;
  }

  for (uint8_t i = 0; i < width; i++)
  {
    data[i] = (uint8_t)(value >> (8U * i));
  }

  return IMX415_WriteBytes(reg, data, width);
}

static IMX415_Status IMX415_ReadBytes(uint16_t reg, uint8_t *data, uint8_t length)
{
  if ((data == 0) || (length == 0U))
  {
    return IMX415_ERROR;
  }

  IMX415_ClearI2CFlags();
  LL_I2C_HandleTransfer(IMX415_I2C, IMX415_I2C_ADDR, LL_I2C_ADDRSLAVE_7BIT,
                        2U, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);

  if (IMX415_WaitFlag(LL_I2C_IsActiveFlag_TXIS) != IMX415_OK)
  {
    return IMX415_ERROR;
  }
  LL_I2C_TransmitData8(IMX415_I2C, (uint8_t)(reg >> 8));

  if (IMX415_WaitFlag(LL_I2C_IsActiveFlag_TXIS) != IMX415_OK)
  {
    return IMX415_ERROR;
  }
  LL_I2C_TransmitData8(IMX415_I2C, (uint8_t)reg);

  if (IMX415_WaitFlag(LL_I2C_IsActiveFlag_TC) != IMX415_OK)
  {
    return IMX415_ERROR;
  }

  LL_I2C_HandleTransfer(IMX415_I2C, IMX415_I2C_ADDR, LL_I2C_ADDRSLAVE_7BIT,
                        length, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);

  for (uint8_t i = 0; i < length; i++)
  {
    if (IMX415_WaitFlag(LL_I2C_IsActiveFlag_RXNE) != IMX415_OK)
    {
      return IMX415_ERROR;
    }
    data[i] = LL_I2C_ReceiveData8(IMX415_I2C);
  }

  return IMX415_WaitStop();
}

void IMX415_BusInit(void)
{
  LL_GPIO_InitTypeDef gpio = {0};
  LL_I2C_InitTypeDef i2c = {0};

  LL_RCC_SetI2CClockSource(LL_RCC_I2C2_CLKSOURCE_CLKP);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOD);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C2);

  gpio.Pin = LL_GPIO_PIN_4 | LL_GPIO_PIN_14;
  gpio.Mode = LL_GPIO_MODE_ALTERNATE;
  gpio.Speed = LL_GPIO_SPEED_FREQ_LOW;
  gpio.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  gpio.Pull = LL_GPIO_PULL_UP;
  gpio.Alternate = LL_GPIO_AF_4;
  LL_GPIO_Init(GPIOD, &gpio);

  LL_GPIO_SetOutputPin(IMX415_PWDN_GPIO, IMX415_PWDN_PIN);
  LL_GPIO_ResetOutputPin(IMX415_RST_GPIO, IMX415_RST_PIN);
  gpio.Pin = IMX415_PWDN_PIN | IMX415_RST_PIN;
  gpio.Mode = LL_GPIO_MODE_OUTPUT;
  gpio.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(GPIOG, &gpio);

  LL_I2C_Disable(IMX415_I2C);
  i2c.PeripheralMode = LL_I2C_MODE_I2C;
  i2c.Timing = IMX415_I2C_TIMING;
  i2c.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
  i2c.DigitalFilter = 0U;
  i2c.OwnAddress1 = 0U;
  i2c.TypeAcknowledge = LL_I2C_ACK;
  i2c.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
  (void)LL_I2C_Init(IMX415_I2C, &i2c);
  LL_I2C_Enable(IMX415_I2C);

  LL_GPIO_SetOutputPin(IMX415_PWDN_GPIO, IMX415_PWDN_PIN);
  LL_GPIO_ResetOutputPin(IMX415_RST_GPIO, IMX415_RST_PIN);
  vTaskDelay(pdMS_TO_TICKS(200));
  LL_GPIO_SetOutputPin(IMX415_RST_GPIO, IMX415_RST_PIN);
  vTaskDelay(pdMS_TO_TICKS(5));
}

IMX415_Status IMX415_ReadID(uint16_t *chip_id)
{
  uint8_t data[2];

  if (chip_id == 0)
  {
    return IMX415_ERROR;
  }

  if (IMX415_WriteLe(IMX415_MODE, IMX415_MODE_OPERATING, 1U) != IMX415_OK)
  {
    return IMX415_ERROR;
  }
  vTaskDelay(pdMS_TO_TICKS(80));

  if (IMX415_ReadBytes(IMX415_SENSOR_INFO, data, sizeof(data)) != IMX415_OK)
  {
    return IMX415_ERROR;
  }
  *chip_id = (uint16_t)(((uint16_t)data[1] << 8) | data[0]);
  *chip_id &= IMX415_SENSOR_INFO_MASK;

  (void)IMX415_WriteLe(IMX415_MODE, IMX415_MODE_STANDBY, 1U);
  return IMX415_OK;
}

IMX415_Status IMX415_InitStream(void)
{
  for (uint32_t i = 0; i < (sizeof(imx415_init_regs) / sizeof(imx415_init_regs[0])); i++)
  {
    if (IMX415_WriteLe(imx415_init_regs[i].reg,
                       imx415_init_regs[i].value,
                       imx415_init_regs[i].width) != IMX415_OK)
    {
      return IMX415_ERROR;
    }
  }

  if ((IMX415_WriteLe(IMX415_VMAX, IMX415_DEMO_VMAX_LINES, 3U) != IMX415_OK) ||
      (IMX415_WriteLe(IMX415_SHR0,
                      IMX415_DEMO_VMAX_LINES - IMX415_DEMO_EXPOSURE_LINES,
                      3U) != IMX415_OK) ||
      (IMX415_WriteLe(IMX415_GAIN_PCG_0, IMX415_DEMO_ANALOG_GAIN, 2U) != IMX415_OK))
  {
    return IMX415_ERROR;
  }

  return IMX415_OK;
}

IMX415_Status IMX415_EnableTestPattern(uint8_t enable)
{
  if (enable != 0U)
  {
    if ((IMX415_WriteLe(IMX415_BLKLEVEL, 0x0000U, 2U) != IMX415_OK) ||
        (IMX415_WriteLe(IMX415_TPG_EN_DUOUT, 0x01U, 1U) != IMX415_OK) ||
        (IMX415_WriteLe(IMX415_TPG_PATSEL_DUOUT, 0x0AU, 1U) != IMX415_OK) ||  // 0x0A => 横条 0x0B => 竖条
        (IMX415_WriteLe(IMX415_TPG_COLORWIDTH, 0x01U, 1U) != IMX415_OK) ||
        (IMX415_WriteLe(IMX415_TESTCLKEN_MIPI, 0x20U, 1U) != IMX415_OK) ||
        (IMX415_WriteLe(IMX415_DIG_CLP_MODE, 0x00U, 1U) != IMX415_OK) ||
        (IMX415_WriteLe(IMX415_WRJ_OPEN, 0x00U, 1U) != IMX415_OK))
    {
      return IMX415_ERROR;
    }
  }
  else
  {
    if ((IMX415_WriteLe(IMX415_BLKLEVEL, 0x0032U, 2U) != IMX415_OK) ||
        (IMX415_WriteLe(IMX415_TPG_EN_DUOUT, 0x00U, 1U) != IMX415_OK) ||
        (IMX415_WriteLe(IMX415_TESTCLKEN_MIPI, 0x00U, 1U) != IMX415_OK) ||
        (IMX415_WriteLe(IMX415_DIG_CLP_MODE, 0x01U, 1U) != IMX415_OK) ||
        (IMX415_WriteLe(IMX415_WRJ_OPEN, 0x01U, 1U) != IMX415_OK))
    {
      return IMX415_ERROR;
    }
  }

  return IMX415_OK;
}

IMX415_Status IMX415_StartStream(void)
{
  if (IMX415_WriteLe(IMX415_MODE, IMX415_MODE_OPERATING, 1U) != IMX415_OK)
  {
    return IMX415_ERROR;
  }
  vTaskDelay(pdMS_TO_TICKS(80));

  return IMX415_WriteLe(IMX415_XMSTA, IMX415_XMSTA_START, 1U);
}

IMX415_Status IMX415_StopStream(void)
{
  if (IMX415_WriteLe(IMX415_XMSTA, IMX415_XMSTA_STOP, 1U) != IMX415_OK)
  {
    return IMX415_ERROR;
  }

  return IMX415_WriteLe(IMX415_MODE, IMX415_MODE_STANDBY, 1U);
}
