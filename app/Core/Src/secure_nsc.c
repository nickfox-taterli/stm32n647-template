#include <stddef.h>
#include "stm32n6xx.h"
#include "secure_nsc.h"

#if defined ( __ICCARM__ )
#  define CMSE_NS_CALL  __cmse_nonsecure_call
#  define CMSE_NS_ENTRY __cmse_nonsecure_entry
#else
#  define CMSE_NS_CALL  __attribute((cmse_nonsecure_call))
#  define CMSE_NS_ENTRY __attribute((cmse_nonsecure_entry))
#endif

void *pSecureFaultCallback = NULL;
void *pSecureErrorCallback = NULL;

CMSE_NS_ENTRY void SECURE_RegisterCallback(SECURE_CallbackIDTypeDef CallbackId, void *func)
{
  if(func != NULL)
  {
    switch(CallbackId)
    {
      case SECURE_FAULT_CB_ID:
        pSecureFaultCallback = func;
        break;
      case GTZC_ERROR_CB_ID:
        pSecureErrorCallback = func;
        break;
      default:
        break;
    }
  }
}
