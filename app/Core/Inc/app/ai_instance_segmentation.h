#ifndef AI_INSTANCE_SEGMENTATION_H
#define AI_INSTANCE_SEGMENTATION_H

#include <stdint.h>

void AIInstanceSegmentationTask(void *argument);
uint8_t *AIInstanceSegmentation_GetInputBuffer(void);
void AIInstanceSegmentation_SubmitFrame(void);
/* Submit a frame written by a peripheral DMA engine such as DCMIPP. */
void AIInstanceSegmentation_SubmitCameraFrame(void);
/* Release a claimed input buffer when a peripheral capture is abandoned. */
void AIInstanceSegmentation_CancelFrame(void);
uint8_t AIInstanceSegmentation_IsReady(void);
uint32_t AIInstanceSegmentation_GetResultSequence(void);
uint8_t AIInstanceSegmentation_WaitForResult(uint32_t previous_sequence,
                                             uint32_t timeout_ms);

typedef struct
{
  float x_center;
  float y_center;
  float width;
  float height;
} AIInstanceSegmentationBox;

uint32_t AIInstanceSegmentation_GetBoxes(AIInstanceSegmentationBox *boxes,
                                         uint32_t capacity);

#endif
