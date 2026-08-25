#ifndef VENC_RECORDER_H
#define VENC_RECORDER_H

#include <stdint.h>

typedef enum
{
  VENC_RECORDER_IDLE = 0,
  VENC_RECORDER_REQUESTED,
  VENC_RECORDER_RECORDING,
  VENC_RECORDER_COMPLETE,
  VENC_RECORDER_ERROR,
} VencRecorderState;

typedef struct
{
  VencRecorderState state;
  uint32_t frames;
  uint32_t target_frames;
  uint32_t bytes;
  uint32_t elapsed_ms;
  int32_t error;
} VencRecorderStatus;

/* Configure the dedicated encoder arena as non-cacheable before D-cache starts. */
void VencRecorder_MemoryInit(void);

/* Request one fixed ten-second H.264 recording. Returns zero when accepted. */
int VencRecorder_Request(void);
void VencRecorder_GetStatus(VencRecorderStatus *status);

/* Called by the camera task once for every complete Pipe1 RGB565 frame. */
void VencRecorder_ProcessFrame(const uint16_t *rgb565_frame);

#endif /* VENC_RECORDER_H */
