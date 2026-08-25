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
} CameraDemoStatus;

CameraDemoStatus CameraDemo_Init(uint16_t *sensor_id);
CameraDemoStatus CameraDemo_CaptureToLcd(void);
const uint16_t *CameraDemo_GetPreviewBuffer(void);
void CameraDemo_DCMIPP_IRQHandler(void);

#endif /* CAMERA_DEMO_H */
