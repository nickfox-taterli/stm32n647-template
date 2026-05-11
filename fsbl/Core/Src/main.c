/* Includes ------------------------------------------------------------------*/
#include "stm32n647xx.h"

#include "stm32n6xx_ll_pwr.h"
#include "stm32n6xx_ll_rcc.h"
#include "stm32n6xx_ll_bus.h"
#include "stm32n6xx_ll_system.h"
#include "stm32n6xx_ll_cortex.h"
#include "stm32n6xx_ll_utils.h"
#include "stm32n6xx_ll_gpio.h"
#include "stm32n6xx_ll_xspi.h"

#include "stm32n6xx_it.h"

#include "hyperram.h"
#include "norflash.h"

#include "boot/stm32_boot_xip.h"

/* Private variables ---------------------------------------------------------*/
NORFlash_ObjectTypeDef NORFlashObject = {0};
static HyperRAM_ObjectTypeDef HyperRAMObject = {0};

/* Private function prototypes -----------------------------------------------*/
void Clock_Init(void);

static void MX_GPIO_Init(void);
static void MX_XSPI1_Init(void);
static void MX_XSPI2_Init(void);

/**
  * @brief  The application entry point.
  */
int main(void)
{
  /* Enable the CPU Cache */
  SCB_EnableICache();
  SCB_EnableDCache();

  /* MCU Configuration */
  NVIC_SetPriorityGrouping(3);
  SystemCoreClockUpdate();

  /* Configure the system clock */
  Clock_Init();

  LL_Init1msTick(SystemCoreClock);

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_XSPI1_Init();
  MX_XSPI2_Init();

  /* Initialize HyperRAM on XSPI1 */
  HyperRAM_Init(XSPI1, &HyperRAMObject);
  HyperRAM_EnableMemoryMappedMode(XSPI1, &HyperRAMObject);

  /* Initialize NOR Flash on XSPI2 */
  NORFlash_Init(XSPI2, &NORFlashObject,
                LL_XSPI_SINGLE_MEM, LL_XSPI_SAMPLE_SHIFT_NONE, LL_XSPI_MEMTYPE_MACRONIX,
                SystemCoreClock / 2U);

  /* Launch the application via XIP */
  if (BOOT_OK != BOOT_Application())
  {
    while(1);
  }

  while(1)
  {
  }
}

void Clock_Init(void)
{
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_PWR);
  LL_PWR_ConfigSupply(LL_PWR_SMPS_SUPPLY);
  while (LL_PWR_IsActiveFlag_ACTVOSRDY() == 0U) {}

  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  while(LL_PWR_IsActiveFlag_VOSRDY() == 0) {}

  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() == 0) {}

  if ((LL_RCC_GetCpuClkSource() == LL_RCC_CPU_CLKSOURCE_STATUS_IC1) ||
     (LL_RCC_GetSysClkSource() == LL_RCC_SYS_CLKSOURCE_STATUS_IC2_IC6_IC11))
  {
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {}
    LL_RCC_SetCpuClkSource(LL_RCC_CPU_CLKSOURCE_HSI);
    while(LL_RCC_GetCpuClkSource() != LL_RCC_CPU_CLKSOURCE_STATUS_HSI) {}
  }

  LL_RCC_PLL1_Disable();
  while(LL_RCC_PLL1_IsReady() == 1) {}
  LL_RCC_PLL1_DisableModulationSpreadSpectrum();
  LL_RCC_PLL1_DisableBypass();
  LL_RCC_PLL1_SetSource(LL_RCC_PLLSOURCE_HSI);
  LL_RCC_PLL1_SetM(4);
  LL_RCC_PLL1_SetN(75);
  LL_RCC_PLL1_SetP1(1);
  LL_RCC_PLL1_SetP2(1);
  LL_RCC_PLL1_SetFRACN(0);
  LL_RCC_PLL1_AssertModulationSpreadSpectrumReset();
  LL_RCC_PLL1_DisableFractionalModulationSpreadSpectrum();
  LL_RCC_PLL1P_Enable();
  LL_RCC_PLL1_Enable();
  while(LL_RCC_PLL1_IsReady() != 1) {}

  LL_RCC_SetAHBPrescaler(LL_RCC_AHB_DIV_2);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetAPB4Prescaler(LL_RCC_APB4_DIV_1);
  LL_RCC_SetAPB5Prescaler(LL_RCC_APB5_DIV_1);

  LL_RCC_IC1_SetSource(LL_RCC_ICCLKSOURCE_PLL1);
  LL_RCC_IC1_SetDivider(2);
  LL_RCC_IC1_Enable();
  LL_RCC_SetCpuClkSource(LL_RCC_CPU_CLKSOURCE_IC1);
  while(LL_RCC_GetCpuClkSource() != LL_RCC_CPU_CLKSOURCE_STATUS_IC1) {}

  LL_RCC_IC2_SetSource(LL_RCC_ICCLKSOURCE_PLL1);
  LL_RCC_IC2_SetDivider(3);
  LL_RCC_IC6_SetSource(LL_RCC_ICCLKSOURCE_PLL1);
  LL_RCC_IC6_SetDivider(4);
  LL_RCC_IC11_SetSource(LL_RCC_ICCLKSOURCE_PLL1);
  LL_RCC_IC11_SetDivider(3);
  LL_RCC_IC2_Enable();
  LL_RCC_IC6_Enable();
  LL_RCC_IC11_Enable();
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_IC2_IC6_IC11);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_IC2_IC6_IC11) {}
  LL_SetSystemCoreClock(600000000);
}

static void MX_XSPI1_Init(void)
{
  LL_XSPI_InitTypeDef ll_xspi1_init;
  ll_xspi1_init.FifoThresholdByte       = 4;
  ll_xspi1_init.MemoryMode              = LL_XSPI_SINGLE_MEM;
  ll_xspi1_init.MemoryType              = LL_XSPI_MEMTYPE_HYPERBUS;
  ll_xspi1_init.MemorySize              = LL_XSPI_SIZE_256MB;
  ll_xspi1_init.ChipSelectHighTimeCycle = 2;
  ll_xspi1_init.FreeRunningClock        = LL_XSPI_FREERUNCLK_DISABLE;
  ll_xspi1_init.ClockMode               = LL_XSPI_CLOCK_MODE_0;
  ll_xspi1_init.WrapSize                = LL_XSPI_WRAP_32_BYTES;
  ll_xspi1_init.ClockPrescaler          = 0;
  ll_xspi1_init.SampleShifting          = LL_XSPI_SAMPLE_SHIFT_NONE;
  ll_xspi1_init.ChipSelectBoundary      = LL_XSPI_CSBOUND_NONE;
  ll_xspi1_init.MaxTran                 = 0;
  ll_xspi1_init.Refresh                 = 0;
  ll_xspi1_init.MemorySelect            = LL_XSPI_CSSEL_NCS1;
  ll_xspi1_init.MemoryExtended          = LL_XSPI_MEMEXT_SW;

  if (LL_XSPI_Init(XSPI1, &ll_xspi1_init) != SUCCESS)
  {
    while(1);
  }

  LL_XSPI_HyperbusCfgTypeDef ll_hbcfg = {0};
  ll_hbcfg.RWRecoveryTimeCycle = 7;
  ll_hbcfg.AccessTimeCycle = 7;
  ll_hbcfg.WriteZeroLatency = LL_XSPI_LATENCY_ON_WRITE;
  ll_hbcfg.LatencyMode = LL_XSPI_FIXED_LATENCY;
  LL_XSPI_HyperbusCfg(XSPI1, &ll_hbcfg);
}

static void MX_XSPI2_Init(void)
{
  LL_XSPI_InitTypeDef ll_xspi2_init;
  ll_xspi2_init.FifoThresholdByte       = 4;
  ll_xspi2_init.MemoryMode              = LL_XSPI_SINGLE_MEM;
  ll_xspi2_init.MemoryType              = LL_XSPI_MEMTYPE_MACRONIX;
  ll_xspi2_init.MemorySize              = LL_XSPI_SIZE_256MB;
  ll_xspi2_init.ChipSelectHighTimeCycle = 1;
  ll_xspi2_init.FreeRunningClock        = LL_XSPI_FREERUNCLK_DISABLE;
  ll_xspi2_init.ClockMode               = LL_XSPI_CLOCK_MODE_0;
  ll_xspi2_init.WrapSize                = LL_XSPI_WRAP_NOT_SUPPORTED;
  ll_xspi2_init.ClockPrescaler          = 0;
  ll_xspi2_init.SampleShifting          = LL_XSPI_SAMPLE_SHIFT_NONE;
  ll_xspi2_init.ChipSelectBoundary      = LL_XSPI_CSBOUND_NONE;
  ll_xspi2_init.MaxTran                 = 0;
  ll_xspi2_init.Refresh                 = 0;
  ll_xspi2_init.MemorySelect            = LL_XSPI_CSSEL_NCS1;
  ll_xspi2_init.MemoryExtended          = LL_XSPI_MEMEXT_SW;

  if (LL_XSPI_Init(XSPI2, &ll_xspi2_init) != SUCCESS)
  {
    while(1);
  }
}

static void MX_GPIO_Init(void)
{
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOP);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOO);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPION);
  
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Enable VDDIO2 and VDDIO3 supplies (required for GPIO P/O/N) */
  LL_PWR_EnableVddIO2();
  LL_PWR_SetVddIO2VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_1V8);
  LL_PWR_EnableVddIO3();
  LL_PWR_SetVddIO3VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_1V8);
  LL_mDelay(1);

  /* --- XSPI1 (HyperRAM) clock and GPIO --- */
  LL_RCC_SetXSPIClockSource(LL_RCC_XSPI1_CLKSOURCE_HCLK);
  LL_AHB5_GRP1_EnableClock(LL_AHB5_GRP1_PERIPH_XSPIM);
  LL_AHB5_GRP1_EnableClock(LL_AHB5_GRP1_PERIPH_XSPI1);

  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOP);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOO);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_7|LL_GPIO_PIN_6|LL_GPIO_PIN_0|LL_GPIO_PIN_4
                        |LL_GPIO_PIN_1|LL_GPIO_PIN_5|LL_GPIO_PIN_3|LL_GPIO_PIN_2;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_9;
  LL_GPIO_Init(GPIOP, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_5|LL_GPIO_PIN_2|LL_GPIO_PIN_0|LL_GPIO_PIN_4;
  LL_GPIO_Init(GPIOO, &GPIO_InitStruct);

  /* --- XSPI2 (Macronix Flash) clock and GPIO --- */
  LL_RCC_SetXSPIClockSource(LL_RCC_XSPI2_CLKSOURCE_HCLK);
  LL_AHB5_GRP1_EnableClock(LL_AHB5_GRP1_PERIPH_XSPI2);

  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPION);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_4|LL_GPIO_PIN_6|LL_GPIO_PIN_8|LL_GPIO_PIN_0
                        |LL_GPIO_PIN_3|LL_GPIO_PIN_5|LL_GPIO_PIN_1|LL_GPIO_PIN_9
                        |LL_GPIO_PIN_2|LL_GPIO_PIN_10|LL_GPIO_PIN_11;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_9;
  LL_GPIO_Init(GPION, &GPIO_InitStruct);

  /* --- XSPIM IO Manager --- */
  LL_XSPIM_SetReq2AckTime(0);
  LL_XSPIM_SetCSOverride_XSPI1(LL_XSPIM_CSSEL_OVR_NCS1);
  LL_XSPIM_SetCSOverride_XSPI2(LL_XSPIM_CSSEL_OVR_NCS1);
}
