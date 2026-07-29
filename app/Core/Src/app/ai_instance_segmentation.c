#include "ai_instance_segmentation.h"

#include "FreeRTOS.h"
#include "task.h"
#include "console_log.h"
#include "stm32n6xx_ll_bus.h"
#include "npu_cache.h"
#include "app_postprocess.h"
#include "app_config.h"
#include "stai_network.h"
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

uint32_t AIInstanceSegmentation_GetBoxes(AIInstanceSegmentationBox *boxes,
                                         uint32_t capacity)
{
  uint32_t copy_count;
  uint32_t count;

  if ((boxes == NULL) || (capacity == 0U))
  {
    return 0U;
  }

  taskENTER_CRITICAL();
  count = s_box_count;
  copy_count = (count < capacity) ? count : capacity;
  for (uint32_t i = 0; i < copy_count; ++i)
  {
    boxes[i].x_center = s_boxes[i].x_center;
    boxes[i].y_center = s_boxes[i].y_center;
    boxes[i].width = s_boxes[i].width;
    boxes[i].height = s_boxes[i].height;
  }
  taskEXIT_CRITICAL();
  return copy_count;
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
  LOG_INFO("ai: ready yolov8-seg / STEdgeAI 4\r\n");

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
    { LOG_WARN("ai: objects=0 pp=%ld\r\n", (long)err); }
    else for (uint32_t i = 0; i < s_box_count; ++i)
    {
      uint32_t cls = (uint32_t)s_boxes[i].class_index;
      uint32_t confidence = (uint32_t)(s_boxes[i].conf * 1000.0f + 0.5f);
      LOG_INFO("ai: %s %lu.%03lu\r\n",
        (cls < NN_CLASSES) ? s_classes[cls] : "unknown",
        (unsigned long)(confidence / 1000U), (unsigned long)(confidence % 1000U));
    }
    taskENTER_CRITICAL();
    s_ai_buffer_busy = 0U;
    taskEXIT_CRITICAL();
  }
}
