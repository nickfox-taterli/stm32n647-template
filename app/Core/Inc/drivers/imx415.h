#ifndef IMX415_H
#define IMX415_H

#include <stdint.h>

#define IMX415_CHIP_ID  0x0514U

/* Demo configuration programmed by IMX415_InitStream(). */
#define IMX415_DEMO_VMAX_LINES         2250U
#define IMX415_DEMO_EXPOSURE_LINES       600U
#define IMX415_DEMO_ANALOG_GAIN           60U

typedef enum
{
  IMX415_OK = 0,
  IMX415_ERROR = 1,
} IMX415_Status;

typedef enum
{
  IMX415_TEST_PATTERN_OFF = 0,
  IMX415_TEST_PATTERN_HORIZONTAL_COLOR_BAR,
  IMX415_TEST_PATTERN_VERTICAL_COLOR_BAR,
} IMX415_TestPattern;

void IMX415_BusInit(void);
IMX415_Status IMX415_ReadID(uint16_t *chip_id);
IMX415_Status IMX415_InitStream(void);
IMX415_Status IMX415_EnableTestPattern(uint8_t enable);
IMX415_Status IMX415_StartStream(void);
IMX415_Status IMX415_StopStream(void);

#endif /* IMX415_H */
