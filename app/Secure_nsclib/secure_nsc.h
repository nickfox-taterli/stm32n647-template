#ifndef SECURE_NSC_H
#define SECURE_NSC_H

#include <stdint.h>

#if defined ( __ICCARM__ )
#  define CMSE_NS_CALL  __cmse_nonsecure_call
#  define CMSE_NS_ENTRY __cmse_nonsecure_entry
#else
#  define CMSE_NS_CALL  __attribute((cmse_nonsecure_call))
#  define CMSE_NS_ENTRY __attribute((cmse_nonsecure_entry))
#endif

typedef void (CMSE_NS_CALL *funcptr)(void);
typedef funcptr funcptr_NS;

typedef enum
{
  SECURE_FAULT_CB_ID     = 0x00U,
  GTZC_ERROR_CB_ID       = 0x01U
} SECURE_CallbackIDTypeDef;

void SECURE_RegisterCallback(SECURE_CallbackIDTypeDef CallbackId, void *func);

#endif
