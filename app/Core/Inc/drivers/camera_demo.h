#ifndef CAMERA_DEMO_H
#define CAMERA_DEMO_H

#include <stdint.h>

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
} CameraDemoStatus;

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
  uint16_t fb_min;
  uint16_t fb_max;
} CameraDemoDebug;

CameraDemoStatus CameraDemo_Init(uint16_t *sensor_id);
CameraDemoStatus CameraDemo_CaptureToLcd(void);
void CameraDemo_GetDebug(CameraDemoDebug *debug);

#endif /* CAMERA_DEMO_H */
