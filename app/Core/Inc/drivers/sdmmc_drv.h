#ifndef SDMMC_DRV_H
#define SDMMC_DRV_H

#include "stm32n6xx_hal_sd.h"

SD_HandleTypeDef *sd_get_handle(int dev);
int  sd_is_ready(int dev);
int  sd_wait_card_transfer(SD_HandleTypeDef *hsd, uint32_t timeout_ms);
int  sd_wait_done_flag(volatile uint32_t *flag, SD_HandleTypeDef *hsd,
                       uint32_t timeout_ms, const char *op);
volatile uint32_t *sd_tx_done_flag(SD_HandleTypeDef *hsd);
volatile uint32_t *sd_rx_done_flag(SD_HandleTypeDef *hsd);
void sd_dcache_clean(void *addr, uint32_t size);
void sd_dcache_invalidate(void *addr, uint32_t size);
HAL_StatusTypeDef sd_init_card(SD_HandleTypeDef *hsd, SDMMC_TypeDef *instance,
                               SDMMC_TypeDef *diag_instance, const char *fail_label);
void sd_print_info(SD_HandleTypeDef *hsd, int dev);
void SD_InitTask(void *argument);

#endif /* SDMMC_DRV_H */
