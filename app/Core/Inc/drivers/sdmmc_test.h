#ifndef SDMMC_TEST_H
#define SDMMC_TEST_H

#include "stm32n6xx_hal_sd.h"

int do_rw_test(SD_HandleTypeDef *hsd, uint8_t xor_, uint32_t blk, const char *label);
int do_rw_test_it(SD_HandleTypeDef *hsd, uint8_t xor_, uint32_t blk, const char *label);
int do_rw_test_dma(SD_HandleTypeDef *hsd, uint8_t xor_, uint32_t blk, const char *label);

#endif /* SDMMC_TEST_H */
