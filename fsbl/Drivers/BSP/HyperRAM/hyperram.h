#ifndef __HYPERRAM_H
#define __HYPERRAM_H

#include "stm32n6xx_ll_xspi.h"

typedef enum
{
    HyperRAM_OK,
    HyperRAM_ERROR,
} HyperRAM_StatusTypeDef;

typedef struct
{
    XSPI_HyperbusCmdTypeDef BaseCommand;

    uint32_t Size;
} HyperRAM_ObjectTypeDef;

HyperRAM_StatusTypeDef HyperRAM_Init(XSPI_TypeDef *XSPIx, HyperRAM_ObjectTypeDef *HyperRAMObject);
HyperRAM_StatusTypeDef HyperRAM_EnableMemoryMappedMode(XSPI_TypeDef *XSPIx, HyperRAM_ObjectTypeDef *HyperRAMObject);
HyperRAM_StatusTypeDef HyperRAM_DisableMemoryMappedMode(XSPI_TypeDef *XSPIx);
HyperRAM_StatusTypeDef HyperRAM_GetMemoryMappedAddress(XSPI_TypeDef *XSPIx, uint32_t *BaseAddress);

#endif
