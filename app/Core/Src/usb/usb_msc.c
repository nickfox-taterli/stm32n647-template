#include "tusb.h"
#include "usb_descriptors.h"
#include "sdmmc_drv.h"
#include "stm32n6xx_ll_bus.h"
#include "stm32n6xx_ll_pwr.h"
#include "stm32n6xx_ll_rcc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "serial_console.h"
#include <string.h>

#define USB_LOG(msg) ((void)serial_console_write((msg), (unsigned short)(sizeof(msg) - 1U)))

static uint8_t msc_sector[512] __attribute__((aligned(32)));

static bool usb_otg1_board_init(void)
{
  TickType_t hse_start;

  /* Match ATK's verified USB1 OTG HS clock/PHY sequence.  The PHY input is
   * HSE/2 (24 MHz); the DWC2 core reference must come from OTGPHY1, not from
   * the raw HSE/2 oscillator bypass path. */
  LL_RCC_HSE_Enable();
  hse_start = xTaskGetTickCount();
  while (LL_RCC_HSE_IsReady() == 0U) {
    if ((xTaskGetTickCount() - hse_start) >= pdMS_TO_TICKS(100U)) {
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(1U));
  }

  LL_PWR_EnableVddUSB();
  LL_RCC_HSE_SelectHSEAsDiv2Clock();
  LL_RCC_SetOTGPHYClockSource(LL_RCC_OTGPHY1_CLKSOURCE_HSE_DIV_2);
  LL_RCC_SetOTGPHYCKREFClockSource(LL_RCC_OTGPHY1CKREF_CLKSOURCE_OTGPHY1);

  LL_AHB5_GRP1_EnableClock(LL_AHB5_GRP1_PERIPH_OTG1);
  vTaskDelay(pdMS_TO_TICKS(1));

  USB1_HS_PHYC->USBPHYC_CR &= ~USB_USBPHYC_CR_FSEL;
  USB1_HS_PHYC->USBPHYC_CR |= USB_USBPHYC_CR_OTGDISABLE0 |
                              USB_USBPHYC_CR_FSEL_1 |
                              USB_USBPHYC_CR_CMN |
                              USB_USBPHYC_CR_RETENABLEN1;
  LL_AHB5_GRP1_EnableClock(LL_AHB5_GRP1_PERIPH_OTGPHY1);

  NVIC_SetPriority(USB1_OTG_HS_IRQn,
                   NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 7U, 0U));
  return true;
}

void USBDeviceTask(void *argument)
{
  (void)argument;

  USB_LOG("[USB] task start\r\n");
  if (!usb_otg1_board_init()) {
    USB_LOG("[USB] HSE start failed\r\n");
    vTaskDelete(NULL);
  }
  USB_LOG("[USB] OTG1 PHY ready\r\n");
  tusb_rhport_init_t init = {
    .role = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO,
  };
  if (!tusb_init(BOARD_TUD_RHPORT, &init)) {
    USB_LOG("[USB] TinyUSB init failed\r\n");
    vTaskDelete(NULL);
  }
  USB_LOG("[USB] OTG1 MSC ready (SDMMC2 shared block device)\r\n");

  while (1) {
    tud_task();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
  (void)lun;
  if (!sd_is_ready(2)) {
    (void)tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x04, 0x01);
    return false;
  }
  return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size)
{
  (void)lun;
  SD_HandleTypeDef *hsd = sd_get_handle(2);
  *block_count = hsd ? hsd->SdCard.BlockNbr : 0U;
  *block_size = 512U;
}

uint32_t tud_msc_inquiry2_cb(uint8_t lun, scsi_inquiry_resp_t *resp, uint32_t bufsize)
{
  (void)lun;
  (void)bufsize;
  memcpy(resp->vendor_id, "STMicro ", 8);
  memcpy(resp->product_id, "STM32N6 SD NAND", 15);
  memcpy(resp->product_rev, "1.0 ", 4);
  return sizeof(scsi_inquiry_resp_t);
}

bool tud_msc_is_writable_cb(uint8_t lun)
{
  (void)lun;
  return true;
}

static int msc_check_range(uint32_t lba, uint32_t offset, uint32_t size)
{
  SD_HandleTypeDef *hsd = sd_get_handle(2);
  if (!hsd || lba >= hsd->SdCard.BlockNbr || offset > 512U || size > (512U - offset)) return -1;
  return 0;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                          void *buffer, uint32_t bufsize)
{
  (void)lun;
  if (msc_check_range(lba, offset, bufsize) != 0 || sd_storage_lock(5000U) != 0) {
    return TUD_MSC_RET_ERROR;
  }
  HAL_StatusTypeDef ret = sd_storage_read_locked(lba, 1U, msc_sector);
  if (ret == HAL_OK) memcpy(buffer, msc_sector + offset, bufsize);
  sd_storage_unlock();
  return (ret == HAL_OK) ? (int32_t)bufsize : TUD_MSC_RET_ERROR;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                           uint8_t *buffer, uint32_t bufsize)
{
  (void)lun;
  if (msc_check_range(lba, offset, bufsize) != 0 || sd_storage_lock(5000U) != 0) {
    return TUD_MSC_RET_ERROR;
  }

  HAL_StatusTypeDef ret = HAL_OK;
  if (offset != 0U || bufsize != 512U) ret = sd_storage_read_locked(lba, 1U, msc_sector);
  if (ret == HAL_OK) {
    memcpy(msc_sector + offset, buffer, bufsize);
    ret = sd_storage_write_locked(lba, 1U, msc_sector);
  }
  sd_storage_unlock();
  return (ret == HAL_OK) ? (int32_t)bufsize : TUD_MSC_RET_ERROR;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition,
                           bool start, bool load_eject)
{
  (void)lun;
  (void)power_condition;
  (void)start;
  (void)load_eject;
  return true;
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16],
                        void *buffer, uint16_t bufsize)
{
  (void)buffer;
  (void)bufsize;
  (void)tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
  (void)scsi_cmd;
  return TUD_MSC_RET_ERROR;
}
