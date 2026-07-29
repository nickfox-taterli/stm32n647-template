#ifndef AI_INSTANCE_SEGMENTATION_H
#define AI_INSTANCE_SEGMENTATION_H

#include <stdint.h>

void AIInstanceSegmentationTask(void *argument);
uint8_t *AIInstanceSegmentation_GetInputBuffer(void);
void AIInstanceSegmentation_SubmitFrame(void);

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
