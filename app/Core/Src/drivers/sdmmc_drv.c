#include "sdmmc_drv.h"
#include "stm32n6xx_hal.h"
#include "stm32n6xx_ll_bus.h"
#include "stm32n6xx_ll_gpio.h"
#include "stm32n6xx_ll_rcc.h"
#include "stm32n6xx_ll_pwr.h"
#include "stm32n6xx_ll_utils.h"
#include "shell.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

#define P(fmt, ...) shellPrint(shellGetCurrent(), fmt, ##__VA_ARGS__)

/* ================================================================
 *  Module state
 * ================================================================ */

static SD_HandleTypeDef hsd1;
static SD_HandleTypeDef hsd2;
static volatile uint32_t sd1_tx_done;
static volatile uint32_t sd1_rx_done;
static volatile uint32_t sd2_tx_done;
static volatile uint32_t sd2_rx_done;

static int sd1_ready;
static int sd2_ready;

/* ================================================================
 *  RIF configuration
 * ================================================================ */

#define RIF_CID_1_VALUE              1U
#define RIF_MASTER_INDEX_SDMMC1      2U
#define RIF_MASTER_INDEX_SDMMC2      3U
#define RIF_RISC_REG_SDMMC           1U
#define RIF_RISC_BIT_SDMMC1          21U
#define RIF_RISC_BIT_SDMMC2          22U

static void sdmmc_rif_config(uint32_t master_id, uint32_t slave_bit)
{
    uint32_t attr;

    LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_RIFSC);
    attr = RIFSC->RIMC_ATTRx[master_id];
    attr &= ~(RIFSC_RIMC_ATTRx_MCID | RIFSC_RIMC_ATTRx_MSEC | RIFSC_RIMC_ATTRx_MPRIV);
    attr |= (RIF_CID_1_VALUE << RIFSC_RIMC_ATTRx_MCID_Pos);
    attr |= RIFSC_RIMC_ATTRx_MSEC | RIFSC_RIMC_ATTRx_MPRIV;
    RIFSC->RIMC_ATTRx[master_id] = attr;

    RIFSC->RISC_SECCFGRx[RIF_RISC_REG_SDMMC] |= (1UL << slave_bit);
    RIFSC->RISC_PRIVCFGRx[RIF_RISC_REG_SDMMC] |= (1UL << slave_bit);
}

/* ================================================================
 *  MSP init (LL-based)
 * ================================================================ */

static void sdmmc1_ll_msp_init(void)
{
    LL_PWR_SetVddIO4VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_3V3);
    LL_PWR_EnableVddIO4();
    sdmmc_rif_config(RIF_MASTER_INDEX_SDMMC1, RIF_RISC_BIT_SDMMC1);

    LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOC);
    LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOH);

    LL_AHB5_GRP1_EnableClock(LL_AHB5_GRP1_PERIPH_SDMMC1);
    LL_AHB5_GRP1_ForceReset(LL_AHB5_GRP1_PERIPH_SDMMC1);
    LL_AHB5_GRP1_ReleaseReset(LL_AHB5_GRP1_PERIPH_SDMMC1);

    LL_MISC_ForceReset(LL_SDMMC1DLL);
    LL_MISC_ReleaseReset(LL_SDMMC1DLL);

    LL_RCC_SetSDMMCClockSource(LL_RCC_SDMMC1_CLKSOURCE_HCLK);

    P("[MSP1] ENR=%08lX PWR=%08lX CLK=%08lX\r\n",
      (unsigned long)RCC->AHB5ENR,
      (unsigned long)SDMMC1->POWER,
      (unsigned long)SDMMC1->CLKCR);

    LL_GPIO_InitTypeDef gpio = {0};
    gpio.Mode       = LL_GPIO_MODE_ALTERNATE;
    gpio.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio.Pull       = LL_GPIO_PULL_NO;
    gpio.Alternate  = LL_GPIO_AF_10;

    gpio.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9 | LL_GPIO_PIN_10 | LL_GPIO_PIN_11;
    LL_GPIO_Init(GPIOC, &gpio);

    gpio.Pin  = LL_GPIO_PIN_12;
    LL_GPIO_Init(GPIOC, &gpio);

    gpio.Pin  = LL_GPIO_PIN_2;
    LL_GPIO_Init(GPIOH, &gpio);

    LL_GPIO_EnablePinSecure(GPIOC, LL_GPIO_PIN_8 | LL_GPIO_PIN_9 | LL_GPIO_PIN_10
                                  | LL_GPIO_PIN_11 | LL_GPIO_PIN_12);
    LL_GPIO_DisablePinPrivilege(GPIOC, LL_GPIO_PIN_8 | LL_GPIO_PIN_9 | LL_GPIO_PIN_10
                                       | LL_GPIO_PIN_11 | LL_GPIO_PIN_12);
    LL_GPIO_EnablePinSecure(GPIOH, LL_GPIO_PIN_2);
    LL_GPIO_DisablePinPrivilege(GPIOH, LL_GPIO_PIN_2);

    NVIC_SetPriority(SDMMC1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 6U, 0U));
    NVIC_EnableIRQ(SDMMC1_IRQn);
}

static void sdmmc2_ll_msp_init(void)
{
    LL_PWR_SetVddIO4VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_3V3);
    LL_PWR_EnableVddIO4();
    LL_PWR_SetVddIO5VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_3V3);
    LL_PWR_EnableVddIO5();
    sdmmc_rif_config(RIF_MASTER_INDEX_SDMMC2, RIF_RISC_BIT_SDMMC2);

    LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOC);
    LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOD);
    LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOE);

    LL_AHB5_GRP1_EnableClock(LL_AHB5_GRP1_PERIPH_SDMMC2);
    LL_AHB5_GRP1_ForceReset(LL_AHB5_GRP1_PERIPH_SDMMC2);
    LL_AHB5_GRP1_ReleaseReset(LL_AHB5_GRP1_PERIPH_SDMMC2);

    LL_MISC_ForceReset(LL_SDMMC2DLL);
    LL_MISC_ReleaseReset(LL_SDMMC2DLL);

    LL_RCC_SetSDMMCClockSource(LL_RCC_SDMMC2_CLKSOURCE_HCLK);

    P("[MSP2] ENR=%08lX PWR=%08lX CLK=%08lX\r\n",
      (unsigned long)RCC->AHB5ENR,
      (unsigned long)SDMMC2->POWER,
      (unsigned long)SDMMC2->CLKCR);

    LL_GPIO_InitTypeDef gpio = {0};
    gpio.Mode       = LL_GPIO_MODE_ALTERNATE;
    gpio.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio.Pull       = LL_GPIO_PULL_NO;
    gpio.Alternate  = LL_GPIO_AF_11;

    gpio.Pin = LL_GPIO_PIN_3 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5;
    LL_GPIO_Init(GPIOC, &gpio);

    gpio.Pin = LL_GPIO_PIN_0;
    LL_GPIO_Init(GPIOC, &gpio);

    gpio.Pin  = LL_GPIO_PIN_2;
    LL_GPIO_Init(GPIOD, &gpio);

    gpio.Pin  = LL_GPIO_PIN_4;
    LL_GPIO_Init(GPIOE, &gpio);

    LL_GPIO_EnablePinSecure(GPIOC, LL_GPIO_PIN_0 | LL_GPIO_PIN_3
                                  | LL_GPIO_PIN_4 | LL_GPIO_PIN_5);
    LL_GPIO_DisablePinPrivilege(GPIOC, LL_GPIO_PIN_0 | LL_GPIO_PIN_3
                                       | LL_GPIO_PIN_4 | LL_GPIO_PIN_5);
    LL_GPIO_EnablePinSecure(GPIOD, LL_GPIO_PIN_2);
    LL_GPIO_DisablePinPrivilege(GPIOD, LL_GPIO_PIN_2);
    LL_GPIO_EnablePinSecure(GPIOE, LL_GPIO_PIN_4);
    LL_GPIO_DisablePinPrivilege(GPIOE, LL_GPIO_PIN_4);

    NVIC_SetPriority(SDMMC2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 6U, 0U));
    NVIC_EnableIRQ(SDMMC2_IRQn);
}

/* ================================================================
 *  HAL_SD_MspInit / MspDeInit
 * ================================================================ */

void HAL_SD_MspInit(SD_HandleTypeDef *hsd)
{
    if (hsd->Instance == SDMMC1) {
        sdmmc1_ll_msp_init();
    } else if (hsd->Instance == SDMMC2) {
        sdmmc2_ll_msp_init();
    }
}

void HAL_SD_MspDeInit(SD_HandleTypeDef *hsd)
{
    if (hsd->Instance == SDMMC1)
    {
        NVIC_DisableIRQ(SDMMC1_IRQn);
        LL_AHB5_GRP1_DisableClock(LL_AHB5_GRP1_PERIPH_SDMMC1);
    }
    else if (hsd->Instance == SDMMC2)
    {
        NVIC_DisableIRQ(SDMMC2_IRQn);
        LL_AHB5_GRP1_DisableClock(LL_AHB5_GRP1_PERIPH_SDMMC2);
    }
}

/* ================================================================
 *  ISR + Callbacks
 * ================================================================ */

void SDMMC1_IRQHandler(void)
{
    HAL_SD_IRQHandler(&hsd1);
}

void SDMMC2_IRQHandler(void)
{
    HAL_SD_IRQHandler(&hsd2);
}

void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
    if (hsd->Instance == SDMMC1) sd1_tx_done = 1U;
    else                         sd2_tx_done = 1U;
}

void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
    if (hsd->Instance == SDMMC1) sd1_rx_done = 1U;
    else                         sd2_rx_done = 1U;
}

/* ================================================================
 *  Internal helpers (exposed for test/cmd modules)
 * ================================================================ */

int sd_wait_card_transfer(SD_HandleTypeDef *hsd, uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();
    HAL_SD_CardStateTypeDef st;
    while ((HAL_GetTick() - t0) < timeout_ms) {
        st = HAL_SD_GetCardState(hsd);
        if (st == HAL_SD_CARD_TRANSFER) return 0;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    P("[ERR] not TRANSFER, state=0x%08lX\r\n", (unsigned long)HAL_SD_GetCardState(hsd));
    return -1;
}

volatile uint32_t *sd_tx_done_flag(SD_HandleTypeDef *hsd)
{
    return (hsd->Instance == SDMMC1) ? &sd1_tx_done : &sd2_tx_done;
}

volatile uint32_t *sd_rx_done_flag(SD_HandleTypeDef *hsd)
{
    return (hsd->Instance == SDMMC1) ? &sd1_rx_done : &sd2_rx_done;
}

int sd_wait_done_flag(volatile uint32_t *flag, SD_HandleTypeDef *hsd,
                      uint32_t timeout_ms, const char *op)
{
    uint32_t t0 = HAL_GetTick();

    while ((HAL_GetTick() - t0) < timeout_ms) {
        if (*flag != 0U) return 0;
        if (hsd->ErrorCode != HAL_SD_ERROR_NONE) break;
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    P("[FAIL] %s timeout/done=0 Err=0x%08lX State=%lu STA=%08lX\r\n",
      op, (unsigned long)hsd->ErrorCode, (unsigned long)hsd->State,
      (unsigned long)hsd->Instance->STA);
    return -1;
}

void sd_dcache_clean(void *addr, uint32_t size)
{
    SCB_CleanDCache_by_Addr((uint32_t *)addr, (int32_t)size);
}

void sd_dcache_invalidate(void *addr, uint32_t size)
{
    SCB_InvalidateDCache_by_Addr((uint32_t *)addr, (int32_t)size);
}

/* ================================================================
 *  Public helpers
 * ================================================================ */

int sd_is_ready(int dev)
{
    return (dev == 1) ? sd1_ready : (dev == 2) ? sd2_ready : 0;
}

SD_HandleTypeDef *sd_get_handle(int dev)
{
    if (dev == 1 && sd1_ready) return &hsd1;
    if (dev == 2 && sd2_ready) return &hsd2;
    return NULL;
}

/* ================================================================
 *  Card init + info
 * ================================================================ */

HAL_StatusTypeDef sd_init_card(SD_HandleTypeDef *hsd, SDMMC_TypeDef *instance,
                               SDMMC_TypeDef *diag_instance, const char *fail_label)
{
    HAL_StatusTypeDef ret;

    memset(hsd, 0, sizeof(*hsd));
    hsd->Instance                 = instance;
    hsd->Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
    hsd->Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    hsd->Init.BusWide             = SDMMC_BUS_WIDE_4B;
    hsd->Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
    hsd->Init.ClockDiv            = 4;

    ret = HAL_SD_Init(hsd);

    if (ret != HAL_OK) {
        P("[%s] Init fail: ret=%d Err=0x%08lX PWR=%08lX CLKCR=%08lX STA=%08lX\r\n",
          fail_label, ret, (unsigned long)hsd->ErrorCode,
          (unsigned long)diag_instance->POWER,
          (unsigned long)diag_instance->CLKCR,
          (unsigned long)diag_instance->STA);
    }
    return ret;
}

void sd_print_info(SD_HandleTypeDef *hsd, int dev)
{
    HAL_SD_CardCIDTypeDef cid;
    const char *name;

    if (dev == 1) name = "SDMMC1 (SD Card)";
    else          name = "SDMMC2 (SD NAND)";

    P("Device:        %s\r\n", name);
    P("Card Type:     %s\r\n",
      (hsd->SdCard.CardType == CARD_SDHC_SDXC) ? "SDHC/SDXC" :
      (hsd->SdCard.CardType == CARD_SDSC) ? "SDSC" : "Unknown");
    P("Card Version:  %s\r\n",
      (hsd->SdCard.CardVersion == CARD_V2_X) ? "2.0" :
      (hsd->SdCard.CardVersion == CARD_V1_X) ? "1.0" : "Unknown");
    P("Class:         0x%lX\r\n", (unsigned long)hsd->SdCard.Class);
    P("RCA:           0x%04lX\r\n", (unsigned long)hsd->SdCard.RelCardAdd);
    P("Block Size:    %lu bytes\r\n", (unsigned long)hsd->SdCard.BlockSize);
    P("Block Count:   %lu\r\n", (unsigned long)hsd->SdCard.BlockNbr);
    P("Capacity:      %lu MB\r\n",
      (unsigned long)((uint64_t)hsd->SdCard.BlockNbr * hsd->SdCard.BlockSize / (1024 * 1024)));
    P("Bus Width:     %s\r\n",
      (hsd->Init.BusWide == SDMMC_BUS_WIDE_4B) ? "4-bit" :
      (hsd->Init.BusWide == SDMMC_BUS_WIDE_1B) ? "1-bit" : "?");
    P("Speed:         %s\r\n",
      (hsd->SdCard.CardSpeed == CARD_ULTRA_HIGH_SPEED) ? "UHS" :
      (hsd->SdCard.CardSpeed == CARD_HIGH_SPEED) ? "High Speed (50MHz)" :
      (hsd->SdCard.CardSpeed == CARD_NORMAL_SPEED) ? "Normal (25MHz)" : "Unknown");

    if (HAL_SD_GetCardCID(hsd, &cid) == HAL_OK) {
        P("Manufacturer:  0x%02X\r\n", cid.ManufacturerID);
        P("OEM:           0x%04X\r\n", cid.OEM_AppliID);
        P("Product Name:  %c%c%c%c%c\r\n",
          (char)((cid.ProdName1 >> 24) & 0xFF),
          (char)((cid.ProdName1 >> 16) & 0xFF),
          (char)((cid.ProdName1 >> 8) & 0xFF),
          (char)(cid.ProdName1 & 0xFF),
          (char)(cid.ProdName2));
        P("Product Rev:   %d.%d\r\n", (cid.ProdRev >> 4) & 0xF, cid.ProdRev & 0xF);
        P("Serial:        0x%08lX\r\n", (unsigned long)cid.ProdSN);
        P("Manuf. Date:   %d/%d\r\n",
          (cid.ManufactDate >> 4) & 0xFF,
          2000 + (cid.ManufactDate & 0xFF));
    }

    P("CSD:           %08lX %08lX %08lX %08lX\r\n",
      (unsigned long)hsd->CSD[0], (unsigned long)hsd->CSD[1],
      (unsigned long)hsd->CSD[2], (unsigned long)hsd->CSD[3]);
    P("CID:           %08lX %08lX %08lX %08lX\r\n",
      (unsigned long)hsd->CID[0], (unsigned long)hsd->CID[1],
      (unsigned long)hsd->CID[2], (unsigned long)hsd->CID[3]);
}

/* ================================================================
 *  Boot auto-initialization task
 * ================================================================ */

void SD_InitTask(void *argument)
{
    (void)argument;

    P("\r\n[SD] Initializing SDMMC1 (SD Card)...\r\n");
    if (sd_init_card(&hsd1, SDMMC1, SDMMC1, "SD1") == HAL_OK) {
        if (sd_wait_card_transfer(&hsd1, 5000) == 0) {
            sd1_ready = 1;
            sd_print_info(&hsd1, 1);
            P("[SD] SDMMC1 ready\r\n\r\n");
        } else {
            P("[SD] SDMMC1 card not in TRANSFER state\r\n\r\n");
        }
    } else {
        P("[SD] SDMMC1 init failed\r\n\r\n");
    }

    P("[SD] Initializing SDMMC2 (SD NAND)...\r\n");
    if (sd_init_card(&hsd2, SDMMC2, SDMMC2, "SD2") == HAL_OK) {
        if (sd_wait_card_transfer(&hsd2, 5000) == 0) {
            sd2_ready = 1;
            sd_print_info(&hsd2, 2);
            P("[SD] SDMMC2 ready\r\n\r\n");
        } else {
            P("[SD] SDMMC2 card not in TRANSFER state\r\n\r\n");
        }
    } else {
        P("[SD] SDMMC2 init failed\r\n\r\n");
    }

    vTaskDelete(NULL);
}
