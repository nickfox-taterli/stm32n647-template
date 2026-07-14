#ifndef AI_INSTANCE_SEGMENTATION_H
#define AI_INSTANCE_SEGMENTATION_H

#include <stdint.h>

void AIInstanceSegmentationTask(void *argument);
uint8_t *AIInstanceSegmentation_GetInputBuffer(void);
void AIInstanceSegmentation_SubmitFrame(void);
void AIInstanceSegmentation_DrawDetections(uint16_t *framebuffer, uint32_t width, uint32_t height);

#endif
