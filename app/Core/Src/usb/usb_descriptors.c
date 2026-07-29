#include "tusb.h"
#include "usb_descriptors.h"
#include <string.h>

#define USB_VID 0x0483U
#define USB_PID 0x5721U
#define USB_BCD 0x0200U

enum { ITF_NUM_MSC = 0, ITF_NUM_TOTAL };
#define EPNUM_MSC_OUT 0x01U
#define EPNUM_MSC_IN  0x81U
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

static tusb_desc_device_t const desc_device = {
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = USB_BCD,
  .bDeviceClass       = 0,
  .bDeviceSubClass    = 0,
  .bDeviceProtocol    = 0,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor           = USB_VID,
  .idProduct          = USB_PID,
  .bcdDevice          = 0x0100,
  .iManufacturer      = 1,
  .iProduct           = 2,
  .iSerialNumber      = 3,
  .bNumConfigurations = 1,
};

static uint8_t const desc_fs_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x80, 100),
  TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),
};

#if TUD_OPT_HIGH_SPEED
static uint8_t const desc_hs_configuration[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x80, 100),
  TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EPNUM_MSC_OUT, EPNUM_MSC_IN, 512),
};

static uint8_t desc_other_speed_configuration[CONFIG_TOTAL_LEN];
static tusb_desc_device_qualifier_t const desc_device_qualifier = {
  .bLength            = sizeof(tusb_desc_device_qualifier_t),
  .bDescriptorType    = TUSB_DESC_DEVICE_QUALIFIER,
  .bcdUSB             = USB_BCD,
  .bDeviceClass       = 0,
  .bDeviceSubClass    = 0,
  .bDeviceProtocol    = 0,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .bNumConfigurations = 1,
  .bReserved          = 0,
};
#endif

uint8_t const *tud_descriptor_device_cb(void)
{
  return (uint8_t const *)&desc_device;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
  (void)index;
#if TUD_OPT_HIGH_SPEED
  return (tud_speed_get() == TUSB_SPEED_HIGH) ? desc_hs_configuration : desc_fs_configuration;
#else
  return desc_fs_configuration;
#endif
}

#if TUD_OPT_HIGH_SPEED
uint8_t const *tud_descriptor_device_qualifier_cb(void)
{
  return (uint8_t const *)&desc_device_qualifier;
}

uint8_t const *tud_descriptor_other_speed_configuration_cb(uint8_t index)
{
  (void)index;
  memcpy(desc_other_speed_configuration,
         (tud_speed_get() == TUSB_SPEED_HIGH) ? desc_fs_configuration : desc_hs_configuration,
         CONFIG_TOTAL_LEN);
  desc_other_speed_configuration[1] = TUSB_DESC_OTHER_SPEED_CONFIG;
  return desc_other_speed_configuration;
}
#endif

enum { STRID_LANGID = 0, STRID_MANUFACTURER, STRID_PRODUCT, STRID_SERIAL };
static char const *const string_desc[] = {
  "", "STMicroelectronics", "STM32N6 SD NAND", "N647SDNAND",
};
static uint16_t desc_string[32 + 1];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void)langid;
  size_t count;

  if (index == STRID_LANGID) {
    desc_string[1] = 0x0409;
    count = 1;
  } else {
    if (index >= (sizeof(string_desc) / sizeof(string_desc[0]))) return NULL;
    count = strlen(string_desc[index]);
    if (count > 32) count = 32;
    for (size_t i = 0; i < count; ++i) desc_string[i + 1] = string_desc[index][i];
  }

  desc_string[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2U * count + 2U));
  return desc_string;
}
