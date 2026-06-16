#include "sdmmc_test.h"
#include "sdmmc_drv.h"
#include "stm32n6xx_hal.h"
#include "shell.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

#define P(fmt, ...) shellPrint(shellGetCurrent(), fmt, ##__VA_ARGS__)

int do_rw_test(SD_HandleTypeDef *hsd, uint8_t xor_,
                uint32_t blk, const char *label)
{
    uint32_t bsz = hsd->SdCard.BlockSize ? hsd->SdCard.BlockSize : 512;
    HAL_StatusTypeDef ret;
    ALIGN_32BYTES(uint8_t tx[512]);
    ALIGN_32BYTES(uint8_t rx[512]);

    for (int i = 0; i < (int)bsz; i++) tx[i] = (uint8_t)(i ^ xor_);
    memset(rx, 0, bsz);

    ret = HAL_SD_WriteBlocks(hsd, tx, blk, 1, 5000);
    if (ret != HAL_OK) { P("[FAIL] W=%d E=0x%08lX\r\n", ret, (unsigned long)hsd->ErrorCode); return -1; }
    if (sd_wait_card_transfer(hsd, 5000) != 0) return -1;

    ret = HAL_SD_ReadBlocks(hsd, rx, blk, 1, 5000);
    if (ret != HAL_OK) { P("[FAIL] R=%d E=0x%08lX\r\n", ret, (unsigned long)hsd->ErrorCode); return -1; }
    if (sd_wait_card_transfer(hsd, 5000) != 0) return -1;

    if (memcmp(tx, rx, bsz) == 0) {
        P("[PASS] %s blk 0x%lX OK\r\n", label, (unsigned long)blk);
        return 0;
    }
    P("[FAIL] %s blk 0x%lX mismatch\r\n", label, (unsigned long)blk);
    return -1;
}

int do_rw_test_it(SD_HandleTypeDef *hsd, uint8_t xor_,
                   uint32_t blk, const char *label)
{
    uint32_t bsz = hsd->SdCard.BlockSize ? hsd->SdCard.BlockSize : 512;
    HAL_StatusTypeDef ret;
    volatile uint32_t *tx_done = sd_tx_done_flag(hsd);
    volatile uint32_t *rx_done = sd_rx_done_flag(hsd);
    ALIGN_32BYTES(uint8_t tx[512]);
    ALIGN_32BYTES(uint8_t rx[512]);

    for (int i = 0; i < (int)bsz; i++) tx[i] = (uint8_t)(i ^ xor_);
    memset(rx, 0, bsz);
    *tx_done = 0U;
    *rx_done = 0U;

    ret = HAL_SD_WriteBlocks_IT(hsd, tx, blk, 1);
    if (ret != HAL_OK) { P("[FAIL] IT W=%d E=0x%08lX\r\n", ret, (unsigned long)hsd->ErrorCode); return -1; }
    if (sd_wait_done_flag(tx_done, hsd, 5000, "IT write") != 0) return -1;
    if (sd_wait_card_transfer(hsd, 5000) != 0) return -1;

    ret = HAL_SD_ReadBlocks_IT(hsd, rx, blk, 1);
    if (ret != HAL_OK) { P("[FAIL] IT R=%d E=0x%08lX\r\n", ret, (unsigned long)hsd->ErrorCode); return -1; }
    if (sd_wait_done_flag(rx_done, hsd, 5000, "IT read") != 0) return -1;
    if (sd_wait_card_transfer(hsd, 5000) != 0) return -1;

    if (memcmp(tx, rx, bsz) == 0) {
        P("[PASS] %s IT blk 0x%lX OK\r\n", label, (unsigned long)blk);
        return 0;
    }
    P("[FAIL] %s IT blk 0x%lX mismatch\r\n", label, (unsigned long)blk);
    return -1;
}

int do_rw_test_dma(SD_HandleTypeDef *hsd, uint8_t xor_,
                    uint32_t blk, const char *label)
{
    uint32_t bsz = hsd->SdCard.BlockSize ? hsd->SdCard.BlockSize : 512;
    HAL_StatusTypeDef ret;
    volatile uint32_t *tx_done = sd_tx_done_flag(hsd);
    volatile uint32_t *rx_done = sd_rx_done_flag(hsd);
    ALIGN_32BYTES(uint8_t tx[512]);
    ALIGN_32BYTES(uint8_t rx[512]);

    for (int i = 0; i < (int)bsz; i++) tx[i] = (uint8_t)(i ^ xor_);
    memset(rx, 0, bsz);
    *tx_done = 0U;
    *rx_done = 0U;

    sd_dcache_clean(tx, bsz);
    ret = HAL_SD_WriteBlocks_DMA(hsd, tx, blk, 1);
    if (ret != HAL_OK) { P("[FAIL] DMA W=%d E=0x%08lX\r\n", ret, (unsigned long)hsd->ErrorCode); return -1; }
    if (sd_wait_done_flag(tx_done, hsd, 5000, "DMA write") != 0) return -1;
    if (sd_wait_card_transfer(hsd, 5000) != 0) return -1;

    *rx_done = 0U;
    ret = HAL_SD_ReadBlocks_DMA(hsd, rx, blk, 1);
    if (ret != HAL_OK) { P("[FAIL] DMA R=%d E=0x%08lX\r\n", ret, (unsigned long)hsd->ErrorCode); return -1; }
    if (sd_wait_done_flag(rx_done, hsd, 5000, "DMA read") != 0) return -1;
    sd_dcache_invalidate(rx, bsz);
    if (sd_wait_card_transfer(hsd, 5000) != 0) return -1;

    if (memcmp(tx, rx, bsz) == 0) {
        P("[PASS] %s DMA blk 0x%lX OK\r\n", label, (unsigned long)blk);
        return 0;
    }
    P("[FAIL] %s DMA blk 0x%lX mismatch\r\n", label, (unsigned long)blk);
    return -1;
}
