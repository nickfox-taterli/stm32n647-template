#include "venc_recorder.h"

#include "FreeRTOS.h"
#include "task.h"
#include "console_log.h"
#include "sdmmc_drv.h"
#include "stm32n647xx.h"
#include "stm32n6xx_ll_bus.h"
#include "stm32n6xx_ll_cortex.h"
#include "stm32n6xx_ll_venc.h"

#include "ewl.h"
#include "h264encapi.h"

#include <stddef.h>
#include <string.h>

#define VENC_WIDTH                 512U
#define VENC_HEIGHT                300U
#define VENC_FPS                   5U
#define VENC_SECONDS               10U
#define VENC_TARGET_FRAMES         (VENC_FPS * VENC_SECONDS)
#define VENC_BITRATE               2000000U
#define VENC_GOP_LENGTH            VENC_FPS
#define VENC_OUTPUT_BUFFER_SIZE    (256U * 1024U)
#define VENC_EWL_ARENA_SIZE        (3U * 1024U * 1024U)
#define VENC_SD_BATCH_BLOCKS       128U
#define VENC_SD_BATCH_BYTES        (VENC_SD_BATCH_BLOCKS * 512U)
#define VENC_DATA_START_LBA        1U

enum
{
  VENC_ERR_NONE = 0,
  VENC_ERR_SD_NOT_READY = -1,
  VENC_ERR_SD_WRITE = -2,
  VENC_ERR_ARENA = -3,
  VENC_ERR_INIT = -4,
  VENC_ERR_CONFIG = -5,
  VENC_ERR_STREAM_START = -6,
  VENC_ERR_ENCODE = -7,
  VENC_ERR_STREAM_END = -8,
  VENC_ERR_NO_SPACE = -9,
};

typedef struct
{
  uint8_t magic[8];
  uint32_t version;
  uint32_t data_lba;
  uint32_t data_bytes;
  uint32_t frames;
  uint32_t width;
  uint32_t height;
  uint32_t fps_num;
  uint32_t fps_den;
  uint32_t elapsed_ms;
  uint32_t bitrate;
} VencDiskMetadata;

extern uint8_t __venc_ram_start__;
extern uint8_t __venc_ram_end__;

static uint8_t s_ewl_arena[VENC_EWL_ARENA_SIZE]
  __attribute__((section(".VENC_RAM"), aligned(32)));
static uint8_t s_output_buffer[VENC_OUTPUT_BUFFER_SIZE]
  __attribute__((section(".VENC_RAM"), aligned(32)));
static uint8_t s_sd_batch[VENC_SD_BATCH_BYTES]
  __attribute__((section(".VENC_RAM"), aligned(32)));
static uint8_t s_metadata_sector[512] __attribute__((aligned(32)));

static size_t s_ewl_used;
static H264EncInst s_encoder;
static H264EncIn s_enc_in;
static H264EncOut s_enc_out;
static uint8_t s_encoder_live;
static uint32_t s_sd_next_lba;
static uint32_t s_sd_batch_used;
static uint32_t s_stream_bytes;
static TickType_t s_start_tick;
static TickType_t s_frame_deadline;
static VencRecorderStatus s_status = {
  .state = VENC_RECORDER_IDLE,
  .target_frames = VENC_TARGET_FRAMES,
};

static void *venc_arena_alloc(size_t size)
{
  size_t start = (s_ewl_used + 7U) & ~(size_t)7U;
  size_t end = start + size;

  if ((end < start) || (end > sizeof(s_ewl_arena)))
  {
    return NULL;
  }
  s_ewl_used = end;
  return &s_ewl_arena[start];
}

/* Strong EWL allocation hooks. The encoder allocates only during init; the
 * arena is reset between sessions, so individual frees intentionally do not
 * reclaim blocks. */
i32 EWLMallocLinear(const void *instance, u32 size, EWLLinearMem_t *info)
{
  void *memory;
  (void)instance;

  if (info == NULL)
  {
    return EWL_ERROR;
  }
  size = (size + 7U) & ~7U;
  memory = venc_arena_alloc(size);
  if (memory == NULL)
  {
    return EWL_ERROR;
  }
  info->virtualAddress = (u32 *)memory;
  info->busAddress = (ptr_t)memory;
  info->size = size;
  return EWL_OK;
}

void EWLFreeLinear(const void *instance, EWLLinearMem_t *info)
{
  (void)instance;
  if (info != NULL)
  {
    info->virtualAddress = NULL;
    info->busAddress = 0U;
    info->size = 0U;
  }
}

i32 EWLMallocRefFrm(const void *instance, u32 size, EWLLinearMem_t *info)
{
  return EWLMallocLinear(instance, size, info);
}

void EWLFreeRefFrm(const void *instance, EWLLinearMem_t *info)
{
  EWLFreeLinear(instance, info);
}

void *EWLmalloc(u32 size)
{
  return venc_arena_alloc(size);
}

void EWLfree(void *memory)
{
  (void)memory;
}

void *EWLcalloc(u32 count, u32 size)
{
  size_t bytes = (size_t)count * size;
  void *memory;

  if ((count != 0U) && ((bytes / count) != size))
  {
    return NULL;
  }
  memory = venc_arena_alloc(bytes);
  if (memory != NULL)
  {
    memset(memory, 0, bytes);
  }
  return memory;
}

void EWLPoolChoiceCb(u8 **pool_ptr, size_t *size)
{
  if (pool_ptr != NULL)
  {
    *pool_ptr = NULL;
  }
  if (size != NULL)
  {
    *size = 0U;
  }
}

void EWLPoolReleaseCb(u8 **pool_ptr)
{
  (void)pool_ptr;
}

static int venc_storage_flush(void)
{
  uint32_t blocks;
  SD_HandleTypeDef *sd = sd_get_handle(2);

  if (s_sd_batch_used == 0U)
  {
    return 0;
  }
  if (sd == NULL)
  {
    return VENC_ERR_SD_NOT_READY;
  }

  blocks = (s_sd_batch_used + 511U) / 512U;
  if ((s_sd_next_lba + blocks) > sd->SdCard.BlockNbr)
  {
    return VENC_ERR_NO_SPACE;
  }
  memset(&s_sd_batch[s_sd_batch_used], 0, blocks * 512U - s_sd_batch_used);
  if (sd_storage_write(s_sd_next_lba, blocks, s_sd_batch) != HAL_OK)
  {
    return VENC_ERR_SD_WRITE;
  }
  s_sd_next_lba += blocks;
  s_sd_batch_used = 0U;
  return 0;
}

static int venc_storage_append(const void *data, uint32_t size)
{
  const uint8_t *source = (const uint8_t *)data;

  while (size != 0U)
  {
    uint32_t space = sizeof(s_sd_batch) - s_sd_batch_used;
    uint32_t chunk = (size < space) ? size : space;

    memcpy(&s_sd_batch[s_sd_batch_used], source, chunk);
    s_sd_batch_used += chunk;
    s_stream_bytes += chunk;
    source += chunk;
    size -= chunk;

    if (s_sd_batch_used == sizeof(s_sd_batch))
    {
      int error = venc_storage_flush();
      if (error != 0)
      {
        return error;
      }
    }
  }
  return 0;
}

static int venc_write_metadata(uint8_t complete)
{
  VencDiskMetadata metadata;

  memset(s_metadata_sector, 0, sizeof(s_metadata_sector));
  if (complete != 0U)
  {
    memset(&metadata, 0, sizeof(metadata));
    memcpy(metadata.magic, "N6VENC1", 8U);
    metadata.version = 1U;
    metadata.data_lba = VENC_DATA_START_LBA;
    metadata.data_bytes = s_stream_bytes;
    metadata.frames = s_status.frames;
    metadata.width = VENC_WIDTH;
    metadata.height = VENC_HEIGHT;
    metadata.fps_num = VENC_FPS;
    metadata.fps_den = 1U;
    metadata.elapsed_ms = s_status.elapsed_ms;
    metadata.bitrate = VENC_BITRATE;
    memcpy(s_metadata_sector, &metadata, sizeof(metadata));
  }
  return (sd_storage_write(0U, 1U, s_metadata_sector) == HAL_OK)
    ? 0 : VENC_ERR_SD_WRITE;
}

static void venc_release_encoder(void)
{
  if (s_encoder_live != 0U)
  {
    (void)H264EncRelease(s_encoder);
    s_encoder_live = 0U;
    s_encoder = NULL;
  }
}

static int venc_encoder_begin(void)
{
  H264EncConfig config;
  H264EncPreProcessingCfg preprocessing;
  H264EncCodingCtrl coding;
  H264EncRateCtrl rate;
  H264EncRet result;
  int error;

  if (!sd_is_ready(2))
  {
    return VENC_ERR_SD_NOT_READY;
  }

  /* Destroy stale MBR/metadata immediately; only completed sessions get magic. */
  error = venc_write_metadata(0U);
  if (error != 0)
  {
    return error;
  }

  LL_APB4_GRP2_EnableClock(LL_APB4_GRP2_PERIPH_SYSCFG);
  CLEAR_BIT(SYSCFG->VENCRAMCR, SYSCFG_VENCRAMCR_VENCRAM_EN);
  LL_VENC_Init();

  s_ewl_used = 0U;
  s_sd_next_lba = VENC_DATA_START_LBA;
  s_sd_batch_used = 0U;
  s_stream_bytes = 0U;
  memset(&s_enc_in, 0, sizeof(s_enc_in));
  memset(&s_enc_out, 0, sizeof(s_enc_out));
  memset(&config, 0, sizeof(config));

  config.streamType = H264ENC_BYTE_STREAM;
  config.viewMode = H264ENC_BASE_VIEW_SINGLE_BUFFER;
  config.level = H264ENC_LEVEL_2_2;
  config.width = VENC_WIDTH;
  config.height = VENC_HEIGHT;
  config.frameRateNum = VENC_FPS;
  config.frameRateDenom = 1U;
  config.refFrameAmount = 1U;

  result = H264EncInit(&config, &s_encoder);
  if (result != H264ENC_OK)
  {
    LOG_ERROR("venc: H264EncInit=%d arena=%lu\r\n",
              (int)result, (unsigned long)s_ewl_used);
    return (s_ewl_used >= sizeof(s_ewl_arena)) ? VENC_ERR_ARENA : VENC_ERR_INIT;
  }
  s_encoder_live = 1U;

  result = H264EncGetPreProcessing(s_encoder, &preprocessing);
  if (result != H264ENC_OK)
  {
    return VENC_ERR_CONFIG;
  }
  preprocessing.origWidth = VENC_WIDTH;
  preprocessing.origHeight = VENC_HEIGHT;
  preprocessing.inputType = H264ENC_RGB565;
  preprocessing.rotation = H264ENC_ROTATE_0;
  preprocessing.videoStabilization = 0U;
  preprocessing.colorConversion.type = H264ENC_RGBTOYUV_BT601;
  result = H264EncSetPreProcessing(s_encoder, &preprocessing);
  if (result != H264ENC_OK)
  {
    return VENC_ERR_CONFIG;
  }

  result = H264EncGetCodingCtrl(s_encoder, &coding);
  if (result != H264ENC_OK)
  {
    return VENC_ERR_CONFIG;
  }
  coding.idrHeader = 1U;
  coding.enableCabac = 0U;
  coding.transform8x8Mode = 0U;
  coding.inputLineBufEn = 0U;
  coding.inputLineBufLoopBackEn = 0U;
  coding.inputLineBufHwModeEn = 0U;
  result = H264EncSetCodingCtrl(s_encoder, &coding);
  if (result != H264ENC_OK)
  {
    return VENC_ERR_CONFIG;
  }

  result = H264EncGetRateCtrl(s_encoder, &rate);
  if (result != H264ENC_OK)
  {
    return VENC_ERR_CONFIG;
  }
  rate.pictureRc = 1U;
  rate.mbRc = 1U;
  rate.pictureSkip = 0U;
  rate.qpHdr = 26;
  rate.qpMin = 10U;
  rate.qpMax = 51U;
  rate.bitPerSecond = VENC_BITRATE;
  rate.hrd = 0U;
  rate.gopLen = VENC_GOP_LENGTH;
  result = H264EncSetRateCtrl(s_encoder, &rate);
  if (result != H264ENC_OK)
  {
    LOG_ERROR("venc: H264EncSetRateCtrl=%d\r\n", (int)result);
    return VENC_ERR_CONFIG;
  }

  s_enc_in.pOutBuf = (u32 *)s_output_buffer;
  s_enc_in.busOutBuf = (size_t)s_output_buffer;
  s_enc_in.outBufSize = sizeof(s_output_buffer);
  result = H264EncStrmStart(s_encoder, &s_enc_in, &s_enc_out);
  if (result != H264ENC_OK)
  {
    return VENC_ERR_STREAM_START;
  }
  error = venc_storage_append(s_enc_in.pOutBuf, s_enc_out.streamSize);
  if (error != 0)
  {
    return error;
  }

  s_start_tick = xTaskGetTickCount();
  s_frame_deadline = s_start_tick;
  LOG_INFO("venc: start 512x300@%lu bitrate=2000000 arena=%lu header=%lu\r\n",
           (unsigned long)VENC_FPS, (unsigned long)s_ewl_used,
           (unsigned long)s_enc_out.streamSize);
  return 0;
}

static int venc_encode_frame(const uint16_t *rgb565_frame)
{
  H264EncRet result;

  s_enc_in.busLuma = (ptr_t)rgb565_frame;
  s_enc_in.busChromaU = 0U;
  s_enc_in.busChromaV = 0U;
  s_enc_in.timeIncrement = (s_status.frames == 0U) ? 0U : 1U;
  s_enc_in.codingType = ((s_status.frames % VENC_GOP_LENGTH) == 0U)
    ? H264ENC_INTRA_FRAME : H264ENC_PREDICTED_FRAME;
  s_enc_in.ipf = H264ENC_REFERENCE_AND_REFRESH;
  s_enc_in.ltrf = H264ENC_NO_REFERENCE_NO_REFRESH;

  result = H264EncStrmEncode(s_encoder, &s_enc_in, &s_enc_out,
                             NULL, NULL, NULL);
  if (result != H264ENC_FRAME_READY)
  {
    LOG_ERROR("venc: frame %lu result=%d hw=0x%08lx\r\n",
              (unsigned long)s_status.frames, (int)result,
              (unsigned long)VENC_REG(1U));
    return VENC_ERR_ENCODE;
  }
  return venc_storage_append(s_enc_in.pOutBuf, s_enc_out.streamSize);
}

static int venc_encoder_finish(void)
{
  H264EncRet result;
  int error;

  result = H264EncStrmEnd(s_encoder, &s_enc_in, &s_enc_out);
  if (result != H264ENC_OK)
  {
    return VENC_ERR_STREAM_END;
  }
  error = venc_storage_append(s_enc_in.pOutBuf, s_enc_out.streamSize);
  if (error == 0)
  {
    error = venc_storage_flush();
  }
  s_status.elapsed_ms = (uint32_t)((xTaskGetTickCount() - s_start_tick) * portTICK_PERIOD_MS);
  s_status.bytes = s_stream_bytes;
  if (error == 0)
  {
    error = venc_write_metadata(1U);
  }
  venc_release_encoder();
  return error;
}

void VencRecorder_MemoryInit(void)
{
  uint32_t attributes = LL_MPU_INSTRUCTION_ACCESS_DISABLE |
                        LL_MPU_ACCESS_NOT_SHAREABLE |
                        LL_MPU_REGION_ALL_RW;
  uint32_t memory_attributes = (LL_MPU_NOT_CACHEABLE << 4U) |
                               LL_MPU_NOT_CACHEABLE;

  LL_MPU_Disable();
  LL_MPU_ConfigAttributes(LL_MPU_ATTRIBUTES_NUMBER0, memory_attributes);
  LL_MPU_ConfigRegion(LL_MPU_REGION_NUMBER0, attributes,
                      LL_MPU_ATTRIBUTES_NUMBER0,
                      (uint32_t)&__venc_ram_start__,
                      (uint32_t)&__venc_ram_end__ - 1U);
  LL_MPU_Enable(LL_MPU_CTRL_PRIVILEGED_DEFAULT);
}

int VencRecorder_Request(void)
{
  int accepted = -1;

  if (!sd_is_ready(2))
  {
    return -1;
  }
  taskENTER_CRITICAL();
  if ((s_status.state == VENC_RECORDER_IDLE) ||
      (s_status.state == VENC_RECORDER_COMPLETE) ||
      (s_status.state == VENC_RECORDER_ERROR))
  {
    s_status.state = VENC_RECORDER_REQUESTED;
    s_status.frames = 0U;
    s_status.target_frames = VENC_TARGET_FRAMES;
    s_status.bytes = 0U;
    s_status.elapsed_ms = 0U;
    s_status.error = VENC_ERR_NONE;
    accepted = 0;
  }
  taskEXIT_CRITICAL();
  return accepted;
}

void VencRecorder_GetStatus(VencRecorderStatus *status)
{
  if (status == NULL)
  {
    return;
  }
  taskENTER_CRITICAL();
  *status = s_status;
  taskEXIT_CRITICAL();
}

void VencRecorder_ProcessFrame(const uint16_t *rgb565_frame)
{
  int error;

  if ((rgb565_frame == NULL) ||
      ((s_status.state != VENC_RECORDER_REQUESTED) &&
       (s_status.state != VENC_RECORDER_RECORDING)))
  {
    return;
  }

  if (s_status.state == VENC_RECORDER_REQUESTED)
  {
    error = venc_encoder_begin();
    if (error != 0)
    {
      venc_release_encoder();
      s_status.error = error;
      s_status.state = VENC_RECORDER_ERROR;
      LOG_ERROR("venc: start failed error=%d arena=%lu\r\n",
                error, (unsigned long)s_ewl_used);
      return;
    }
    s_status.state = VENC_RECORDER_RECORDING;
  }

  error = venc_encode_frame(rgb565_frame);
  if (error != 0)
  {
    venc_release_encoder();
    s_status.error = error;
    s_status.bytes = s_stream_bytes;
    s_status.state = VENC_RECORDER_ERROR;
    LOG_ERROR("venc: stopped error=%d frame=%lu bytes=%lu\r\n",
              error, (unsigned long)s_status.frames,
              (unsigned long)s_stream_bytes);
    return;
  }

  s_status.frames++;
  s_status.bytes = s_stream_bytes;
  if (s_status.frames >= s_status.target_frames)
  {
    error = venc_encoder_finish();
    s_status.error = error;
    s_status.state = (error == 0) ? VENC_RECORDER_COMPLETE : VENC_RECORDER_ERROR;
    LOG_INFO("venc: %s frames=%lu bytes=%lu elapsed=%lums\r\n",
             (error == 0) ? "complete" : "finish failed",
             (unsigned long)s_status.frames,
             (unsigned long)s_status.bytes,
             (unsigned long)s_status.elapsed_ms);
  }
  else
  {
    /* First-pass real-time pacing. Encoding and the next Pipe1 snapshot must
     * fit inside this interval; missed deadlines naturally run without delay. */
    vTaskDelayUntil(&s_frame_deadline, pdMS_TO_TICKS(1000U / VENC_FPS));
  }
}
