#include "camera_demo.h"

#include "FreeRTOS.h"
#include "task.h"

#include <string.h>

#include "imx415.h"
#include "rgb_lcd.h"
#include "ai_instance_segmentation.h"
#include "app_config.h"
#include "stm32n647xx.h"
#include "stm32n6xx_ll_bus.h"
#include "stm32n6xx_ll_rcc.h"

#define CAMERA_SRC_WIDTH             3864U
#define CAMERA_SRC_HEIGHT            2192U
#define CAMERA_PREVIEW_WIDTH         512U
#define CAMERA_PREVIEW_HEIGHT        300U
#define CAMERA_ISP_WIDTH             (CAMERA_SRC_WIDTH / 2U)
#define CAMERA_ISP_HEIGHT            (CAMERA_SRC_HEIGHT / 2U)
#define CAMERA_PREVIEW_PITCH_BYTES   (CAMERA_PREVIEW_WIDTH * sizeof(uint16_t))
#define CAMERA_PREVIEW_BYTES         (CAMERA_PREVIEW_PITCH_BYTES * CAMERA_PREVIEW_HEIGHT)
#define CAMERA_CAPTURE_TIMEOUT_MS    1000U
#define CAMERA_NOTIFY_P1_FRAME       (1UL << 0)
#define CAMERA_NOTIFY_P2_FRAME       (1UL << 1)
#define CAMERA_NOTIFY_P2_OVERRUN     (1UL << 2)
#define CAMERA_NOTIFY_ALL             (CAMERA_NOTIFY_P1_FRAME | \
                                      CAMERA_NOTIFY_P2_FRAME | \
                                      CAMERA_NOTIFY_P2_OVERRUN)
/* Keep the camera scaler synchronized with the generated model input. */
#define CAMERA_AI_WIDTH              NN_WIDTH
#define CAMERA_AI_HEIGHT             NN_HEIGHT
#define CAMERA_AI_PITCH_BYTES        (CAMERA_AI_WIDTH * 3U)
#define CAMERA_AI_CROP_SIZE          CAMERA_ISP_HEIGHT
#define CAMERA_AI_CROP_X             ((CAMERA_ISP_WIDTH - CAMERA_AI_CROP_SIZE) / 2U)
#define CAMERA_AI_HRATIO             ((CAMERA_AI_CROP_SIZE * 8192U) / CAMERA_AI_WIDTH)
#define CAMERA_AI_VRATIO             ((CAMERA_AI_CROP_SIZE * 8192U) / CAMERA_AI_HEIGHT)
#define CAMERA_AI_HDIV               ((CAMERA_AI_WIDTH * 1023U + CAMERA_AI_CROP_SIZE / 2U) / CAMERA_AI_CROP_SIZE)
#define CAMERA_AI_VDIV               ((CAMERA_AI_HEIGHT * 1023U + CAMERA_AI_CROP_SIZE / 2U) / CAMERA_AI_CROP_SIZE)
#define CAMERA_PIPE1_ROI_COUNT       8U
#define CAMERA_PIPE1_ROI_LINE_SIZE   1U /* 2 pixels, encoded as 01. */
/* Boot with the sensor in normal (non-pattern) mode. */
#define CAMERA_USE_TEST_PATTERN      0U

#define DCMIPP_DT_RAW10              0x2BU
#define DCMIPP_CSI_DT_BPP10          3U
#define DCMIPP_CSI_TWO_DATA_LANES    (2U << CSI_LMCFGR_LANENB_Pos)
#define DCMIPP_CSI_DATA_LANE0        1U
#define DCMIPP_CSI_DATA_LANE1        2U
#define DCMIPP_MODE_SNAPSHOT         DCMIPP_P1FCTCR_CPTMODE
#define DCMIPP_SERIAL_MODE           DCMIPP_CMCR_INSEL
#define DCMIPP_PIXEL_PACKER_RGB565   (1U << DCMIPP_P1PPCR_FORMAT_Pos)
/* P1DMCR TYPE encodings, matching the STM32N6 HAL (DCMIPP_RAWBAYER_*). */
#define DCMIPP_RAWBAYER_RGGB         (0U << DCMIPP_P1DMCR_TYPE_Pos)
#define DCMIPP_RAWBAYER_GRBG         (1U << DCMIPP_P1DMCR_TYPE_Pos)
#define DCMIPP_RAWBAYER_GBRG         (2U << DCMIPP_P1DMCR_TYPE_Pos)
#define DCMIPP_RAWBAYER_BGGR         (3U << DCMIPP_P1DMCR_TYPE_Pos)
#define DCMIPP_ISP_HDEC_1_OUT_2      (1U << DCMIPP_P1DECR_HDEC_Pos)
#define DCMIPP_ISP_VDEC_1_OUT_2      (1U << DCMIPP_P1DECR_VDEC_Pos)
#define DCMIPP_DOWNSIZE_HRATIO       ((CAMERA_ISP_WIDTH * 8192U) / CAMERA_PREVIEW_WIDTH)
#define DCMIPP_DOWNSIZE_VRATIO       ((CAMERA_ISP_HEIGHT * 8192U) / CAMERA_PREVIEW_HEIGHT)
#define DCMIPP_DOWNSIZE_HDIV         ((CAMERA_PREVIEW_WIDTH * 1023U + (CAMERA_ISP_WIDTH / 2U)) / CAMERA_ISP_WIDTH)
#define DCMIPP_DOWNSIZE_VDIV         ((CAMERA_PREVIEW_HEIGHT * 1023U + (CAMERA_ISP_HEIGHT / 2U)) / CAMERA_ISP_HEIGHT)

/* Default white-balance gains (warm-tuned), units x1000 (1000 == 1.0x). */
#define CAMERA_WB_DEFAULT_R_X1000    2500U
#define CAMERA_WB_DEFAULT_G_X1000    1000U
#define CAMERA_WB_DEFAULT_B_X1000    1500U

static uint16_t s_camera_preview[CAMERA_PREVIEW_WIDTH * CAMERA_PREVIEW_HEIGHT]
  __attribute__((aligned(32)));
static uint8_t s_ai_capture_armed;
static TaskHandle_t s_camera_task;

static int CameraDemo_ConvertGain(uint16_t gain_x1000, uint8_t *shift, uint8_t *multiplier)
{
  uint32_t scaled = (uint32_t)gain_x1000 * 128U;
  uint8_t s = 0U;

  while ((((scaled + 500U) / 1000U) > 255U) && (s < 3U))
  {
    scaled = (scaled + 1U) / 2U;
    s++;
  }

  uint32_t m = (scaled + 500U) / 1000U;

  if ((m < 1U) || (m > 255U))
  {
    return -1;
  }

  /* gain ~= multiplier / 128 * 2^shift; SHF fields are 3 bits, s is 0..3. */
  *shift = s;
  *multiplier = (uint8_t)m;
  return 0;
}

/* Program the P1 exposure-gain (white balance) registers from x1000 gains.
 * Called once during init; there is no runtime WB control surface. */
static void CameraDemo_ApplyWhiteBalance(uint16_t r_x1000,
                                         uint16_t g_x1000,
                                         uint16_t b_x1000)
{
  uint8_t shift_r;
  uint8_t mult_r;
  uint8_t shift_g;
  uint8_t mult_g;
  uint8_t shift_b;
  uint8_t mult_b;

  (void)CameraDemo_ConvertGain(r_x1000, &shift_r, &mult_r);
  (void)CameraDemo_ConvertGain(g_x1000, &shift_g, &mult_g);
  (void)CameraDemo_ConvertGain(b_x1000, &shift_b, &mult_b);

  MODIFY_REG(DCMIPP->P1EXCR1,
             DCMIPP_P1EXCR1_SHFR | DCMIPP_P1EXCR1_MULTR,
             ((uint32_t)shift_r << DCMIPP_P1EXCR1_SHFR_Pos) |
             ((uint32_t)mult_r << DCMIPP_P1EXCR1_MULTR_Pos));

  MODIFY_REG(DCMIPP->P1EXCR2,
             DCMIPP_P1EXCR2_SHFG | DCMIPP_P1EXCR2_MULTG |
             DCMIPP_P1EXCR2_SHFB | DCMIPP_P1EXCR2_MULTB,
             ((uint32_t)shift_g << DCMIPP_P1EXCR2_SHFG_Pos) |
             ((uint32_t)mult_g << DCMIPP_P1EXCR2_MULTG_Pos) |
             ((uint32_t)shift_b << DCMIPP_P1EXCR2_SHFB_Pos) |
             ((uint32_t)mult_b << DCMIPP_P1EXCR2_MULTB_Pos));

  SET_BIT(DCMIPP->P1EXCR1, DCMIPP_P1EXCR1_ENABLE);
}

static void CameraDemo_CopyPreviewToLcd(void)
{
  uint16_t *fb = RGB_LCD_GetFrameBuffer();

  SCB_InvalidateDCache_by_Addr((uint32_t *)s_camera_preview, sizeof(s_camera_preview));
  /* Keep the preview at its native 512x300 size. The remaining LCD area is
   * initialized to black once; only the sensor/display scan direction is
   * corrected here. Clearing the live framebuffer every frame makes LTDC scan
   * partially black frames and causes severe flicker. */
  for (uint32_t y = 0; y < CAMERA_PREVIEW_HEIGHT; y++)
  {
    /* IMX415 VREVERSE has already corrected the sensor line order. */
    const uint16_t *src = &s_camera_preview[y * CAMERA_PREVIEW_WIDTH];
    uint16_t *dst = &fb[y * RGB_LCD_WIDTH];
    memcpy(dst, src, CAMERA_PREVIEW_PITCH_BYTES);
  }

  RGB_LCD_FlushRows(0U, CAMERA_PREVIEW_HEIGHT);
}

static uint32_t CameraDemo_NormalizedToPreviewX(float normalized)
{
  float source_x = (float)CAMERA_AI_CROP_X +
                   (normalized * (float)CAMERA_AI_CROP_SIZE);

  if (source_x <= 0.0f)
  {
    return 0U;
  }
  if (source_x >= (float)CAMERA_ISP_WIDTH)
  {
    return CAMERA_PREVIEW_WIDTH;
  }
  return (uint32_t)((source_x * (float)CAMERA_PREVIEW_WIDTH /
                     (float)CAMERA_ISP_WIDTH) + 0.5f);
}

static uint32_t CameraDemo_NormalizedToPreviewY(float normalized)
{
  float source_y = normalized * (float)CAMERA_AI_CROP_SIZE;

  if (source_y <= 0.0f)
  {
    return 0U;
  }
  if (source_y >= (float)CAMERA_ISP_HEIGHT)
  {
    return CAMERA_PREVIEW_HEIGHT;
  }
  return (uint32_t)((source_y * (float)CAMERA_PREVIEW_HEIGHT /
                     (float)CAMERA_ISP_HEIGHT) + 0.5f);
}

static void CameraDemo_UpdatePipe1Rois(void)
{
  AIInstanceSegmentationBox boxes[CAMERA_PIPE1_ROI_COUNT];
  volatile uint32_t *roi_registers = &DCMIPP->P1RIxCR1;
  uint32_t box_count = AIInstanceSegmentation_GetBoxes(boxes,
                                                        CAMERA_PIPE1_ROI_COUNT);
  uint32_t common = CAMERA_PIPE1_ROI_LINE_SIZE;

  for (uint32_t i = 0; i < CAMERA_PIPE1_ROI_COUNT; ++i)
  {
    roi_registers[i * 2U] = 0U;
    roi_registers[(i * 2U) + 1U] = 0U;
  }

  if (box_count > CAMERA_PIPE1_ROI_COUNT)
  {
    box_count = CAMERA_PIPE1_ROI_COUNT;
  }

  for (uint32_t i = 0; i < box_count; ++i)
  {
    float x0_normalized = boxes[i].x_center - (boxes[i].width * 0.5f);
    float x1_normalized = boxes[i].x_center + (boxes[i].width * 0.5f);
    float y0_normalized = boxes[i].y_center - (boxes[i].height * 0.5f);
    float y1_normalized = boxes[i].y_center + (boxes[i].height * 0.5f);
    uint32_t x0 = CameraDemo_NormalizedToPreviewX(x0_normalized);
    uint32_t x1 = CameraDemo_NormalizedToPreviewX(x1_normalized);
    uint32_t y0 = CameraDemo_NormalizedToPreviewY(y0_normalized);
    uint32_t y1 = CameraDemo_NormalizedToPreviewY(y1_normalized);

    if (x1 <= x0)
    {
      x1 = (x0 < CAMERA_PREVIEW_WIDTH) ? (x0 + 1U) : x0;
    }
    if (y1 <= y0)
    {
      y1 = (y0 < CAMERA_PREVIEW_HEIGHT) ? (y0 + 1U) : y0;
    }
    if ((x0 >= CAMERA_PREVIEW_WIDTH) || (y0 >= CAMERA_PREVIEW_HEIGHT) ||
        (x1 <= x0) || (y1 <= y0))
    {
      continue;
    }
    if (x1 > CAMERA_PREVIEW_WIDTH)
    {
      x1 = CAMERA_PREVIEW_WIDTH;
    }
    if (y1 > CAMERA_PREVIEW_HEIGHT)
    {
      y1 = CAMERA_PREVIEW_HEIGHT;
    }
    if ((x1 <= x0) || (y1 <= y0))
    {
      continue;
    }

    /* DCMIPP ROI coordinates are in Pipe1's post-downsize 512x300 output.
     * Pipe2 sees a square crop from the half-resolution ISP image; map the
     * detection back through that crop before programming Pipe1. */
    roi_registers[i * 2U] =
      ((x0 << DCMIPP_P1RIxCR1_HSTART_Pos) & DCMIPP_P1RIxCR1_HSTART) |
      ((y0 << DCMIPP_P1RIxCR1_VSTART_Pos) & DCMIPP_P1RIxCR1_VSTART) |
      (3U << DCMIPP_P1RIxCR1_CLG_Pos);
    roi_registers[(i * 2U) + 1U] =
      (((x1 - x0) << DCMIPP_P1RIxCR2_HSIZE_Pos) & DCMIPP_P1RIxCR2_HSIZE) |
      (((y1 - y0) << DCMIPP_P1RIxCR2_VSIZE_Pos) & DCMIPP_P1RIxCR2_VSIZE);
    common |= (1UL << (DCMIPP_P1CMRICR_ROI1EN_Pos + i));
  }

  DCMIPP->P1CMRICR = common;
}

static void CameraDemo_CSI_WritePHYReg(uint32_t reg_msb, uint32_t reg_lsb, uint32_t val)
{
  CSI->PTCR1 |= CSI_PTCR1_TWM;
  CSI->PTCR0 |= CSI_PTCR0_TCKEN;
  CSI->PTCR1 |= CSI_PTCR1_TWM;
  CSI->PTCR0 = 0U;
  CSI->PTCR1 = 0U;
  CSI->PTCR1 |= (reg_msb & 0xFFU);
  CSI->PTCR0 |= CSI_PTCR0_TCKEN;
  CSI->PTCR0 = 0U;
  CSI->PTCR1 |= CSI_PTCR1_TWM;
  CSI->PTCR0 |= CSI_PTCR0_TCKEN;
  CSI->PTCR1 |= CSI_PTCR1_TWM | (reg_lsb & 0xFFU);
  CSI->PTCR0 = 0U;
  CSI->PTCR1 = 0U;
  CSI->PTCR1 |= (val & 0xFFU);
  CSI->PTCR0 |= CSI_PTCR0_TCKEN;
  CSI->PTCR0 = 0U;
}

static void CameraDemo_ClockInit(void)
{
  LL_RCC_IC17_Disable();
  LL_RCC_IC17_SetSource(LL_RCC_ICCLKSOURCE_PLL1);
  LL_RCC_IC17_SetDivider(4U);
  LL_RCC_IC17_Enable();
  LL_RCC_SetDCMIPPClockSource(LL_RCC_DCMIPP_CLKSOURCE_IC17);

  LL_RCC_IC18_Disable();
  LL_RCC_IC18_SetSource(LL_RCC_ICCLKSOURCE_PLL1);
  LL_RCC_IC18_SetDivider(60U);
  LL_RCC_IC18_Enable();

  LL_APB5_GRP1_EnableClock(LL_APB5_GRP1_PERIPH_DCMIPP);
  LL_APB5_GRP1_EnableClock(LL_APB5_GRP1_PERIPH_CSI);
  LL_APB5_GRP1_ForceReset(LL_APB5_GRP1_PERIPH_DCMIPP | LL_APB5_GRP1_PERIPH_CSI);
  LL_APB5_GRP1_ReleaseReset(LL_APB5_GRP1_PERIPH_DCMIPP | LL_APB5_GRP1_PERIPH_CSI);
}

static void CameraDemo_DCMIPPInterruptInit(void)
{
  NVIC_SetPriority(DCMIPP_IRQn,
                   NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 6U, 0U));
  NVIC_ClearPendingIRQ(DCMIPP_IRQn);
  NVIC_EnableIRQ(DCMIPP_IRQn);
}

static CameraDemoStatus CameraDemo_CSIInit(void)
{
  CameraDemo_ClockInit();

  CSI->CR &= ~CSI_CR_CSIEN;
  CSI->LMCFGR = DCMIPP_CSI_TWO_DATA_LANES |
                (DCMIPP_CSI_DATA_LANE0 << CSI_LMCFGR_DL0MAP_Pos) |
                (DCMIPP_CSI_DATA_LANE1 << CSI_LMCFGR_DL1MAP_Pos);
  CSI->CR |= CSI_CR_CSIEN;

  CSI->VC0CFGR1 = (DCMIPP_CSI_DT_BPP10 << CSI_VC0CFGR1_CDTFT_Pos) |
                  CSI_VC0CFGR1_ALLDT;

  CSI->PRCR &= ~CSI_PRCR_PEN;
  CSI->PCR = 0U;
  CSI->PTCR0 |= CSI_PTCR0_TCKEN;
  vTaskDelay(pdMS_TO_TICKS(1));
  CSI->PTCR0 = 0U;

  CSI->PFCR = (0x28U << CSI_PFCR_CCFR_Pos) | (0x29U << CSI_PFCR_HSFR_Pos);
  CameraDemo_CSI_WritePHYReg(0x00U, 0x08U, 0x38U);
  CameraDemo_CSI_WritePHYReg(0x00U, 0xE4U, 0x11U);
  CameraDemo_CSI_WritePHYReg(0x00U, 0xE3U, 460U >> 8);
  CameraDemo_CSI_WritePHYReg(0x00U, 0xE3U, 460U & 0xFFU);
  CSI->PFCR = (0x28U << CSI_PFCR_CCFR_Pos) |
              (0x29U << CSI_PFCR_HSFR_Pos) |
              CSI_PFCR_DLD;
  CSI->PCR = CSI_PCR_DL0EN | CSI_PCR_DL1EN | CSI_PCR_CLEN | CSI_PCR_PWRDOWN;
  CSI->PRCR |= CSI_PRCR_PEN;
  CSI->PMCR = 0U;

  DCMIPP->CMCR |= DCMIPP_SERIAL_MODE;
  DCMIPP->P1FSCR = DCMIPP_DT_RAW10;
  DCMIPP->P1FCTCR = 0U;
  DCMIPP->P1PPCR = DCMIPP_PIXEL_PACKER_RGB565;
  DCMIPP->P1PPM0PR = CAMERA_PREVIEW_PITCH_BYTES << DCMIPP_P1PPM0PR_PITCH_Pos;
  /*
   * 3864 pixels is too wide for direct demosaic. Feed the ISP with a
   * half-resolution Bayer frame before final preview downsize.
   */
  DCMIPP->P1DECR = DCMIPP_P1DECR_ENABLE |
                   DCMIPP_ISP_HDEC_1_OUT_2 |
                   DCMIPP_ISP_VDEC_1_OUT_2;
  DCMIPP->P1DSCR = (DCMIPP_DOWNSIZE_HDIV << DCMIPP_P1DSCR_HDIV_Pos) |
                   (DCMIPP_DOWNSIZE_VDIV << DCMIPP_P1DSCR_VDIV_Pos) |
                   DCMIPP_P1DSCR_ENABLE;
  DCMIPP->P1DSRTIOR = (DCMIPP_DOWNSIZE_HRATIO << DCMIPP_P1DSRTIOR_HRATIO_Pos) |
                      (DCMIPP_DOWNSIZE_VRATIO << DCMIPP_P1DSRTIOR_VRATIO_Pos);
  DCMIPP->P1DSSZR = (CAMERA_PREVIEW_WIDTH << DCMIPP_P1DSSZR_HSIZE_Pos) |
                    (CAMERA_PREVIEW_HEIGHT << DCMIPP_P1DSSZR_VSIZE_Pos);
  /* Keep the Bayer phase validated with this IMX415 module. VREVERSE changes
   * line order on this board but does not require changing this ISP setting. */
  DCMIPP->P1DMCR = DCMIPP_P1DMCR_ENABLE | DCMIPP_RAWBAYER_GBRG;
  DCMIPP->P1FCR = DCMIPP_P1FCR_CFRAMEF | DCMIPP_P1FCR_COVRF |
                  DCMIPP_P1FCR_CVSYNCF;
  DCMIPP->P1IER = DCMIPP_P1IER_FRAMEIE;
  CameraDemo_DCMIPPInterruptInit();

  /* Pipe2 branches after Pipe1 ISP. Crop the IMX415 image to a centered
   * square, resize in hardware and write packed RGB888 straight into the
   * network input buffer. */
  DCMIPP->P2FSCR = DCMIPP_DT_RAW10;
  DCMIPP->P2FCTCR = 0U;
  DCMIPP->P2CRSTR = (CAMERA_AI_CROP_X << DCMIPP_P2CRSTR_HSTART_Pos);
  DCMIPP->P2CRSZR = (CAMERA_AI_CROP_SIZE << DCMIPP_P2CRSZR_HSIZE_Pos) |
                    (CAMERA_AI_CROP_SIZE << DCMIPP_P2CRSZR_VSIZE_Pos) |
                    DCMIPP_P2CRSZR_ENABLE;
  DCMIPP->P2DSCR = (CAMERA_AI_HDIV << DCMIPP_P2DSCR_HDIV_Pos) |
                   (CAMERA_AI_VDIV << DCMIPP_P2DSCR_VDIV_Pos) |
                   DCMIPP_P2DSCR_ENABLE;
  DCMIPP->P2DSRTIOR = (CAMERA_AI_HRATIO << DCMIPP_P2DSRTIOR_HRATIO_Pos) |
                      (CAMERA_AI_VRATIO << DCMIPP_P2DSRTIOR_VRATIO_Pos);
  DCMIPP->P2DSSZR = (CAMERA_AI_WIDTH << DCMIPP_P2DSSZR_HSIZE_Pos) |
                    (CAMERA_AI_HEIGHT << DCMIPP_P2DSSZR_VSIZE_Pos);
  DCMIPP->P2PPCR = 0U; /* RGB888, one memory plane. */
  DCMIPP->P2PPM0PR = CAMERA_AI_PITCH_BYTES;
  DCMIPP->P2FCR = DCMIPP_P2FCR_CFRAMEF | DCMIPP_P2FCR_COVRF |
                  DCMIPP_P2FCR_CVSYNCF;
  DCMIPP->P2IER = DCMIPP_P2IER_FRAMEIE | DCMIPP_P2IER_OVRIE;

  /* Program the default white-balance gains once; the runtime control surface
   * has been removed, so these are the only values ever written here. */
  CameraDemo_ApplyWhiteBalance(CAMERA_WB_DEFAULT_R_X1000,
                               CAMERA_WB_DEFAULT_G_X1000,
                               CAMERA_WB_DEFAULT_B_X1000);
  return CAMERA_DEMO_OK;
}

static void CameraDemo_StartPipe1(void)
{
  uint8_t *ai_input = AIInstanceSegmentation_GetInputBuffer();
  uint32_t stale_notifications;

  /* P1/P2 completion is delivered as event bits. Discard anything left by a
   * previous snapshot before arming this one. */
  (void)xTaskNotifyWait(CAMERA_NOTIFY_ALL, CAMERA_NOTIFY_ALL,
                        &stale_notifications, 0U);

  SCB_CleanInvalidateDCache_by_Addr((uint32_t *)s_camera_preview, sizeof(s_camera_preview));
  CameraDemo_UpdatePipe1Rois();
  DCMIPP->P1FCR = DCMIPP_P1FCR_CFRAMEF | DCMIPP_P1FCR_COVRF |
                  DCMIPP_P1FCR_CVSYNCF;
  DCMIPP->P1PPM0AR1 = (uint32_t)s_camera_preview;
  DCMIPP->P1FCTCR = DCMIPP_MODE_SNAPSHOT;
  DCMIPP->P1FSCR = DCMIPP_DT_RAW10 | DCMIPP_P1FSCR_PIPEN;
  s_ai_capture_armed = (ai_input != NULL) ? 1U : 0U;
  if (s_ai_capture_armed != 0U)
  {
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)ai_input,
                                      CAMERA_AI_PITCH_BYTES * CAMERA_AI_HEIGHT);
    DCMIPP->P2FCR = DCMIPP_P2FCR_CFRAMEF | DCMIPP_P2FCR_COVRF |
                    DCMIPP_P2FCR_CVSYNCF;
    DCMIPP->P2PPM0AR1 = (uint32_t)ai_input;
    DCMIPP->P2FCTCR = DCMIPP_P2FCTCR_CPTMODE;
    DCMIPP->P2FSCR = DCMIPP_DT_RAW10 | DCMIPP_P2FSCR_PIPEN;
    DCMIPP->P2FCTCR |= DCMIPP_P2FCTCR_CPTREQ;
  }
  /* Arm both pipes before the next CSI frame arrives. */
  DCMIPP->P1FCTCR |= DCMIPP_P1FCTCR_CPTREQ;
}

static CameraDemoStatus CameraDemo_WaitPipe1Frame(void)
{
  uint32_t events = 0U;
  uint32_t needed = CAMERA_NOTIFY_P1_FRAME;
  TickType_t start = xTaskGetTickCount();

  if (s_ai_capture_armed != 0U)
  {
    needed |= CAMERA_NOTIFY_P2_FRAME;
  }

  while ((events & needed) != needed)
  {
    TickType_t elapsed = xTaskGetTickCount() - start;
    TickType_t remaining = pdMS_TO_TICKS(CAMERA_CAPTURE_TIMEOUT_MS);
    uint32_t notification;

    if (elapsed >= remaining)
    {
      if (s_ai_capture_armed != 0U)
      {
        AIInstanceSegmentation_CancelFrame();
        s_ai_capture_armed = 0U;
      }
      if ((events & CAMERA_NOTIFY_P2_OVERRUN) != 0U)
      {
        return CAMERA_DEMO_OVERRUN;
      }
      return CAMERA_DEMO_FRAME_TIMEOUT;
    }

    remaining -= elapsed;
    if (xTaskNotifyWait(0U, CAMERA_NOTIFY_ALL, &notification, remaining) == pdFALSE)
    {
      if (s_ai_capture_armed != 0U)
      {
        AIInstanceSegmentation_CancelFrame();
        s_ai_capture_armed = 0U;
      }
      return CAMERA_DEMO_FRAME_TIMEOUT;
    }
    events |= notification;

    if ((events & CAMERA_NOTIFY_P2_OVERRUN) != 0U)
    {
      if (s_ai_capture_armed != 0U)
      {
        AIInstanceSegmentation_CancelFrame();
        s_ai_capture_armed = 0U;
      }
      return CAMERA_DEMO_OVERRUN;
    }
  }

  CameraDemo_CopyPreviewToLcd();
  if ((s_ai_capture_armed != 0U) &&
      ((events & CAMERA_NOTIFY_P2_FRAME) != 0U))
  {
    AIInstanceSegmentation_SubmitCameraFrame();
  }
  return CAMERA_DEMO_OK;
}

CameraDemoStatus CameraDemo_Init(uint16_t *sensor_id)
{
  s_camera_task = xTaskGetCurrentTaskHandle();

  IMX415_BusInit();
  if (IMX415_ReadID(sensor_id) != IMX415_OK)
  {
    return CAMERA_DEMO_SENSOR_ID_ERROR;
  }
  if ((*sensor_id & 0x0FFFU) != IMX415_CHIP_ID)
  {
    return CAMERA_DEMO_SENSOR_ID_ERROR;
  }
  if (IMX415_InitStream() != IMX415_OK)
  {
    return CAMERA_DEMO_SENSOR_INIT_ERROR;
  }
  if (IMX415_EnableTestPattern(CAMERA_USE_TEST_PATTERN) != IMX415_OK)
  {
    return CAMERA_DEMO_SENSOR_INIT_ERROR;
  }
  if (CameraDemo_CSIInit() != CAMERA_DEMO_OK)
  {
    return CAMERA_DEMO_CSI_INIT_ERROR;
  }
  if (IMX415_StartStream() != IMX415_OK)
  {
    return CAMERA_DEMO_SENSOR_STREAM_ERROR;
  }

  CSI->CR |= CSI_CR_VC0START;
  TickType_t start = xTaskGetTickCount();
  while ((CSI->SR0 & CSI_SR0_VC0STATEF) == 0U)
  {
    if ((xTaskGetTickCount() - start) > pdMS_TO_TICKS(CAMERA_CAPTURE_TIMEOUT_MS))
    {
      return CAMERA_DEMO_CSI_WAIT_TIMEOUT;
    }
  }
  return CAMERA_DEMO_OK;
}

void CameraDemo_DCMIPP_IRQHandler(void)
{
  BaseType_t higher_priority_task_woken = pdFALSE;
  uint32_t status1 = DCMIPP->P1SR;
  uint32_t status2 = DCMIPP->P2SR;
  uint32_t events = 0U;

  if (((status1 & DCMIPP_P1SR_FRAMEF) != 0U) &&
      ((DCMIPP->P1IER & DCMIPP_P1IER_FRAMEIE) != 0U))
  {
    DCMIPP->P1FCR = DCMIPP_P1FCR_CFRAMEF;
    events |= CAMERA_NOTIFY_P1_FRAME;
  }

  if (((status2 & DCMIPP_P2SR_FRAMEF) != 0U) &&
      ((DCMIPP->P2IER & DCMIPP_P2IER_FRAMEIE) != 0U))
  {
    DCMIPP->P2FCR = DCMIPP_P2FCR_CFRAMEF;
    events |= CAMERA_NOTIFY_P2_FRAME;
  }

  if (((status2 & DCMIPP_P2SR_OVRF) != 0U) &&
      ((DCMIPP->P2IER & DCMIPP_P2IER_OVRIE) != 0U))
  {
    DCMIPP->P2FCR = DCMIPP_P2FCR_COVRF;
    events |= CAMERA_NOTIFY_P2_OVERRUN;
  }

  if ((events != 0U) && (s_camera_task != NULL))
  {
    (void)xTaskNotifyFromISR(s_camera_task, events, eSetBits,
                             &higher_priority_task_woken);
  }

  portYIELD_FROM_ISR(higher_priority_task_woken);
}

CameraDemoStatus CameraDemo_CaptureToLcd(void)
{
  CameraDemo_StartPipe1();
  return CameraDemo_WaitPipe1Frame();
}

const uint16_t *CameraDemo_GetPreviewBuffer(void)
{
  return s_camera_preview;
}
