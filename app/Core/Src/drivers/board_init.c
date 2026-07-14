#include "board_init.h"
#include "stm32n647xx.h"
#include "stm32n6xx_ll_bus.h"
#include "stm32n6xx_ll_gpio.h"
#include "stm32n6xx_ll_pwr.h"

#define RIF_CID_1_VALUE              1U
#define RIF_MASTER_INDEX_SDMMC1      2U
#define RIF_MASTER_INDEX_NPU         1U
#define RIF_MASTER_INDEX_SDMMC2      3U
#define RIF_MASTER_INDEX_DMA2D       8U
#define RIF_MASTER_INDEX_DCMIPP      9U
#define RIF_MASTER_INDEX_LTDC1       10U
#define RIF_MASTER_INDEX_LTDC2       11U
#define RIF_RISC_REG_PERIPH0         0U
#define RIF_RISC_BIT_I2C2            10U
#define RIF_RISC_REG_SDMMC           1U
#define RIF_RISC_BIT_SDMMC1          21U
#define RIF_RISC_BIT_SDMMC2          22U
#define RIF_RISC_REG_CAMERA          2U
#define RIF_RISC_BIT_CSI             28U
#define RIF_RISC_BIT_DCMIPP          29U
#define RIF_RISC_REG_DISPLAY         3U
#define RIF_RISC_BIT_DMA2D           5U
#define RIF_RISC_BIT_LTDC            6U
#define RIF_RISC_BIT_LTDCL1          7U
#define RIF_RISC_BIT_LTDCL2          8U
#define RIF_RISC_REG_NPU             3U
#define RIF_RISC_BIT_NPU             10U

static void RIF_SetMasterSecurePriv(uint32_t master_id)
{
  uint32_t attr = RIFSC->RIMC_ATTRx[master_id];

  attr &= ~(RIFSC_RIMC_ATTRx_MCID | RIFSC_RIMC_ATTRx_MSEC | RIFSC_RIMC_ATTRx_MPRIV);
  attr |= (RIF_CID_1_VALUE << RIFSC_RIMC_ATTRx_MCID_Pos);
  attr |= RIFSC_RIMC_ATTRx_MSEC | RIFSC_RIMC_ATTRx_MPRIV;
  RIFSC->RIMC_ATTRx[master_id] = attr;
}

static void RIF_SetSlaveSecurePriv(uint32_t reg_index, uint32_t bit)
{
  RIFSC->RISC_SECCFGRx[reg_index] |= (1UL << bit);
  RIFSC->RISC_PRIVCFGRx[reg_index] |= (1UL << bit);
}

void SystemIsolation_Config(void)
{
  LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_RIFSC);

  LL_PWR_ConfigSecure(LL_PWR_SEC0_SEC);
  LL_PWR_ConfigPrivilege(0);

  LL_PWR_SetVddIO2VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_1V8);
  LL_PWR_EnableVddIO2();
  LL_PWR_SetVddIO4VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_3V3);
  LL_PWR_EnableVddIO4();
  LL_PWR_SetVddIO5VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_3V3);
  LL_PWR_EnableVddIO5();

  RIF_SetMasterSecurePriv(RIF_MASTER_INDEX_SDMMC1);
  RIF_SetMasterSecurePriv(RIF_MASTER_INDEX_NPU);
  RIF_SetMasterSecurePriv(RIF_MASTER_INDEX_SDMMC2);
  RIF_SetMasterSecurePriv(RIF_MASTER_INDEX_DMA2D);
  RIF_SetMasterSecurePriv(RIF_MASTER_INDEX_DCMIPP);
  RIF_SetMasterSecurePriv(RIF_MASTER_INDEX_LTDC1);
  RIF_SetMasterSecurePriv(RIF_MASTER_INDEX_LTDC2);
  RIF_SetSlaveSecurePriv(RIF_RISC_REG_PERIPH0, RIF_RISC_BIT_I2C2);
  RIF_SetSlaveSecurePriv(RIF_RISC_REG_SDMMC, RIF_RISC_BIT_SDMMC1);
  RIF_SetSlaveSecurePriv(RIF_RISC_REG_SDMMC, RIF_RISC_BIT_SDMMC2);
  RIF_SetSlaveSecurePriv(RIF_RISC_REG_CAMERA, RIF_RISC_BIT_CSI);
  RIF_SetSlaveSecurePriv(RIF_RISC_REG_CAMERA, RIF_RISC_BIT_DCMIPP);
  RIF_SetSlaveSecurePriv(RIF_RISC_REG_DISPLAY, RIF_RISC_BIT_DMA2D);
  RIF_SetSlaveSecurePriv(RIF_RISC_REG_DISPLAY, RIF_RISC_BIT_LTDC);
  RIF_SetSlaveSecurePriv(RIF_RISC_REG_DISPLAY, RIF_RISC_BIT_LTDCL1);
  RIF_SetSlaveSecurePriv(RIF_RISC_REG_DISPLAY, RIF_RISC_BIT_LTDCL2);
  RIF_SetSlaveSecurePriv(RIF_RISC_REG_NPU, RIF_RISC_BIT_NPU);

  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOA);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOC);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOD);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOF);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOG);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOH);

  LL_GPIO_EnablePinSecure(GPIOE, LL_GPIO_PIN_10);
  LL_GPIO_DisablePinPrivilege(GPIOE, LL_GPIO_PIN_10);

  LL_GPIO_EnablePinSecure(GPIOG, LL_GPIO_PIN_10);
  LL_GPIO_DisablePinPrivilege(GPIOG, LL_GPIO_PIN_10);

  LL_GPIO_EnablePinSecure(GPIOE, LL_GPIO_PIN_5 | LL_GPIO_PIN_6);
  LL_GPIO_DisablePinPrivilege(GPIOE, LL_GPIO_PIN_5 | LL_GPIO_PIN_6);

  LL_GPIO_EnablePinSecure(GPIOC, LL_GPIO_PIN_0 | LL_GPIO_PIN_3 | LL_GPIO_PIN_4 |
                                 LL_GPIO_PIN_5 | LL_GPIO_PIN_8 | LL_GPIO_PIN_9 |
                                 LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12);
  LL_GPIO_DisablePinPrivilege(GPIOC, LL_GPIO_PIN_0 | LL_GPIO_PIN_3 | LL_GPIO_PIN_4 |
                                     LL_GPIO_PIN_5 | LL_GPIO_PIN_8 | LL_GPIO_PIN_9 |
                                     LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12);
  LL_GPIO_EnablePinSecure(GPIOD, LL_GPIO_PIN_2);
  LL_GPIO_DisablePinPrivilege(GPIOD, LL_GPIO_PIN_2);
  LL_GPIO_EnablePinSecure(GPIOD, LL_GPIO_PIN_4 | LL_GPIO_PIN_14);
  LL_GPIO_DisablePinPrivilege(GPIOD, LL_GPIO_PIN_4 | LL_GPIO_PIN_14);
  LL_GPIO_EnablePinSecure(GPIOE, LL_GPIO_PIN_4);
  LL_GPIO_DisablePinPrivilege(GPIOE, LL_GPIO_PIN_4);
  LL_GPIO_EnablePinSecure(GPIOH, LL_GPIO_PIN_2);
  LL_GPIO_DisablePinPrivilege(GPIOH, LL_GPIO_PIN_2);

  LL_GPIO_EnablePinSecure(GPIOA, LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 |
                                 LL_GPIO_PIN_3 | LL_GPIO_PIN_5 | LL_GPIO_PIN_7 |
                                 LL_GPIO_PIN_8 | LL_GPIO_PIN_9 | LL_GPIO_PIN_10 |
                                 LL_GPIO_PIN_11 | LL_GPIO_PIN_12 | LL_GPIO_PIN_15);
  LL_GPIO_DisablePinPrivilege(GPIOA, LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 |
                                     LL_GPIO_PIN_3 | LL_GPIO_PIN_5 | LL_GPIO_PIN_7 |
                                     LL_GPIO_PIN_8 | LL_GPIO_PIN_9 | LL_GPIO_PIN_10 |
                                     LL_GPIO_PIN_11 | LL_GPIO_PIN_12 | LL_GPIO_PIN_15);
  LL_GPIO_EnablePinSecure(GPIOB, LL_GPIO_PIN_1 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5 |
                                 LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12 |
                                 LL_GPIO_PIN_15);
  LL_GPIO_DisablePinPrivilege(GPIOB, LL_GPIO_PIN_1 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5 |
                                     LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12 |
                                     LL_GPIO_PIN_15);
  LL_GPIO_EnablePinSecure(GPIOF, LL_GPIO_PIN_8 | LL_GPIO_PIN_9);
  LL_GPIO_DisablePinPrivilege(GPIOF, LL_GPIO_PIN_8 | LL_GPIO_PIN_9);
  LL_GPIO_EnablePinSecure(GPIOG, LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 |
                                 LL_GPIO_PIN_4 | LL_GPIO_PIN_6 | LL_GPIO_PIN_9 |
                                 LL_GPIO_PIN_12 | LL_GPIO_PIN_13 | LL_GPIO_PIN_15);
  LL_GPIO_DisablePinPrivilege(GPIOG, LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 |
                                     LL_GPIO_PIN_4 | LL_GPIO_PIN_6 | LL_GPIO_PIN_9 |
                                     LL_GPIO_PIN_12 | LL_GPIO_PIN_13 | LL_GPIO_PIN_15);
  LL_GPIO_EnablePinSecure(GPIOH, LL_GPIO_PIN_4);
  LL_GPIO_DisablePinPrivilege(GPIOH, LL_GPIO_PIN_4);
}

void MX_GPIO_Init(void)
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
