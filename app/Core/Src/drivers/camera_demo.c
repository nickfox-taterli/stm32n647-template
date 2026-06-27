#include "camera_demo.h"

#include "FreeRTOS.h"
#include "task.h"

#include "imx415.h"
#include "rgb_lcd.h"
#include "stm32n647xx.h"
#include "stm32n6xx_ll_bus.h"
#include "stm32n6xx_ll_rcc.h"

#define CAMERA_SRC_WIDTH             3864U
#define CAMERA_SRC_HEIGHT            2192U
#define CAMERA_PREVIEW_WIDTH         512U
#define CAMERA_PREVIEW_HEIGHT        300U
#define CAMERA_ISP_WIDTH             (CAMERA_SRC_WIDTH / 2U)
#define CAMERA_ISP_HEIGHT            CAMERA_SRC_HEIGHT
#define CAMERA_PREVIEW_PITCH_BYTES   (CAMERA_PREVIEW_WIDTH * sizeof(uint16_t))
#define CAMERA_PREVIEW_BYTES         (CAMERA_PREVIEW_PITCH_BYTES * CAMERA_PREVIEW_HEIGHT)
#define CAMERA_CAPTURE_TIMEOUT_MS    1000U
#define CAMERA_USE_TEST_PATTERN      0U

#define DCMIPP_DT_RAW10              0x2BU
#define DCMIPP_CSI_DT_BPP10          3U
#define DCMIPP_CSI_TWO_DATA_LANES    (2U << CSI_LMCFGR_LANENB_Pos)
#define DCMIPP_CSI_DATA_LANE0        1U
#define DCMIPP_CSI_DATA_LANE1        2U
#define DCMIPP_MODE_SNAPSHOT         DCMIPP_P1FCTCR_CPTMODE
#define DCMIPP_SERIAL_MODE           DCMIPP_CMCR_INSEL
#define DCMIPP_PIXEL_PACKER_RGB565   (1U << DCMIPP_P1PPCR_FORMAT_Pos)
#define DCMIPP_RAWBAYER_GBRG         (2U << DCMIPP_P1DMCR_TYPE_Pos)
#define DCMIPP_ISP_HDEC_1_OUT_2      (1U << DCMIPP_P1DECR_HDEC_Pos)
#define DCMIPP_DOWNSIZE_HRATIO       ((CAMERA_ISP_WIDTH * 8192U) / CAMERA_PREVIEW_WIDTH)
#define DCMIPP_DOWNSIZE_VRATIO       ((CAMERA_ISP_HEIGHT * 8192U) / CAMERA_PREVIEW_HEIGHT)
#define DCMIPP_DOWNSIZE_HDIV         ((CAMERA_PREVIEW_WIDTH * 1023U + (CAMERA_ISP_WIDTH / 2U)) / CAMERA_ISP_WIDTH)
#define DCMIPP_DOWNSIZE_VDIV         ((CAMERA_PREVIEW_HEIGHT * 1023U + (CAMERA_ISP_HEIGHT / 2U)) / CAMERA_ISP_HEIGHT)

static uint16_t s_camera_preview[CAMERA_PREVIEW_WIDTH * CAMERA_PREVIEW_HEIGHT]
  __attribute__((aligned(32)));
static CameraDemoDebug s_camera_debug;

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
  for (uint32_t y = 0; y < RGB_LCD_HEIGHT; y++)
  {
    uint32_t sy = (y * CAMERA_PREVIEW_HEIGHT) / RGB_LCD_HEIGHT;
    const uint16_t *src = &s_camera_preview[sy * CAMERA_PREVIEW_WIDTH];
    uint16_t *dst = &fb[y * RGB_LCD_WIDTH];

    for (uint32_t x = 0; x < RGB_LCD_WIDTH; x++)
    {
      uint32_t sx = (x * CAMERA_PREVIEW_WIDTH) / RGB_LCD_WIDTH;

      dst[x] = src[sx];
    }
  }

  RGB_LCD_Flush();
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
   * 3864 pixels is too wide for direct demosaic.
   * ISP horizontal decimation: 3864 -> 1932.
   */
  DCMIPP->P1DECR = DCMIPP_P1DECR_ENABLE | DCMIPP_ISP_HDEC_1_OUT_2;
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

  return CAMERA_DEMO_OK;
}

static void CameraDemo_StartPipe1(void)
{
  SCB_CleanInvalidateDCache_by_Addr((uint32_t *)s_camera_preview, sizeof(s_camera_preview));
  DCMIPP->P1FCR = DCMIPP_P1FCR_CFRAMEF | DCMIPP_P1FCR_COVRF |
                  DCMIPP_P1FCR_CVSYNCF;
  DCMIPP->P1PPM0AR1 = (uint32_t)s_camera_preview;
  DCMIPP->P1FCTCR = DCMIPP_MODE_SNAPSHOT;
  DCMIPP->P1FSCR = DCMIPP_DT_RAW10 | DCMIPP_P1FSCR_PIPEN;
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
  DCMIPP->P1FCR = DCMIPP_P1FCR_CFRAMEF | DCMIPP_P1FCR_CVSYNCF;
  CameraDemo_CopyPreviewToLcd();
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
  return CAMERA_DEMO_OK;
}

CameraDemoStatus CameraDemo_CaptureToLcd(void)
{
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
