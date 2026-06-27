#ifndef IMX415_H
#define IMX415_H

#include <stdint.h>

#define IMX415_CHIP_ID  0x0514U

typedef enum
{
  IMX415_OK = 0,
  IMX415_ERROR = 1,
} IMX415_Status;

void IMX415_BusInit(void);
IMX415_Status IMX415_ReadID(uint16_t *chip_id);
IMX415_Status IMX415_InitStream(void);
IMX415_Status IMX415_EnableTestPattern(uint8_t enable);
IMX415_Status IMX415_StartStream(void);
IMX415_Status IMX415_StopStream(void);

#endif /* IMX415_H */
