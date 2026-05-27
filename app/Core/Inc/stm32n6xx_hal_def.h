#ifndef STM32N6xx_HAL_DEF_H
#define STM32N6xx_HAL_DEF_H

#include "stm32n6xx.h"
#include <stddef.h>

typedef enum {
    HAL_OK       = 0x00,
    HAL_ERROR    = 0x01,
    HAL_BUSY     = 0x02,
    HAL_TIMEOUT  = 0x03
} HAL_StatusTypeDef;

typedef enum {
    HAL_UNLOCKED = 0x00,
    HAL_LOCKED   = 0x01
} HAL_LockTypeDef;

typedef enum {
    HAL_TICK_FREQ_10HZ    = 100U,
    HAL_TICK_FREQ_100HZ   = 10U,
    HAL_TICK_FREQ_1KHZ    = 1U,
    HAL_TICK_FREQ_DEFAULT = HAL_TICK_FREQ_1KHZ
} HAL_TickFreqTypeDef;

#define HAL_MAX_DELAY 0xFFFFFFFFU
#define UNUSED(X) ((void)(X))

#define __HAL_LOCK(__HANDLE__) \
    do { \
        if ((__HANDLE__)->Lock == HAL_LOCKED) return HAL_BUSY; \
        (__HANDLE__)->Lock = HAL_LOCKED; \
    } while (0)

#define __HAL_UNLOCK(__HANDLE__) \
    do { \
        (__HANDLE__)->Lock = HAL_UNLOCKED; \
    } while (0)

#define __HAL_RESET_HANDLE_STATE(__HANDLE__) ((__HANDLE__)->State = 0)

#define __weak __attribute__((weak))

#define ALIGN_32BYTES(buf) buf __attribute__((aligned(32)))

#endif /* STM32N6xx_HAL_DEF_H */
