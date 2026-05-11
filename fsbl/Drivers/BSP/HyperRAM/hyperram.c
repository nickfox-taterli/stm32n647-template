#include "hyperram.h"
#include "stm32n6xx_ll_xspi.h"

#define HYPERRAM_IDENTIFICATION_REGISTER_0              ((uint32_t)0x00000000 << 1)
#define HYPERRAM_IDENTIFICATION_REGISTER_1              ((uint32_t)0x00000001 << 1)
#define HYPERRAM_CONFIGURATION_REGISTER_1               ((uint32_t)0x00000801 << 1)

#define HYPERRAM_IDENTIFICATION_0_MANUFACTURER_POS      (0)
#define HYPERRAM_IDENTIFICATION_0_COL_COUNT_POS         (8)
#define HYPERRAM_IDENTIFICATION_0_ROW_COUNT_POS         (4)
#define HYPERRAM_IDENTIFICATION_1_DEVICE_TYPE_POS       (0)
#define HYPERRAM_CONFIGURATION_1_MASTER_CLOCK_TYPE_POS  (6)

#define HYPERRAM_IDENTIFICATION_0_MANUFACTURER_MASK     ((uint16_t)0x000F << HYPERRAM_IDENTIFICATION_0_MANUFACTURER_POS)
#define HYPERRAM_IDENTIFICATION_0_COL_COUNT_MASK        ((uint16_t)0x000F << HYPERRAM_IDENTIFICATION_0_COL_COUNT_POS)
#define HYPERRAM_IDENTIFICATION_0_ROW_COUNT_MASK        ((uint16_t)0x001F << HYPERRAM_IDENTIFICATION_0_ROW_COUNT_POS)
#define HYPERRAM_IDENTIFICATION_1_DEVICE_TYPE_MASK      ((uint16_t)0x000F << HYPERRAM_IDENTIFICATION_1_DEVICE_TYPE_POS)
#define HYPERRAM_CONFIGURATION_1_MASTER_CLOCK_TYPE_MASK ((uint16_t)0x0001 << HYPERRAM_CONFIGURATION_1_MASTER_CLOCK_TYPE_POS)

#define HYPERRAM_IDENTIFICATION_0_MANUFACTURER          ((uint16_t)0x0006)
#define HYPERRAM_IDENTIFICATION_1_DEVICE_TYPE           ((uint16_t)0x0001)

static HyperRAM_StatusTypeDef HyperRAM_ReadRegister(XSPI_TypeDef *XSPIx, HyperRAM_ObjectTypeDef *HyperRAMObject, uint32_t Address, uint16_t *Data);
static HyperRAM_StatusTypeDef HyperRAM_WriteRegister(XSPI_TypeDef *XSPIx, HyperRAM_ObjectTypeDef *HyperRAMObject, uint32_t Address, uint16_t Data);
static HyperRAM_StatusTypeDef HyperRAM_GetID0(XSPI_TypeDef *XSPIx, HyperRAM_ObjectTypeDef *HyperRAMObject, uint16_t *ID);
static HyperRAM_StatusTypeDef HyperRAM_GetID1(XSPI_TypeDef *XSPIx, HyperRAM_ObjectTypeDef *HyperRAMObject, uint16_t *ID);
static HyperRAM_StatusTypeDef HyperRAM_EnableDifferentialClock(XSPI_TypeDef *XSPIx, HyperRAM_ObjectTypeDef *HyperRAMObject);

HyperRAM_StatusTypeDef HyperRAM_Init(XSPI_TypeDef *XSPIx, HyperRAM_ObjectTypeDef *HyperRAMObject)
{
    uint16_t ID[2];

    HyperRAMObject->BaseCommand.AddressSpace = LL_XSPI_REGISTER_ADDRESS_SPACE;
    HyperRAMObject->BaseCommand.Address = 0x00000000;
    HyperRAMObject->BaseCommand.AddressWidth = LL_XSPI_ADDRESS_32_BITS;
    HyperRAMObject->BaseCommand.DataLength = 2;
    HyperRAMObject->BaseCommand.DQSMode = LL_XSPI_DQS_ENABLE;
    HyperRAMObject->BaseCommand.DataMode = LL_XSPI_DATA_8_LINES;

    HyperRAM_GetID0(XSPIx, HyperRAMObject, &ID[0]);
    if ((ID[0] & HYPERRAM_IDENTIFICATION_0_MANUFACTURER_MASK) != HYPERRAM_IDENTIFICATION_0_MANUFACTURER)
    {
        goto Error;
    }

    HyperRAM_GetID1(XSPIx, HyperRAMObject, &ID[1]);
    if ((ID[1] & HYPERRAM_IDENTIFICATION_1_DEVICE_TYPE_MASK) != HYPERRAM_IDENTIFICATION_1_DEVICE_TYPE)
    {
        goto Error;
    }

    HyperRAMObject->Size = (uint32_t)1 << (((((ID[0] & HYPERRAM_IDENTIFICATION_0_COL_COUNT_MASK) >> HYPERRAM_IDENTIFICATION_0_COL_COUNT_POS) + 1) + (((ID[0] & HYPERRAM_IDENTIFICATION_0_ROW_COUNT_MASK) >> HYPERRAM_IDENTIFICATION_0_ROW_COUNT_POS) + 1)) + 1);

    if (HyperRAM_EnableDifferentialClock(XSPIx, HyperRAMObject) != HyperRAM_OK)
    {
        goto Error;
    }

    return HyperRAM_OK;

Error:
    return HyperRAM_ERROR;
}

HyperRAM_StatusTypeDef HyperRAM_EnableMemoryMappedMode(XSPI_TypeDef *XSPIx, HyperRAM_ObjectTypeDef *HyperRAMObject)
{
    XSPI_HyperbusCmdTypeDef Cmd = HyperRAMObject->BaseCommand;

    Cmd.AddressSpace = LL_XSPI_MEMORY_ADDRESS_SPACE;
    LL_XSPI_HyperbusCmd(XSPIx,
                        Cmd.AddressSpace, Cmd.Address, Cmd.DataLength,
                        Cmd.DQSMode, Cmd.DataMode, Cmd.AddressWidth);

    LL_XSPI_EnableMemoryMappedMode(XSPIx,
                                    XSPI_CR_TCEN, 0x34, 0U, 0U);

    return HyperRAM_OK;
}

HyperRAM_StatusTypeDef HyperRAM_DisableMemoryMappedMode(XSPI_TypeDef *XSPIx)
{
    __DSB();
    LL_XSPI_Abort(XSPIx);

    return HyperRAM_OK;
}

HyperRAM_StatusTypeDef HyperRAM_GetMemoryMappedAddress(XSPI_TypeDef *XSPIx, uint32_t *BaseAddress)
{
    if (XSPIx == XSPI1)
    {
        *BaseAddress = XSPI1_BASE;
    }
    else if (XSPIx == XSPI2)
    {
        *BaseAddress = XSPI2_BASE;
    }
    else
    {
        return HyperRAM_ERROR;
    }

    return HyperRAM_OK;
}

static HyperRAM_StatusTypeDef HyperRAM_ReadRegister(XSPI_TypeDef *XSPIx, HyperRAM_ObjectTypeDef *HyperRAMObject, uint32_t Address, uint16_t *Data)
{
    XSPI_HyperbusCmdTypeDef Cmd = HyperRAMObject->BaseCommand;

    Cmd.Address = Address;
    LL_XSPI_HyperbusCmd(XSPIx,
                        Cmd.AddressSpace, Cmd.Address, Cmd.DataLength,
                        Cmd.DQSMode, Cmd.DataMode, Cmd.AddressWidth);

    LL_XSPI_Receive(XSPIx, (uint8_t *)Data,
                     LL_XSPI_MEMTYPE_HYPERBUS);

    return HyperRAM_OK;
}

static HyperRAM_StatusTypeDef HyperRAM_WriteRegister(XSPI_TypeDef *XSPIx, HyperRAM_ObjectTypeDef *HyperRAMObject, uint32_t Address, uint16_t Data)
{
    XSPI_HyperbusCmdTypeDef Cmd = HyperRAMObject->BaseCommand;

    Cmd.Address = Address;
    LL_XSPI_HyperbusCmd(XSPIx,
                        Cmd.AddressSpace, Cmd.Address, Cmd.DataLength,
                        Cmd.DQSMode, Cmd.DataMode, Cmd.AddressWidth);

    LL_XSPI_Transmit(XSPIx, (uint8_t *)&Data);

    return HyperRAM_OK;
}

static HyperRAM_StatusTypeDef HyperRAM_GetID0(XSPI_TypeDef *XSPIx, HyperRAM_ObjectTypeDef *HyperRAMObject, uint16_t *ID)
{
    if (HyperRAM_ReadRegister(XSPIx, HyperRAMObject, HYPERRAM_IDENTIFICATION_REGISTER_0, ID) != HyperRAM_OK)
    {
        goto Error;
    }

    return HyperRAM_OK;

Error:
    LL_XSPI_Abort(XSPIx);
    return HyperRAM_ERROR;
}

static HyperRAM_StatusTypeDef HyperRAM_GetID1(XSPI_TypeDef *XSPIx, HyperRAM_ObjectTypeDef *HyperRAMObject, uint16_t *ID)
{
    if (HyperRAM_ReadRegister(XSPIx, HyperRAMObject, HYPERRAM_IDENTIFICATION_REGISTER_1, ID) != HyperRAM_OK)
    {
        goto Error;
    }

    return HyperRAM_OK;

Error:
    LL_XSPI_Abort(XSPIx);
    return HyperRAM_ERROR;
}

static HyperRAM_StatusTypeDef HyperRAM_EnableDifferentialClock(XSPI_TypeDef *XSPIx, HyperRAM_ObjectTypeDef *HyperRAMObject)
{
    uint16_t Cfg;

    if (HyperRAM_ReadRegister(XSPIx, HyperRAMObject, HYPERRAM_CONFIGURATION_REGISTER_1, &Cfg) != HyperRAM_OK)
    {
        goto Error;
    }

    Cfg &= ~HYPERRAM_CONFIGURATION_1_MASTER_CLOCK_TYPE_MASK;
    if (HyperRAM_WriteRegister(XSPIx, HyperRAMObject, HYPERRAM_CONFIGURATION_REGISTER_1, Cfg) != HyperRAM_OK)
    {
        goto Error;
    }

    return HyperRAM_OK;

Error:
    LL_XSPI_Abort(XSPIx);
    return HyperRAM_ERROR;
}
