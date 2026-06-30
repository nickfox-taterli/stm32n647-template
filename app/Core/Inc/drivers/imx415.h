#ifndef IMX415_H
#define IMX415_H

#include <stdint.h>

#define IMX415_CHIP_ID  0x0514U

/*
 * Demo / debug configuration. Declared here (not only in the .c) so that the
 * camera_demo control layer can validate queued values against the very same
 * window that the sensor driver enforces, without duplicating magic numbers.
 */
#define IMX415_DEMO_VMAX_LINES         2250U
#define IMX415_DEMO_EXPOSURE_LINES       600U
#define IMX415_DEMO_ANALOG_GAIN           60U
#define IMX415_EXPOSURE_MIN_LINES          4U
#define IMX415_EXPOSURE_OFFSET_LINES       8U
#define IMX415_ANALOG_GAIN_MIN             0U
#define IMX415_ANALOG_GAIN_MAX           100U

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

/* Read-back of the key sensor registers, used by `cam status` to cross-check
 * the software cache against what the IMX415 actually reports. Populated by
 * the camera task (which owns I2C2) and only ever read by the shell task. */
typedef struct
{
  uint32_t vmax;      /* 0x3024, 3 bytes */
  uint32_t shr0;      /* 0x3050, 3 bytes */
  uint16_t gain;      /* 0x3090, 2 bytes */
  uint16_t blklevel;  /* 0x30E2, 2 bytes */
  uint8_t  tpg_en;    /* 0x30E4, 1 byte  */
  uint8_t  tpg_sel;   /* 0x30E6, 1 byte  */
  IMX415_Status read_status; /* IMX415_OK only if every register read succeeded */
} IMX415_DebugRegisters;

void IMX415_BusInit(void);
IMX415_Status IMX415_ReadID(uint16_t *chip_id);
IMX415_Status IMX415_InitStream(void);
IMX415_Status IMX415_EnableTestPattern(uint8_t enable);
IMX415_Status IMX415_StartStream(void);
IMX415_Status IMX415_StopStream(void);

IMX415_Status IMX415_SetExposureLines(uint32_t exposure_lines);
IMX415_Status IMX415_SetAnalogGain(uint16_t gain);
IMX415_Status IMX415_SetTestPattern(IMX415_TestPattern pattern);

IMX415_Status IMX415_GetExposureLines(uint32_t *exposure_lines);
IMX415_Status IMX415_GetAnalogGain(uint16_t *gain);
IMX415_Status IMX415_GetTestPattern(IMX415_TestPattern *pattern);

IMX415_Status IMX415_GetDebugRegisters(IMX415_DebugRegisters *registers);

#endif /* IMX415_H */
