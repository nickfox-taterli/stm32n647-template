#include "norflash.h"
#include "stm32n6xx_ll_xspi.h"
#include "stm32n6xx_ll_utils.h"

/* ========================================================================== */
/* XSPI-level functions (from norflash_xspi.c)                                */
/* ========================================================================== */

void NORFlash_XSPI_Init(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject,
                         uint32_t MemoryMode, uint32_t SampleShifting, uint32_t MemoryType)
{
    XSPIObject->MemoryMode = MemoryMode;
    XSPIObject->SampleShifting = SampleShifting;
    XSPIObject->MemoryType = MemoryType;
    XSPIObject->BaseCommand.OperationType = LL_XSPI_OPTYPE_COMMON_CFG;
    XSPIObject->BaseCommand.IOSelect = LL_XSPI_SELECT_IO_7_0;
    XSPIObject->BaseCommand.Instruction = 0;
    XSPIObject->BaseCommand.InstructionMode = LL_XSPI_INSTRUCTION_1_LINE;
    XSPIObject->BaseCommand.InstructionWidth = LL_XSPI_INSTRUCTION_8_BITS;
    XSPIObject->BaseCommand.InstructionDTRMode = LL_XSPI_INSTRUCTION_DTR_DISABLE;
    XSPIObject->BaseCommand.Address = 0;
    XSPIObject->BaseCommand.AddressMode = LL_XSPI_ADDRESS_1_LINE;
    XSPIObject->BaseCommand.AddressWidth = LL_XSPI_ADDRESS_24_BITS;
    XSPIObject->BaseCommand.AddressDTRMode = LL_XSPI_ADDRESS_DTR_DISABLE;
    XSPIObject->BaseCommand.AlternateBytesMode = LL_XSPI_ALT_BYTES_NONE;
    XSPIObject->BaseCommand.DataMode = LL_XSPI_DATA_1_LINE;
    XSPIObject->BaseCommand.DataLength = 0;
    XSPIObject->BaseCommand.DataDTRMode = LL_XSPI_DATA_DTR_DISABLE;
    XSPIObject->BaseCommand.DummyCycles = 0;
    XSPIObject->BaseCommand.DQSMode = LL_XSPI_DQS_DISABLE;

    __DSB();
    LL_XSPI_Abort(XSPIx);
}

NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_SetClock(XSPI_TypeDef *XSPIx, uint32_t ClockInput, uint32_t ClockRequested, uint32_t *ClockReal)
{
    uint32_t Divider;

    Divider = ClockInput / ClockRequested;
    if (Divider >= 1)
    {
        if ((ClockInput / Divider) <= ClockRequested)
        {
            Divider--;
        }
    }

    LL_XSPI_SetClockPrescaler(XSPIx, Divider);

    if (ClockReal != NULL)
    {
        *ClockReal = ClockInput / (Divider + 1);
    }

    return NORFlash_XSPI_OK;
}

NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_EnableMapMode(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t CommandRead, uint8_t DummyRead, uint8_t CommandWrite, uint8_t DummyWrite)
{
    XSPI_RegularCmdTypeDef Cmd = XSPIObject->BaseCommand;

    Cmd.OperationType = LL_XSPI_OPTYPE_READ_CFG;
    Cmd.Instruction = (Cmd.InstructionWidth == LL_XSPI_INSTRUCTION_16_BITS) ? (((uint16_t)CommandRead << 8) | (uint8_t)(~CommandRead & 0xFF)) : CommandRead;
    Cmd.DummyCycles = DummyRead;
    if (LL_XSPI_ConfigRegularCmd(XSPIx,
                                 XSPIObject->MemoryMode,
                                 XSPIObject->SampleShifting,
                                 &Cmd) != SUCCESS)
    {
        goto Error;
    }

    Cmd.OperationType = LL_XSPI_OPTYPE_WRITE_CFG;
    Cmd.Instruction = (Cmd.InstructionWidth == LL_XSPI_INSTRUCTION_16_BITS) ? (((uint16_t)CommandWrite << 8) | (uint8_t)(~CommandWrite & 0xFF)) : CommandWrite;
    Cmd.DummyCycles = DummyWrite;
    if (LL_XSPI_ConfigRegularCmd(XSPIx,
                                 XSPIObject->MemoryMode,
                                 XSPIObject->SampleShifting,
                                 &Cmd) != SUCCESS)
    {
        goto Error;
    }

    LL_XSPI_EnableMemoryMappedMode(XSPIx, 0U, 0x50,
                                    0U, 0U);

    return NORFlash_XSPI_OK;

Error:
    LL_XSPI_Abort(XSPIx);
    return NORFlash_XSPI_ERROR;
}

NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_DisableMapMode(XSPI_TypeDef *XSPIx)
{
    __DSB();
    LL_XSPI_Abort(XSPIx);

    return NORFlash_XSPI_OK;
}

NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_ConfigPHYLink(NORFlash_XSPI_ObjectTypeDef *XSPIObject, NORFlash_XSPI_PhysicalLinkTypeDef PhyLink)
{
    switch (PhyLink)
    {
        case NORFlash_PHY_LINK_1S1S1S:
        {
            XSPIObject->BaseCommand.InstructionMode = LL_XSPI_INSTRUCTION_1_LINE;
            XSPIObject->BaseCommand.InstructionWidth = LL_XSPI_INSTRUCTION_8_BITS;
            XSPIObject->BaseCommand.InstructionDTRMode = LL_XSPI_INSTRUCTION_DTR_DISABLE;
            XSPIObject->BaseCommand.AddressMode = LL_XSPI_ADDRESS_1_LINE;
            XSPIObject->BaseCommand.AddressWidth = LL_XSPI_ADDRESS_24_BITS;
            XSPIObject->BaseCommand.AddressDTRMode = LL_XSPI_ADDRESS_DTR_DISABLE;
            XSPIObject->BaseCommand.DataMode = LL_XSPI_DATA_1_LINE;
            XSPIObject->BaseCommand.DataDTRMode = LL_XSPI_DATA_DTR_DISABLE;
            XSPIObject->BaseCommand.DummyCycles = 0;
            XSPIObject->BaseCommand.DQSMode = LL_XSPI_DQS_DISABLE;
            break;
        }
        case NORFlash_PHY_LINK_4S4S4S:
        {
            XSPIObject->BaseCommand.InstructionMode = LL_XSPI_INSTRUCTION_4_LINES;
            XSPIObject->BaseCommand.InstructionWidth = LL_XSPI_INSTRUCTION_8_BITS;
            XSPIObject->BaseCommand.InstructionDTRMode = LL_XSPI_INSTRUCTION_DTR_DISABLE;
            XSPIObject->BaseCommand.AddressMode = LL_XSPI_ADDRESS_4_LINES;
            XSPIObject->BaseCommand.AddressWidth = LL_XSPI_ADDRESS_24_BITS;
            XSPIObject->BaseCommand.AddressDTRMode = LL_XSPI_ADDRESS_DTR_DISABLE;
            XSPIObject->BaseCommand.DataMode = LL_XSPI_DATA_4_LINES;
            XSPIObject->BaseCommand.DataDTRMode = LL_XSPI_DATA_DTR_DISABLE;
            XSPIObject->BaseCommand.DummyCycles = 0;
            XSPIObject->BaseCommand.DQSMode = LL_XSPI_DQS_DISABLE;
            break;
        }
        case NORFlash_PHY_LINK_8D8D8D:
        {
            XSPIObject->BaseCommand.InstructionMode = LL_XSPI_INSTRUCTION_8_LINES;
            XSPIObject->BaseCommand.InstructionWidth = LL_XSPI_INSTRUCTION_16_BITS;
            XSPIObject->BaseCommand.InstructionDTRMode = LL_XSPI_INSTRUCTION_DTR_ENABLE;
            XSPIObject->BaseCommand.AddressMode = LL_XSPI_ADDRESS_8_LINES;
            XSPIObject->BaseCommand.AddressWidth = LL_XSPI_ADDRESS_32_BITS;
            XSPIObject->BaseCommand.AddressDTRMode = LL_XSPI_ADDRESS_DTR_ENABLE;
            XSPIObject->BaseCommand.DataMode = LL_XSPI_DATA_8_LINES;
            XSPIObject->BaseCommand.DataDTRMode = LL_XSPI_DATA_DTR_ENABLE;
            XSPIObject->BaseCommand.DummyCycles = 0;
            XSPIObject->BaseCommand.DQSMode = LL_XSPI_DQS_ENABLE;
            break;
        }
        default:
        {
            goto Error;
        }
    }

    XSPIObject->PhyLink = PhyLink;

    return NORFlash_XSPI_OK;

Error:
    return NORFlash_XSPI_ERROR;
}

NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_CommandSendData(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t Command, uint8_t *Data, uint16_t DataSize)
{
    XSPI_RegularCmdTypeDef Cmd = XSPIObject->BaseCommand;

    Cmd.Instruction = (Cmd.InstructionWidth == LL_XSPI_INSTRUCTION_16_BITS) ? (((uint16_t)Command << 8) | (uint8_t)(~Command & 0xFF)) : Command;
    Cmd.AddressMode = LL_XSPI_ADDRESS_NONE;
    Cmd.DataMode = (DataSize == 0) ? LL_XSPI_DATA_NONE : Cmd.DataMode;
    Cmd.DataLength = DataSize;
    if (LL_XSPI_ConfigRegularCmd(XSPIx,
                                 XSPIObject->MemoryMode,
                                 XSPIObject->SampleShifting,
                                 &Cmd) != SUCCESS)
    {
        goto Error;
    }

    if (DataSize != 0)
    {
        LL_XSPI_Transmit(XSPIx, Data);
    }

    return NORFlash_XSPI_OK;

Error:
    LL_XSPI_Abort(XSPIx);
    return NORFlash_XSPI_ERROR;
}

NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_CommandSendAddress(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t Command, uint32_t Address)
{
    XSPI_RegularCmdTypeDef Cmd = XSPIObject->BaseCommand;

    Cmd.Instruction = (Cmd.InstructionWidth == LL_XSPI_INSTRUCTION_16_BITS) ? (((uint16_t)Command << 8) | (uint8_t)(~Command & 0xFF)) : Command;
    Cmd.Address = Address;
    Cmd.DataMode = LL_XSPI_DATA_NONE;
    if (LL_XSPI_ConfigRegularCmd(XSPIx,
                                 XSPIObject->MemoryMode,
                                 XSPIObject->SampleShifting,
                                 &Cmd) != SUCCESS)
    {
        goto Error;
    }

    return NORFlash_XSPI_OK;

Error:
    LL_XSPI_Abort(XSPIx);
    return NORFlash_XSPI_ERROR;
}

NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_CommandRead(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t Command, uint8_t *Data, uint16_t DataSize)
{
    XSPI_RegularCmdTypeDef Cmd = XSPIObject->BaseCommand;

    Cmd.Instruction = (Cmd.InstructionWidth == LL_XSPI_INSTRUCTION_16_BITS) ? (((uint16_t)Command << 8) | (uint8_t)(~Command & 0xFF)) : Command;
    Cmd.AddressMode = LL_XSPI_ADDRESS_NONE;
    Cmd.DataMode = (DataSize == 0) ? LL_XSPI_DATA_NONE : Cmd.DataMode;
    Cmd.DataLength = DataSize;
    if (LL_XSPI_ConfigRegularCmd(XSPIx,
                                 XSPIObject->MemoryMode,
                                 XSPIObject->SampleShifting,
                                 &Cmd) != SUCCESS)
    {
        goto Error;
    }

    if (DataSize != 0)
    {
        LL_XSPI_Receive(XSPIx, Data,
                        XSPIObject->MemoryType);
    }

    return NORFlash_XSPI_OK;

Error:
    LL_XSPI_Abort(XSPIx);
    return NORFlash_XSPI_ERROR;
}

NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_CommandSendAddressReadData(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t Command, uint32_t Address, uint8_t *Data, uint16_t DataSize)
{
    XSPI_RegularCmdTypeDef Cmd = XSPIObject->BaseCommand;

    Cmd.Instruction = (Cmd.InstructionWidth == LL_XSPI_INSTRUCTION_16_BITS) ? (((uint16_t)Command << 8) | (uint8_t)(~Command & 0xFF)) : Command;
    Cmd.Address = Address;
    Cmd.AddressWidth = LL_XSPI_ADDRESS_32_BITS;
    Cmd.DataMode = (DataSize == 0) ? LL_XSPI_DATA_NONE : Cmd.DataMode;
    Cmd.DataLength = DataSize;
    if (LL_XSPI_ConfigRegularCmd(XSPIx,
                                 XSPIObject->MemoryMode,
                                 XSPIObject->SampleShifting,
                                 &Cmd) != SUCCESS)
    {
        goto Error;
    }

    if (DataSize != 0)
    {
        LL_XSPI_Receive(XSPIx, Data,
                        XSPIObject->MemoryType);
    }

    return NORFlash_XSPI_OK;

Error:
    LL_XSPI_Abort(XSPIx);
    return NORFlash_XSPI_ERROR;
}

NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_CommandSendAddressSendData(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t Command, uint32_t Address, uint8_t *Data, uint16_t DataSize)
{
    XSPI_RegularCmdTypeDef Cmd = XSPIObject->BaseCommand;

    Cmd.Instruction = (Cmd.InstructionWidth == LL_XSPI_INSTRUCTION_16_BITS) ? (((uint16_t)Command << 8) | (uint8_t)(~Command & 0xFF)) : Command;
    Cmd.Address = Address;
    Cmd.AddressWidth = LL_XSPI_ADDRESS_32_BITS;
    Cmd.DataMode = (DataSize == 0) ? LL_XSPI_DATA_NONE : Cmd.DataMode;
    Cmd.DataLength = DataSize;
    if (LL_XSPI_ConfigRegularCmd(XSPIx,
                                 XSPIObject->MemoryMode,
                                 XSPIObject->SampleShifting,
                                 &Cmd) != SUCCESS)
    {
        goto Error;
    }

    if (DataSize != 0)
    {
        LL_XSPI_Transmit(XSPIx, Data);
    }

    return NORFlash_XSPI_OK;

Error:
    LL_XSPI_Abort(XSPIx);
    return NORFlash_XSPI_ERROR;
}

NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_WaitStatusRegister(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t Command, uint16_t MatchValue, uint16_t MatchMask, uint8_t DataSize)
{
    XSPI_RegularCmdTypeDef Cmd = XSPIObject->BaseCommand;

    Cmd.Instruction = (Cmd.InstructionWidth == LL_XSPI_INSTRUCTION_16_BITS) ? (((uint16_t)Command << 8) | (uint8_t)(~Command & 0xFF)) : Command;
    Cmd.AddressMode = (Cmd.InstructionMode == LL_XSPI_INSTRUCTION_8_LINES) ? LL_XSPI_ADDRESS_8_LINES : LL_XSPI_ADDRESS_NONE;
    Cmd.Address = 0x00000000;
    Cmd.DataLength = DataSize;
    Cmd.DummyCycles = (Cmd.InstructionMode == LL_XSPI_INSTRUCTION_8_LINES) ? 4 : Cmd.DummyCycles;
    if (LL_XSPI_ConfigRegularCmd(XSPIx,
                                 XSPIObject->MemoryMode,
                                 XSPIObject->SampleShifting,
                                 &Cmd) != SUCCESS)
    {
        goto Error;
    }

    LL_XSPI_AutoPolling(XSPIx, MatchValue, MatchMask,
                        0x10, 0U, XSPI_CR_APMS,
                        XSPIObject->MemoryType);

    return NORFlash_XSPI_OK;

Error:
    LL_XSPI_Abort(XSPIx);
    return NORFlash_XSPI_ERROR;
}

NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_Write(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t Command, uint8_t Dummy, uint32_t Address, const uint8_t *Data, uint32_t DataSize)
{
    XSPI_RegularCmdTypeDef Cmd = XSPIObject->BaseCommand;

    Cmd.Instruction = (Cmd.InstructionWidth == LL_XSPI_INSTRUCTION_16_BITS) ? (((uint16_t)Command << 8) | (uint8_t)(~Command & 0xFF)) : Command;
    Cmd.Address = Address;
    Cmd.DataLength = DataSize;
    Cmd.DummyCycles = Dummy;
    if (LL_XSPI_ConfigRegularCmd(XSPIx,
                                 XSPIObject->MemoryMode,
                                 XSPIObject->SampleShifting,
                                 &Cmd) != SUCCESS)
    {
        goto Error;
    }

    LL_XSPI_Transmit(XSPIx, Data);

    return NORFlash_XSPI_OK;

Error:
    LL_XSPI_Abort(XSPIx);
    return NORFlash_XSPI_ERROR;
}

/* ========================================================================== */
/* MX25UM25645G chip init (from norflash_mx25um25645g.c)                      */
/* ========================================================================== */

static NORFlash_StatusTypeDef NORFlash_MX25UM25645G_Init(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject, uint32_t ClockInput, uint32_t *ClockRequested)
{
    uint8_t Data;

    NORFlashObject->Information.FlashSize = 0x2000000;
    NORFlashObject->Information.SectorSize = 0x1000;
    NORFlashObject->Information.PageSize = 0x100;
    NORFlashObject->Timing.EraseChip = 150000;
    NORFlashObject->Timing.EraseSector = 400;
    NORFlashObject->Timing.ProgramPage = 1;
    NORFlashObject->Command.MapRead.Command = 0xEE;
    NORFlashObject->Command.MapRead.Dummy = 20;
    NORFlashObject->Command.MapWrite.Command = 0x12;
    NORFlashObject->Command.MapWrite.Dummy = 0;
    NORFlashObject->Command.ProgramPage.Command = 0x12;
    NORFlashObject->Command.ProgramPage.Dummy = 0;
    NORFlashObject->Command.EraseSector.Command = 0x21;
    NORFlashObject->Command.EraseSector.Dummy = 0;

    NORFlash_XSPI_CommandSendAddressReadData(XSPIx, &(NORFlashObject->XSPIObject), 0x71, 0x00000000, &Data, sizeof(Data));
    if ((Data & (3 << 0)) != 0)
    {
        return NORFlash_ERROR;
    }

    Data &= ~(3 << 0);
    Data |= (2 << 0);

    NORFlash_EnableWrite(XSPIx, NORFlashObject);

    NORFlash_XSPI_CommandSendAddressSendData(XSPIx, &(NORFlashObject->XSPIObject), 0x72, 0x00000000, &Data, sizeof(Data));

    NORFlash_XSPI_ConfigPHYLink(&(NORFlashObject->XSPIObject), NORFlash_PHY_LINK_8D8D8D);

    if (ClockRequested != NULL)
    {
        *ClockRequested = ClockInput;
    }

    return NORFlash_OK;
}

/* ========================================================================== */
/* High-level NOR flash API (from norflash.c)                                 */
/* ========================================================================== */

NORFlash_StatusTypeDef NORFlash_Init(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject,
                                      uint32_t MemoryMode, uint32_t SampleShifting, uint32_t MemoryType,
                                      uint32_t ClockInput)
{
    uint8_t JEDECID[3];
    uint32_t ClockRequested;

    NORFlash_XSPI_Init(XSPIx, &(NORFlashObject->XSPIObject), MemoryMode, SampleShifting, MemoryType);

    if (NORFlash_XSPI_SetClock(XSPIx, ClockInput, 50000000, NULL) != NORFlash_XSPI_OK)
    {
        goto Error;
    }

    NORFlash_Reset(XSPIx, NORFlashObject);
    LL_mDelay(10);

    if (NORFlash_XSPI_CommandRead(XSPIx, &(NORFlashObject->XSPIObject), 0x9F, JEDECID, sizeof(JEDECID)) != NORFlash_XSPI_OK)
    {
        goto Error;
    }

    if (NORFlash_IS_MX25UM25645G(JEDECID))
    {
        if (NORFlash_MX25UM25645G_Init(XSPIx, NORFlashObject, ClockInput, &ClockRequested) != NORFlash_OK)
        {
            goto Error;
        }
    }
    else
    {
        goto Error;
    }

    if (NORFlash_XSPI_SetClock(XSPIx, ClockInput, ClockRequested, NULL) != NORFlash_XSPI_OK)
    {
        goto Error;
    }

    return NORFlash_OK;

Error:
    return NORFlash_ERROR;
}

void NORFlash_Reset(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject)
{
    uint8_t index;
    NORFlash_XSPI_PhysicalLinkTypeDef PhyLinkTable[] = {
        NORFlash_PHY_LINK_1S1S1S,
        NORFlash_PHY_LINK_4S4S4S,
        NORFlash_PHY_LINK_8D8D8D,
    };

    for (index = 0; index < (sizeof(PhyLinkTable) / sizeof(NORFlash_XSPI_PhysicalLinkTypeDef)); index++)
    {
        NORFlash_XSPI_ConfigPHYLink(&(NORFlashObject->XSPIObject), PhyLinkTable[index]);

        NORFlash_XSPI_CommandSendData(XSPIx, &(NORFlashObject->XSPIObject), 0x66, NULL, 0);
        NORFlash_XSPI_CommandSendData(XSPIx, &(NORFlashObject->XSPIObject), 0x99, NULL, 0);
    }

    NORFlash_XSPI_ConfigPHYLink(&(NORFlashObject->XSPIObject), NORFlash_PHY_LINK_1S1S1S);
}

NORFlash_StatusTypeDef NORFlash_WaitBusy(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject)
{
    uint16_t MatchValue = 0 << 0;
    uint16_t MatchMask = 1 << 0;
    uint8_t DataSize = 1;

    if (NORFlash_XSPI_WaitStatusRegister(XSPIx, &(NORFlashObject->XSPIObject), 0x05, MatchValue, MatchMask, DataSize) != NORFlash_XSPI_OK)
    {
        goto Error;
    }

    return NORFlash_OK;

Error:
    return NORFlash_ERROR;
}

NORFlash_StatusTypeDef NORFlash_EnableWrite(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject)
{
    uint16_t MatchValue = 1 << 1;
    uint16_t MatchMask = 1 << 1;
    uint8_t DataSize = 1;

    if (NORFlash_XSPI_CommandSendData(XSPIx, &(NORFlashObject->XSPIObject), 0x06, NULL, 0) != NORFlash_XSPI_OK)
    {
        goto Error;
    }

    if (NORFlash_XSPI_WaitStatusRegister(XSPIx, &(NORFlashObject->XSPIObject), 0x05, MatchValue, MatchMask, DataSize) != NORFlash_XSPI_OK)
    {
        goto Error;
    }

    return NORFlash_OK;

Error:
    return NORFlash_ERROR;
}

NORFlash_StatusTypeDef NORFlash_EnableMemoryMappedMode(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject)
{
    if (NORFlash_XSPI_EnableMapMode(XSPIx, &(NORFlashObject->XSPIObject), NORFlashObject->Command.MapRead.Command, NORFlashObject->Command.MapRead.Dummy, NORFlashObject->Command.MapWrite.Command, NORFlashObject->Command.MapWrite.Dummy) != NORFlash_XSPI_OK)
    {
        goto Error;
    }

    return NORFlash_OK;

Error:
    return NORFlash_ERROR;
}

NORFlash_StatusTypeDef NORFlash_DisableMemoryMappedMode(XSPI_TypeDef *XSPIx)
{
    if (NORFlash_XSPI_DisableMapMode(XSPIx) != NORFlash_XSPI_OK)
    {
        goto Error;
    }

    return NORFlash_OK;

Error:
    return NORFlash_ERROR;
}

NORFlash_StatusTypeDef NORFlash_GetMemoryMappedAddress(XSPI_TypeDef *XSPIx, uint32_t *BaseAddress)
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
        goto Error;
    }

    return NORFlash_OK;

Error:
    return NORFlash_ERROR;
}

NORFlash_StatusTypeDef NORFlash_EraseChip(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject)
{
    if (NORFlash_WaitBusy(XSPIx, NORFlashObject) != NORFlash_OK)
    {
        goto Error;
    }

    if (NORFlash_EnableWrite(XSPIx, NORFlashObject) != NORFlash_OK)
    {
        goto Error;
    }

    if (NORFlash_XSPI_CommandSendData(XSPIx, &(NORFlashObject->XSPIObject), 0x60, NULL, 0) != NORFlash_XSPI_OK)
    {
        goto Error;
    }

    if (NORFlash_WaitBusy(XSPIx, NORFlashObject) != NORFlash_OK)
    {
        goto Error;
    }

    return NORFlash_OK;

Error:
    return NORFlash_ERROR;
}

NORFlash_StatusTypeDef NORFlash_EraseSector(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject, uint32_t Address, uint32_t Size)
{
    while (Size != 0)
    {
        if ((Size >= NORFlashObject->Information.SectorSize) && ((Address % NORFlashObject->Information.SectorSize) == 0))
        {
            if (NORFlash_WaitBusy(XSPIx, NORFlashObject) != NORFlash_OK)
            {
                goto Error;
            }

            if (NORFlash_EnableWrite(XSPIx, NORFlashObject) != NORFlash_OK)
            {
                goto Error;
            }

            if (NORFlash_XSPI_CommandSendAddress(XSPIx, &(NORFlashObject->XSPIObject), NORFlashObject->Command.EraseSector.Command, Address) != NORFlash_XSPI_OK)
            {
                goto Error;
            }

            if (NORFlash_WaitBusy(XSPIx, NORFlashObject) != NORFlash_OK)
            {
                goto Error;
            }

            Address += NORFlashObject->Information.SectorSize;
            if (Size >= NORFlashObject->Information.SectorSize)
            {
                Size -= NORFlashObject->Information.SectorSize;
            }
            else
            {
                Size = 0;
            }
        }
        else
        {
            goto Error;
        }
    }

    return NORFlash_OK;

Error:
    return NORFlash_ERROR;
}

NORFlash_StatusTypeDef NORFlash_Write(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject, uint32_t Address, const uint8_t *Data, uint32_t DataSize)
{
    uint8_t Misalignment = 0;
    uint32_t WriteSize;

    if ((Address % NORFlashObject->Information.PageSize) != 0)
    {
        Misalignment = 1;
    }

    while (DataSize != 0)
    {
        if (Misalignment != 0)
        {
            WriteSize = NORFlashObject->Information.PageSize - (Address % NORFlashObject->Information.PageSize);
            if (DataSize < WriteSize)
            {
                WriteSize = DataSize;
            }
            Misalignment = 0;
        }
        else
        {
            WriteSize = (DataSize < NORFlashObject->Information.PageSize) ? DataSize : NORFlashObject->Information.PageSize;
        }

        if (NORFlash_WaitBusy(XSPIx, NORFlashObject) != NORFlash_OK)
        {
            goto Error;
        }

        if (NORFlash_EnableWrite(XSPIx, NORFlashObject) != NORFlash_OK)
        {
            goto Error;
        }

        if (NORFlash_XSPI_Write(XSPIx, &(NORFlashObject->XSPIObject), NORFlashObject->Command.ProgramPage.Command, NORFlashObject->Command.ProgramPage.Dummy, Address, Data, WriteSize) != NORFlash_XSPI_OK)
        {
            goto Error;
        }

        DataSize -= WriteSize;
        Address += WriteSize;
        Data += WriteSize;
    }

    if (NORFlash_WaitBusy(XSPIx, NORFlashObject) != NORFlash_OK)
    {
        goto Error;
    }

    return NORFlash_OK;

Error:
    return NORFlash_ERROR;
}

NORFlash_StatusTypeDef NORFlash_GetMemInfo(NORFlash_ObjectTypeDef *NORFlashObject, NORFlash_InformationTypeDef *Information)
{
    if (NORFlashObject->Information.FlashSize == 0)
    {
        goto Error;
    }

    Information->FlashSize = NORFlashObject->Information.FlashSize;
    Information->SectorSize = NORFlashObject->Information.SectorSize;
    Information->PageSize = NORFlashObject->Information.PageSize;

    return NORFlash_OK;

Error:
    return NORFlash_ERROR;
}
