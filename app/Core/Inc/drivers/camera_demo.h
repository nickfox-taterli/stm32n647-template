#ifndef CAMERA_DEMO_H
#define CAMERA_DEMO_H

#include <stdint.h>

#include "imx415.h"

typedef enum
{
  CAMERA_DEMO_OK = 0,
  CAMERA_DEMO_SENSOR_ID_ERROR,
  CAMERA_DEMO_SENSOR_INIT_ERROR,
  CAMERA_DEMO_SENSOR_STREAM_ERROR,
  CAMERA_DEMO_CSI_INIT_ERROR,
  CAMERA_DEMO_CSI_WAIT_TIMEOUT,
  CAMERA_DEMO_FRAME_TIMEOUT,
  CAMERA_DEMO_OVERRUN,
  CAMERA_DEMO_INVALID_ARGUMENT,
  CAMERA_DEMO_CONTROL_APPLY_ERROR,
} CameraDemoStatus;

typedef enum
{
  CAMERA_BAYER_RGGB = 0,
  CAMERA_BAYER_GRBG,
  CAMERA_BAYER_GBRG,
  CAMERA_BAYER_BGGR,
} CameraBayerPattern;

/* Software view of every runtime-controllable knob. `active` is what the
 * hardware was last successfully programmed with, `pending` is what the shell
 * has most recently requested and is applied between two snapshots. */
typedef struct
{
  uint32_t exposure_lines;
  uint16_t analog_gain;

  uint16_t wb_r_x1000;
  uint16_t wb_g_x1000;
  uint16_t wb_b_x1000;

  CameraBayerPattern bayer;
  IMX415_TestPattern test_pattern;

  uint8_t swap_rb;
} CameraDemoControls;

typedef struct
{
  uint32_t csi_sr0;
  uint32_t csi_err1;
  uint32_t csi_err2;
  uint32_t dcmipp_p1sr;
  uint32_t dcmipp_p1fscr;
  uint32_t dcmipp_p1ppm0ar1;
  uint32_t dcmipp_p1dmcr;
  uint32_t dcmipp_p1ppcr;
  uint32_t dcmipp_p1decr;
  uint32_t dcmipp_p1excr1;
  uint32_t dcmipp_p1excr2;
  uint16_t fb_min;
  uint16_t fb_max;
} CameraDemoDebug;

CameraDemoStatus CameraDemo_Init(uint16_t *sensor_id);
CameraDemoStatus CameraDemo_CaptureToLcd(void);
void CameraDemo_GetDebug(CameraDemoDebug *debug);

/* ---- runtime control API (shell -> pending state, applied by camera task) ---- */
CameraDemoStatus CameraDemo_SetExposureLines(uint32_t exposure_lines);
CameraDemoStatus CameraDemo_SetAnalogGain(uint16_t gain);
CameraDemoStatus CameraDemo_SetWhiteBalance(uint16_t r_x1000,
                                            uint16_t g_x1000,
                                            uint16_t b_x1000);
CameraDemoStatus CameraDemo_SetBayerPattern(CameraBayerPattern pattern);
CameraDemoStatus CameraDemo_SetTestPattern(IMX415_TestPattern pattern);
CameraDemoStatus CameraDemo_SetSwapRB(uint8_t enable);
CameraDemoStatus CameraDemo_RestoreDefaults(void);

void CameraDemo_GetControls(CameraDemoControls *active,
                            CameraDemoControls *pending,
                            uint32_t *dirty_mask);

/* Sensor register read-back (refreshed by the camera task each frame). */
void CameraDemo_GetSensorDebug(IMX415_DebugRegisters *registers);

#endif /* CAMERA_DEMO_H */
