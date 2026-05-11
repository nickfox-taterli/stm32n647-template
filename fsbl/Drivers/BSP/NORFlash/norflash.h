#ifndef __NORFLASH_H
#define __NORFLASH_H

#include "stm32n6xx_ll_xspi.h"

/* XSPI-level status */
typedef enum
{
    NORFlash_XSPI_OK,
    NORFlash_XSPI_ERROR,
} NORFlash_XSPI_StatusTypeDef;

/* XSPI physical link mode */
typedef enum {
    NORFlash_PHY_LINK_1S1S1S,
    NORFlash_PHY_LINK_4S4S4S,
    NORFlash_PHY_LINK_8D8D8D,
} NORFlash_XSPI_PhysicalLinkTypeDef;

/* XSPI object: config only, no hardware pointer */
typedef struct
{
    uint32_t MemoryMode;
    uint32_t SampleShifting;
    uint32_t MemoryType;
    XSPI_RegularCmdTypeDef BaseCommand;
    NORFlash_XSPI_PhysicalLinkTypeDef PhyLink;
} NORFlash_XSPI_ObjectTypeDef;

/* High-level NOR flash status */
typedef enum
{
    NORFlash_OK,
    NORFlash_ERROR,
} NORFlash_StatusTypeDef;

/* NOR flash geometry */
typedef struct
{
    uint32_t FlashSize;
    uint32_t SectorSize;
    uint32_t PageSize;
} NORFlash_InformationTypeDef;

/* Command + dummy pair */
typedef struct
{
    uint8_t Command;
    uint8_t Dummy;
} NORFlash_CommandTypeDef;

/* Top-level NOR flash object */
typedef struct
{
    NORFlash_XSPI_ObjectTypeDef XSPIObject;

    NORFlash_InformationTypeDef Information;

    struct
    {
        uint32_t EraseChip;
        uint32_t EraseSector;
        uint32_t ProgramPage;
    } Timing;

    struct
    {
        NORFlash_CommandTypeDef MapRead;
        NORFlash_CommandTypeDef MapWrite;
        NORFlash_CommandTypeDef ProgramPage;
        NORFlash_CommandTypeDef EraseSector;
    } Command;
} NORFlash_ObjectTypeDef;

/* MX25UM25645G JEDEC ID */
#define NORFlash_MX25UM25645G_JEDECID_0 0xC2
#define NORFlash_MX25UM25645G_JEDECID_1 0x80
#define NORFlash_MX25UM25645G_JEDECID_2 0x39

#define NORFlash_IS_MX25UM25645G(_jedecid_) ((_jedecid_[0] == NORFlash_MX25UM25645G_JEDECID_0) && (_jedecid_[1] == NORFlash_MX25UM25645G_JEDECID_1) && (_jedecid_[2] == NORFlash_MX25UM25645G_JEDECID_2))

/* XSPI-level functions (XSPIx passed directly) */
void NORFlash_XSPI_Init(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject,
                         uint32_t MemoryMode, uint32_t SampleShifting, uint32_t MemoryType);
NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_SetClock(XSPI_TypeDef *XSPIx, uint32_t ClockInput, uint32_t ClockRequested, uint32_t *ClockReal);
NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_EnableMapMode(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t CommandRead, uint8_t DummyRead, uint8_t CommandWrite, uint8_t DummyWrite);
NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_DisableMapMode(XSPI_TypeDef *XSPIx);
NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_ConfigPHYLink(NORFlash_XSPI_ObjectTypeDef *XSPIObject, NORFlash_XSPI_PhysicalLinkTypeDef PhyLink);
NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_CommandSendData(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t Command, uint8_t *Data, uint16_t DataSize);
NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_CommandSendAddress(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t Command, uint32_t Address);
NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_CommandRead(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t Command, uint8_t *Data, uint16_t DataSize);
NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_CommandSendAddressReadData(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t Command, uint32_t Address, uint8_t *Data, uint16_t DataSize);
NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_CommandSendAddressSendData(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t Command, uint32_t Address, uint8_t *Data, uint16_t DataSize);
NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_WaitStatusRegister(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t Command, uint16_t MatchValue, uint16_t MatchMask, uint8_t DataSize);
NORFlash_XSPI_StatusTypeDef NORFlash_XSPI_Write(XSPI_TypeDef *XSPIx, NORFlash_XSPI_ObjectTypeDef *XSPIObject, uint8_t Command, uint8_t Dummy, uint32_t Address, const uint8_t *Data, uint32_t DataSize);

/* High-level NOR flash API */
NORFlash_StatusTypeDef NORFlash_Init(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject,
                                      uint32_t MemoryMode, uint32_t SampleShifting, uint32_t MemoryType,
                                      uint32_t ClockInput);
void NORFlash_Reset(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject);
NORFlash_StatusTypeDef NORFlash_WaitBusy(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject);
NORFlash_StatusTypeDef NORFlash_EnableWrite(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject);
NORFlash_StatusTypeDef NORFlash_EnableMemoryMappedMode(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject);
NORFlash_StatusTypeDef NORFlash_DisableMemoryMappedMode(XSPI_TypeDef *XSPIx);
NORFlash_StatusTypeDef NORFlash_GetMemoryMappedAddress(XSPI_TypeDef *XSPIx, uint32_t *BaseAddress);
NORFlash_StatusTypeDef NORFlash_EraseChip(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject);
NORFlash_StatusTypeDef NORFlash_EraseSector(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject, uint32_t Address, uint32_t Size);
NORFlash_StatusTypeDef NORFlash_Write(XSPI_TypeDef *XSPIx, NORFlash_ObjectTypeDef *NORFlashObject, uint32_t Address, const uint8_t *Data, uint32_t DataSize);
NORFlash_StatusTypeDef NORFlash_GetMemInfo(NORFlash_ObjectTypeDef *NORFlashObject, NORFlash_InformationTypeDef *Information);

#endif
