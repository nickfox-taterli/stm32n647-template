#ifndef EWL_CONF_H
#define EWL_CONF_H

#define EWL_USE_MALLOC_MM   0
#define EWL_USE_FREERTOS_MM 1
#define EWL_USE_THREADX_MM  2
#define EWL_USE_STM32MPM_MM 3
#define EWL_USER_MM         4

/* The application supplies a dedicated non-cacheable HyperRAM arena. */
#define EWL_ALLOC_API EWL_USER_MM

#define EWL_USE_POLLING_SYNC  0
#define EWL_USE_FREERTOS_SYNC 1
#define EWL_USE_THREADX_SYNC  2
#define EWL_USER_SYNC         4

/* Polling keeps first bring-up independent of a VENC RTOS interrupt. */
#define EWL_SYNC_API EWL_USE_POLLING_SYNC

#define ALIGNMENT_INCR 8UL
#define MEM_CHUNKS 32

#define assert_param(expr) ((void)0U)

#endif /* EWL_CONF_H */
