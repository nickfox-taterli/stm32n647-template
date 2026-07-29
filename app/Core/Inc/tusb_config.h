#ifndef TUSB_CONFIG_H_
#define TUSB_CONFIG_H_

#define BOARD_TUD_RHPORT       0
#define CFG_TUSB_MCU           OPT_MCU_STM32N6
#define CFG_TUSB_OS            OPT_OS_FREERTOS
#define CFG_TUD_ENABLED        1
#define CFG_TUD_MAX_SPEED      OPT_MODE_HIGH_SPEED

#define CFG_TUD_ENDPOINT0_SIZE 64
#define CFG_TUD_CDC            0
#define CFG_TUD_MSC            1
#define CFG_TUD_HID            0
#define CFG_TUD_MIDI           0
#define CFG_TUD_VENDOR         0
#define CFG_TUD_MSC_EP_BUFSIZE 512

#define CFG_TUSB_DEBUG         0
#define CFG_TUSB_MEM_ALIGN     __attribute__((aligned(4)))
#define CFG_TUD_MEM_SECTION

#endif /* TUSB_CONFIG_H_ */
