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
/* Boot with the sensor in normal (non-pattern) mode; TPG is toggled at runtime
 * via `cam tpg`. The pending-controls apply on frame 1 keeps this in sync. */
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

/* White balance gain window, units of x1000 (1000 == 1.0x). */
#define CAMERA_WB_MIN_X1000          250U
#define CAMERA_WB_MAX_X1000          4000U
/* Default white-balance gains (warm-tuned). */
#define CAMERA_WB_DEFAULT_R_X1000    2500U
#define CAMERA_WB_DEFAULT_G_X1000    1000U
#define CAMERA_WB_DEFAULT_B_X1000    1500U

/* Dirty bits tracking which pending controls diverge from the active hardware
 * state. Set by the shell setters, consumed by the camera task. */
#define CAMERA_CTRL_DIRTY_EXPOSURE    (1UL << 0)
#define CAMERA_CTRL_DIRTY_AGAIN       (1UL << 1)
#define CAMERA_CTRL_DIRTY_WB          (1UL << 2)
#define CAMERA_CTRL_DIRTY_BAYER       (1UL << 3)
#define CAMERA_CTRL_DIRTY_TPG         (1UL << 4)
#define CAMERA_CTRL_DIRTY_SWAP_RB     (1UL << 5)
#define CAMERA_CTRL_DIRTY_ALL         0x3FUL

static uint16_t s_camera_preview[CAMERA_PREVIEW_WIDTH * CAMERA_PREVIEW_HEIGHT]
  __attribute__((aligned(32)));
static CameraDemoDebug s_camera_debug;
static uint8_t s_ai_capture_armed;

static CameraDemoControls s_active_controls =
{
  .exposure_lines = IMX415_DEMO_EXPOSURE_LINES,
  .analog_gain = IMX415_DEMO_ANALOG_GAIN,
  .wb_r_x1000 = CAMERA_WB_DEFAULT_R_X1000,
  .wb_g_x1000 = CAMERA_WB_DEFAULT_G_X1000,
  .wb_b_x1000 = CAMERA_WB_DEFAULT_B_X1000,
  .bayer = CAMERA_BAYER_GBRG,
  .test_pattern = IMX415_TEST_PATTERN_OFF,
  .swap_rb = 0U,
};

static CameraDemoControls s_pending_controls =
{
  .exposure_lines = IMX415_DEMO_EXPOSURE_LINES,
  .analog_gain = IMX415_DEMO_ANALOG_GAIN,
  .wb_r_x1000 = CAMERA_WB_DEFAULT_R_X1000,
  .wb_g_x1000 = CAMERA_WB_DEFAULT_G_X1000,
  .wb_b_x1000 = CAMERA_WB_DEFAULT_B_X1000,
  .bayer = CAMERA_BAYER_GBRG,
  .test_pattern = IMX415_TEST_PATTERN_OFF,
  .swap_rb = 0U,
};

/* Starts fully dirty so the very first snapshot programs every default into
 * the hardware, keeping the active cache and the live registers in sync. */
static volatile uint32_t s_control_dirty = CAMERA_CTRL_DIRTY_ALL;

static IMX415_DebugRegisters s_sensor_debug;

static void CameraDemo_UpdateSensorDebug(void)
{
  (void)IMX415_GetDebugRegisters(&s_sensor_debug);
}


static void CameraDemo_UpdateDebug(void)
{
  s_camera_debug.csi_sr0 = CSI->SR0;
  s_camera_debug.csi_err1 = CSI->ERR1;
  s_camera_debug.csi_err2 = CSI->ERR2;
  s_camera_debug.dcmipp_p1sr = DCMIPP->P1SR;
  s_camera_debug.dcmipp_p1fscr = DCMIPP->P1FSCR;
  s_camera_debug.dcmipp_p1ppm0ar1 = DCMIPP->P1PPM0AR1;
  s_camera_debug.dcmipp_p1dmcr = DCMIPP->P1DMCR;
  s_camera_debug.dcmipp_p1ppcr = DCMIPP->P1PPCR;
  s_camera_debug.dcmipp_p1decr = DCMIPP->P1DECR;
  s_camera_debug.dcmipp_p1excr1 = DCMIPP->P1EXCR1;
  s_camera_debug.dcmipp_p1excr2 = DCMIPP->P1EXCR2;
  s_camera_debug.dcmipp_p1cmricr = DCMIPP->P1CMRICR;
  s_camera_debug.dcmipp_p1ri1cr1 = DCMIPP->P1RIxCR1;
  s_camera_debug.dcmipp_p1ri1cr2 = DCMIPP->P1RIxCR2;
}

static void CameraDemo_UpdateFrameBufferStats(void)
{
  uint16_t minv = 0xFFFFU;
  uint16_t maxv = 0U;

  SCB_InvalidateDCache_by_Addr((uint32_t *)s_camera_preview, sizeof(s_camera_preview));
  for (uint32_t i = 0; i < (CAMERA_PREVIEW_WIDTH * CAMERA_PREVIEW_HEIGHT); i += 97U)
  {
    uint16_t v = s_camera_preview[i];

    if (v < minv)
    {
      minv = v;
    }
    if (v > maxv)
    {
      maxv = v;
    }
  }

  s_camera_debug.fb_min = minv;
  s_camera_debug.fb_max = maxv;
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
    /* The panel scan direction is opposite to the IMX415/ISP line order. */
    uint32_t sy = CAMERA_PREVIEW_HEIGHT - 1U - y;
    const uint16_t *src = &s_camera_preview[sy * CAMERA_PREVIEW_WIDTH];
    uint16_t *dst = &fb[y * RGB_LCD_WIDTH];

    if (s_active_controls.swap_rb == 0U)
    {
      memcpy(dst, src, CAMERA_PREVIEW_PITCH_BYTES);
    }
    else
    {
      for (uint32_t x = 0; x < CAMERA_PREVIEW_WIDTH; x++)
      {
        uint16_t pixel = src[x];

        dst[x] = (uint16_t)(((pixel & 0x001FU) << 11) |
                            ( pixel & 0x07E0U)        |
                            ((pixel & 0xF800U) >> 11));
      }
    }
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
     * Pipe2 sees a 320x320 square crop from the half-resolution ISP image;
     * map the detection back through that crop before programming Pipe1. */
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
  DCMIPP->P1DMCR = DCMIPP_P1DMCR_ENABLE | DCMIPP_RAWBAYER_GBRG;
  DCMIPP->P1FCR = DCMIPP_P1FCR_CFRAMEF | DCMIPP_P1FCR_COVRF |
                  DCMIPP_P1FCR_CVSYNCF;

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
  /* The 993 model was exported for the channel order selected by the
   * reference project's enable_swap=1 setting. */
  DCMIPP->P2PPCR = DCMIPP_P2PPCR_SWAPRB; /* RGB888, one memory plane. */
  DCMIPP->P2PPM0PR = CAMERA_AI_PITCH_BYTES;
  DCMIPP->P2FCR = DCMIPP_P2FCR_CFRAMEF | DCMIPP_P2FCR_COVRF |
                  DCMIPP_P2FCR_CVSYNCF;

  return CAMERA_DEMO_OK;
}

static void CameraDemo_StartPipe1(void)
{
  uint8_t *ai_input = AIInstanceSegmentation_GetInputBuffer();

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
  TickType_t start = xTaskGetTickCount();

  while ((DCMIPP->P1SR & DCMIPP_P1SR_FRAMEF) == 0U)
  {
    if ((DCMIPP->P1SR & DCMIPP_P1SR_OVRF) != 0U)
    {
      CameraDemo_UpdateDebug();
      return CAMERA_DEMO_OVERRUN;
    }
    if ((xTaskGetTickCount() - start) > pdMS_TO_TICKS(CAMERA_CAPTURE_TIMEOUT_MS))
    {
      CameraDemo_UpdateDebug();
      return CAMERA_DEMO_FRAME_TIMEOUT;
    }
  }

  CameraDemo_UpdateDebug();
  CameraDemo_UpdateFrameBufferStats();
  /* Refresh the sensor register snapshot here, in the camera task (which owns
   * I2C2), so `cam status` can cross-check the cache against live hardware
   * without the shell task ever touching the I2C bus. */
  CameraDemo_UpdateSensorDebug();
  DCMIPP->P1FCR = DCMIPP_P1FCR_CFRAMEF | DCMIPP_P1FCR_CVSYNCF;
  CameraDemo_CopyPreviewToLcd();
  if ((s_ai_capture_armed != 0U) &&
      ((DCMIPP->P2SR & DCMIPP_P2SR_FRAMEF) != 0U))
  {
    DCMIPP->P2FCR = DCMIPP_P2FCR_CFRAMEF | DCMIPP_P2FCR_CVSYNCF;
    AIInstanceSegmentation_SubmitFrame();
  }
  return CAMERA_DEMO_OK;
}

CameraDemoStatus CameraDemo_Init(uint16_t *sensor_id)
{
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
      CameraDemo_UpdateDebug();
      return CAMERA_DEMO_CSI_WAIT_TIMEOUT;
    }
  }
  CameraDemo_UpdateSensorDebug();
  return CAMERA_DEMO_OK;
}

/* ---- runtime control application (runs in camera task, owns I2C2 + DCMIPP) ---- */

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

static CameraDemoStatus CameraDemo_ApplyWhiteBalance(const CameraDemoControls *controls)
{
  uint8_t shift_r;
  uint8_t mult_r;
  uint8_t shift_g;
  uint8_t mult_g;
  uint8_t shift_b;
  uint8_t mult_b;

  if (CameraDemo_ConvertGain(controls->wb_r_x1000, &shift_r, &mult_r) != 0)
  {
    return CAMERA_DEMO_INVALID_ARGUMENT;
  }
  if (CameraDemo_ConvertGain(controls->wb_g_x1000, &shift_g, &mult_g) != 0)
  {
    return CAMERA_DEMO_INVALID_ARGUMENT;
  }
  if (CameraDemo_ConvertGain(controls->wb_b_x1000, &shift_b, &mult_b) != 0)
  {
    return CAMERA_DEMO_INVALID_ARGUMENT;
  }

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

  return CAMERA_DEMO_OK;
}

static CameraDemoStatus CameraDemo_ApplyBayerPattern(CameraBayerPattern pattern)
{
  uint32_t raw;

  switch (pattern)
  {
    case CAMERA_BAYER_RGGB: raw = DCMIPP_RAWBAYER_RGGB; break;
    case CAMERA_BAYER_GRBG: raw = DCMIPP_RAWBAYER_GRBG; break;
    case CAMERA_BAYER_GBRG: raw = DCMIPP_RAWBAYER_GBRG; break;
    case CAMERA_BAYER_BGGR: raw = DCMIPP_RAWBAYER_BGGR; break;
    default: return CAMERA_DEMO_INVALID_ARGUMENT;
  }

  MODIFY_REG(DCMIPP->P1DMCR, DCMIPP_P1DMCR_TYPE, raw);
  SET_BIT(DCMIPP->P1DMCR, DCMIPP_P1DMCR_ENABLE);

  return CAMERA_DEMO_OK;
}

/* Apply every pending control between two snapshots. All I2C / DCMIPP access
 * happens here, in the camera task. On any failure the not-yet-applied claimed
 * bits are restored so the next frame retries them; nothing is silently lost. */
static CameraDemoStatus CameraDemo_ApplyPendingControls(void)
{
  CameraDemoControls pending;
  uint32_t claimed;
  uint32_t applied = 0U;

  taskENTER_CRITICAL();
  pending = s_pending_controls;
  claimed = s_control_dirty;
  s_control_dirty &= ~claimed;
  taskEXIT_CRITICAL();

  if (claimed == 0U)
  {
    return CAMERA_DEMO_OK;
  }

  if ((claimed & CAMERA_CTRL_DIRTY_TPG) != 0U)
  {
    if (IMX415_SetTestPattern(pending.test_pattern) != IMX415_OK)
    {
      goto apply_failed;
    }
    s_active_controls.test_pattern = pending.test_pattern;
    applied |= CAMERA_CTRL_DIRTY_TPG;
  }

  if ((claimed & CAMERA_CTRL_DIRTY_EXPOSURE) != 0U)
  {
    if (IMX415_SetExposureLines(pending.exposure_lines) != IMX415_OK)
    {
      goto apply_failed;
    }
    s_active_controls.exposure_lines = pending.exposure_lines;
    applied |= CAMERA_CTRL_DIRTY_EXPOSURE;
  }

  if ((claimed & CAMERA_CTRL_DIRTY_AGAIN) != 0U)
  {
    if (IMX415_SetAnalogGain(pending.analog_gain) != IMX415_OK)
    {
      goto apply_failed;
    }
    s_active_controls.analog_gain = pending.analog_gain;
    applied |= CAMERA_CTRL_DIRTY_AGAIN;
  }

  if ((claimed & CAMERA_CTRL_DIRTY_BAYER) != 0U)
  {
    if (CameraDemo_ApplyBayerPattern(pending.bayer) != CAMERA_DEMO_OK)
    {
      goto apply_failed;
    }
    s_active_controls.bayer = pending.bayer;
    applied |= CAMERA_CTRL_DIRTY_BAYER;
  }

  if ((claimed & CAMERA_CTRL_DIRTY_WB) != 0U)
  {
    if (CameraDemo_ApplyWhiteBalance(&pending) != CAMERA_DEMO_OK)
    {
      goto apply_failed;
    }
    s_active_controls.wb_r_x1000 = pending.wb_r_x1000;
    s_active_controls.wb_g_x1000 = pending.wb_g_x1000;
    s_active_controls.wb_b_x1000 = pending.wb_b_x1000;
    applied |= CAMERA_CTRL_DIRTY_WB;
  }

  if ((claimed & CAMERA_CTRL_DIRTY_SWAP_RB) != 0U)
  {
    /* Software-only state, consumed by CopyPreviewToLcd(). Cannot fail. */
    s_active_controls.swap_rb = pending.swap_rb;
    applied |= CAMERA_CTRL_DIRTY_SWAP_RB;
  }

  return CAMERA_DEMO_OK;

apply_failed:
  taskENTER_CRITICAL();
  s_control_dirty |= (claimed & ~applied);
  taskEXIT_CRITICAL();
  return CAMERA_DEMO_CONTROL_APPLY_ERROR;
}

CameraDemoStatus CameraDemo_CaptureToLcd(void)
{
  CameraDemoStatus status = CameraDemo_ApplyPendingControls();
  if (status != CAMERA_DEMO_OK)
  {
    return status;
  }

  CameraDemo_StartPipe1();
  return CameraDemo_WaitPipe1Frame();
}

void CameraDemo_GetDebug(CameraDemoDebug *debug)
{
  if (debug != 0)
  {
    *debug = s_camera_debug;
  }
}

/* ---- shell-facing setters: validate, then update pending + dirty atomically ---- */

CameraDemoStatus CameraDemo_SetExposureLines(uint32_t exposure_lines)
{
  if ((exposure_lines < IMX415_EXPOSURE_MIN_LINES) ||
      (exposure_lines > (IMX415_DEMO_VMAX_LINES - IMX415_EXPOSURE_OFFSET_LINES)))
  {
    return CAMERA_DEMO_INVALID_ARGUMENT;
  }

  taskENTER_CRITICAL();
  s_pending_controls.exposure_lines = exposure_lines;
  s_control_dirty |= CAMERA_CTRL_DIRTY_EXPOSURE;
  taskEXIT_CRITICAL();
  return CAMERA_DEMO_OK;
}

CameraDemoStatus CameraDemo_SetAnalogGain(uint16_t gain)
{
  /* gain is unsigned, so IMX415_ANALOG_GAIN_MIN (0) is the natural floor;
   * only the upper bound is a real constraint here. */
  if (gain > IMX415_ANALOG_GAIN_MAX)
  {
    return CAMERA_DEMO_INVALID_ARGUMENT;
  }

  taskENTER_CRITICAL();
  s_pending_controls.analog_gain = gain;
  s_control_dirty |= CAMERA_CTRL_DIRTY_AGAIN;
  taskEXIT_CRITICAL();
  return CAMERA_DEMO_OK;
}

CameraDemoStatus CameraDemo_SetWhiteBalance(uint16_t r_x1000,
                                            uint16_t g_x1000,
                                            uint16_t b_x1000)
{
  if ((r_x1000 < CAMERA_WB_MIN_X1000) || (r_x1000 > CAMERA_WB_MAX_X1000) ||
      (g_x1000 < CAMERA_WB_MIN_X1000) || (g_x1000 > CAMERA_WB_MAX_X1000) ||
      (b_x1000 < CAMERA_WB_MIN_X1000) || (b_x1000 > CAMERA_WB_MAX_X1000))
  {
    return CAMERA_DEMO_INVALID_ARGUMENT;
  }

  taskENTER_CRITICAL();
  s_pending_controls.wb_r_x1000 = r_x1000;
  s_pending_controls.wb_g_x1000 = g_x1000;
  s_pending_controls.wb_b_x1000 = b_x1000;
  s_control_dirty |= CAMERA_CTRL_DIRTY_WB;
  taskEXIT_CRITICAL();
  return CAMERA_DEMO_OK;
}

CameraDemoStatus CameraDemo_SetBayerPattern(CameraBayerPattern pattern)
{
  if ((pattern != CAMERA_BAYER_RGGB) &&
      (pattern != CAMERA_BAYER_GRBG) &&
      (pattern != CAMERA_BAYER_GBRG) &&
      (pattern != CAMERA_BAYER_BGGR))
  {
    return CAMERA_DEMO_INVALID_ARGUMENT;
  }

  taskENTER_CRITICAL();
  s_pending_controls.bayer = pattern;
  s_control_dirty |= CAMERA_CTRL_DIRTY_BAYER;
  taskEXIT_CRITICAL();
  return CAMERA_DEMO_OK;
}

CameraDemoStatus CameraDemo_SetTestPattern(IMX415_TestPattern pattern)
{
  if ((pattern != IMX415_TEST_PATTERN_OFF) &&
      (pattern != IMX415_TEST_PATTERN_HORIZONTAL_COLOR_BAR) &&
      (pattern != IMX415_TEST_PATTERN_VERTICAL_COLOR_BAR))
  {
    return CAMERA_DEMO_INVALID_ARGUMENT;
  }

  taskENTER_CRITICAL();
  s_pending_controls.test_pattern = pattern;
  s_control_dirty |= CAMERA_CTRL_DIRTY_TPG;
  taskEXIT_CRITICAL();
  return CAMERA_DEMO_OK;
}

CameraDemoStatus CameraDemo_SetSwapRB(uint8_t enable)
{
  taskENTER_CRITICAL();
  s_pending_controls.swap_rb = (enable != 0U) ? 1U : 0U;
  s_control_dirty |= CAMERA_CTRL_DIRTY_SWAP_RB;
  taskEXIT_CRITICAL();
  return CAMERA_DEMO_OK;
}

CameraDemoStatus CameraDemo_RestoreDefaults(void)
{
  taskENTER_CRITICAL();
  s_pending_controls.exposure_lines = IMX415_DEMO_EXPOSURE_LINES;
  s_pending_controls.analog_gain = IMX415_DEMO_ANALOG_GAIN;
  s_pending_controls.wb_r_x1000 = CAMERA_WB_DEFAULT_R_X1000;
  s_pending_controls.wb_g_x1000 = CAMERA_WB_DEFAULT_G_X1000;
  s_pending_controls.wb_b_x1000 = CAMERA_WB_DEFAULT_B_X1000;
  s_pending_controls.bayer = CAMERA_BAYER_GBRG;
  s_pending_controls.test_pattern = IMX415_TEST_PATTERN_OFF;
  s_pending_controls.swap_rb = 0U;
  s_control_dirty |= CAMERA_CTRL_DIRTY_ALL;
  taskEXIT_CRITICAL();
  return CAMERA_DEMO_OK;
}

void CameraDemo_GetControls(CameraDemoControls *active,
                            CameraDemoControls *pending,
                            uint32_t *dirty_mask)
{
  taskENTER_CRITICAL();
  if (active != 0)
  {
    *active = s_active_controls;
  }
  if (pending != 0)
  {
    *pending = s_pending_controls;
  }
  if (dirty_mask != 0)
  {
    *dirty_mask = s_control_dirty;
  }
  taskEXIT_CRITICAL();
}

void CameraDemo_GetSensorDebug(IMX415_DebugRegisters *registers)
{
  if (registers != 0)
  {
    *registers = s_sensor_debug;
  }
}
