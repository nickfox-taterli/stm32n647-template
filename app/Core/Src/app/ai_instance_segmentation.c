#include "ai_instance_segmentation.h"

#include "FreeRTOS.h"
#include "task.h"
#include "serial_console.h"
#include "stm32n6xx_ll_bus.h"
#include "npu_cache.h"
#include "app_postprocess.h"
#include "app_config.h"
#include "stai_network.h"
#include <stdio.h>
#include <string.h>

#define AI_INPUT_BYTES (NN_WIDTH * NN_HEIGHT * NN_BPP)
#define AI_MAX_BOXES AI_YOLOV8_SEG_PP_MAX_BOXES_LIMIT

STAI_NETWORK_CONTEXT_DECLARE(s_network_context, STAI_NETWORK_CONTEXT_SIZE);
static TaskHandle_t s_ai_task;
static volatile uint32_t s_ai_buffer_busy;
static uint8_t *s_input;
static iseg_pp_outBuffer_t s_boxes[AI_MAX_BOXES];
static volatile uint32_t s_box_count;
static const char *const s_classes[NN_CLASSES] = NN_CLASSES_TABLE;

static void ai_log(const char *s) { (void)serial_console_write(s, (unsigned short)strlen(s)); }

uint8_t *AIInstanceSegmentation_GetInputBuffer(void)
{
  uint8_t *buffer = NULL;
  taskENTER_CRITICAL();
  if ((s_ai_task != NULL) && (s_ai_buffer_busy == 0U))
  {
    s_ai_buffer_busy = 1U;
    buffer = s_input;
  }
  taskEXIT_CRITICAL();
  return buffer;
}

void AIInstanceSegmentation_SubmitFrame(void)
{
  if (s_ai_task != NULL) xTaskNotifyGive(s_ai_task);
}

void AIInstanceSegmentation_DrawDetections(uint16_t *fb, uint32_t width, uint32_t height)
{
  iseg_pp_outBuffer_t boxes[AI_MAX_BOXES];
  uint32_t count;
  taskENTER_CRITICAL();
  count = s_box_count;
  memcpy(boxes, s_boxes, count * sizeof(boxes[0]));
  taskEXIT_CRITICAL();
  for (uint32_t i = 0; i < count; ++i)
  {
    int32_t x0 = (int32_t)((boxes[i].x_center - boxes[i].width * .5f) * width);
    int32_t y0 = (int32_t)((1.0f - boxes[i].y_center - boxes[i].height * .5f) * height);
    int32_t x1 = (int32_t)((boxes[i].x_center + boxes[i].width * .5f) * width);
    int32_t y1 = (int32_t)((1.0f - boxes[i].y_center + boxes[i].height * .5f) * height);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= (int32_t)width) x1 = (int32_t)width - 1;
    if (y1 >= (int32_t)height) y1 = (int32_t)height - 1;
    if ((x1 <= x0) || (y1 <= y0)) continue;
    for (int32_t t = 0; t < 3; ++t)
    {
      for (int32_t x = x0; x <= x1; ++x)
      { fb[(y0 + t) * width + x] = 0x07e0U; fb[(y1 - t) * width + x] = 0x07e0U; }
      for (int32_t y = y0; y <= y1; ++y)
      { fb[y * width + x0 + t] = 0x07e0U; fb[y * width + x1 - t] = 0x07e0U; }
    }
  }
}

static void ai_npu_init(void)
{
  LL_MEM_EnableClock(LL_MEM_AXISRAM3 | LL_MEM_AXISRAM4 | LL_MEM_AXISRAM5 | LL_MEM_AXISRAM6);
  CLEAR_BIT(RAMCFG_SRAM3_AXI->CR, RAMCFG_CR_SRAMSD);
  CLEAR_BIT(RAMCFG_SRAM4_AXI->CR, RAMCFG_CR_SRAMSD);
  CLEAR_BIT(RAMCFG_SRAM5_AXI->CR, RAMCFG_CR_SRAMSD);
  CLEAR_BIT(RAMCFG_SRAM6_AXI->CR, RAMCFG_CR_SRAMSD);
  LL_AHB5_GRP1_EnableClock(LL_AHB5_GRP1_PERIPH_NPU);
  LL_AHB5_GRP1_ForceReset(LL_AHB5_GRP1_PERIPH_NPU);
  LL_AHB5_GRP1_ReleaseReset(LL_AHB5_GRP1_PERIPH_NPU);
  LL_MEM_EnableClock(LL_MEM_CACHEAXIRAM);
  LL_AHB5_GRP1_EnableClock(LL_AHB5_GRP1_PERIPH_CACHEAXI);
  LL_AHB5_GRP1_ForceReset(LL_AHB5_GRP1_PERIPH_CACHEAXI);
  LL_AHB5_GRP1_ReleaseReset(LL_AHB5_GRP1_PERIPH_CACHEAXI);
  npu_cache_enable();
}

void AIInstanceSegmentationTask(void *argument)
{
  char line[128];
  stai_network_info info;
  stai_ptr inputs[1] = {0};
  stai_ptr outputs[STAI_NETWORK_OUT_NUM] = {0};
  stai_size n_inputs = 1;
  stai_size n_outputs = STAI_NETWORK_OUT_NUM;
  uint32_t lengths[STAI_NETWORK_OUT_NUM];
  iseg_yolov8_pp_static_param_t pp;
  (void)argument;

  s_ai_task = xTaskGetCurrentTaskHandle();
  ai_npu_init();
  configASSERT(stai_runtime_init() == STAI_SUCCESS);
  configASSERT(stai_network_init(s_network_context) == STAI_SUCCESS);
  configASSERT(stai_network_get_info(s_network_context, &info) == STAI_SUCCESS);
  configASSERT((info.n_inputs == 1U) && (info.n_outputs == STAI_NETWORK_OUT_NUM));
  configASSERT(stai_network_get_inputs(s_network_context, inputs, &n_inputs) == STAI_SUCCESS);
  configASSERT(stai_network_get_outputs(s_network_context, outputs, &n_outputs) == STAI_SUCCESS);
  configASSERT((n_inputs == 1U) && (n_outputs == STAI_NETWORK_OUT_NUM));
  s_input = (uint8_t *)inputs[0];
  configASSERT(info.inputs[0].size_bytes == AI_INPUT_BYTES);
  for (uint32_t i = 0; i < STAI_NETWORK_OUT_NUM; ++i) lengths[i] = info.outputs[i].size_bytes;
  configASSERT(app_postprocess_init(&pp, &info) == AI_ISEG_POSTPROCESS_ERROR_NO);
  ai_log("ai: ready yolov8-seg / STEdgeAI 4\r\n");

  for (;;)
  {
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    SCB_InvalidateDCache_by_Addr((uint32_t *)s_input, AI_INPUT_BYTES);
    configASSERT(stai_network_run(s_network_context, STAI_MODE_SYNC) == STAI_SUCCESS);
    for (uint32_t i = 0; i < STAI_NETWORK_OUT_NUM; ++i)
      SCB_InvalidateDCache_by_Addr((uint32_t *)outputs[i], lengths[i]);
    iseg_pp_out_t result = {0};
    int32_t err = app_postprocess_run((void **)outputs, STAI_NETWORK_OUT_NUM, &result, &pp);
    taskENTER_CRITICAL();
    s_box_count = ((err == 0) && (result.nb_detect > 0))
      ? ((result.nb_detect > AI_MAX_BOXES) ? AI_MAX_BOXES : (uint32_t)result.nb_detect) : 0U;
    memcpy(s_boxes, result.pOutBuff, s_box_count * sizeof(s_boxes[0]));
    taskEXIT_CRITICAL();
    if (s_box_count == 0U)
    { (void)snprintf(line, sizeof(line), "ai: objects=0 pp=%ld\r\n", (long)err); ai_log(line); }
    else for (uint32_t i = 0; i < s_box_count; ++i)
    {
      uint32_t cls = (uint32_t)s_boxes[i].class_index;
      uint32_t confidence = (uint32_t)(s_boxes[i].conf * 1000.0f + 0.5f);
      (void)snprintf(line, sizeof(line), "ai: %s %lu.%03lu\r\n",
        (cls < NN_CLASSES) ? s_classes[cls] : "unknown",
        (unsigned long)(confidence / 1000U), (unsigned long)(confidence % 1000U));
      ai_log(line);
    }
    taskENTER_CRITICAL();
    s_ai_buffer_busy = 0U;
    taskEXIT_CRITICAL();
  }
}
