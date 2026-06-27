#include "rgb_lcd.h"

#include "stm32n647xx.h"
#include "stm32n6xx_ll_bus.h"
#include "stm32n6xx_ll_gpio.h"
#include "stm32n6xx_ll_rcc.h"

#define RGB_LCD_HSW  20U
#define RGB_LCD_HBP  140U
#define RGB_LCD_HFP  160U
#define RGB_LCD_VSW  3U
#define RGB_LCD_VBP  20U
#define RGB_LCD_VFP  12U

#define RGB_LCD_BYTES_PER_PIXEL  2U
#define RGB_LCD_FRAMEBUFFER_PIXELS (RGB_LCD_WIDTH * RGB_LCD_HEIGHT)

#define RGB_LCD_LTDC_CLOCK_HZ  40000000U
#define RGB_LCD_PLL1_CLOCK_HZ  1200000000U

#define RGB_LCD_LAYER_PF_RGB565  0x4U
#define RGB_LCD_BF1_CA           0x00000400U
#define RGB_LCD_BF2_CA           0x00000005U

static uint16_t s_lcd_framebuffer[RGB_LCD_FRAMEBUFFER_PIXELS]
  __attribute__((section(".EXTRAM"), aligned(32)));

static void RGB_LCD_GPIO_Init(void)
{
  LL_GPIO_InitTypeDef gpio = {0};

  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOA);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOB);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOF);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOG);
  LL_AHB4_GRP1_EnableClock(LL_AHB4_GRP1_PERIPH_GPIOH);

  gpio.Mode = LL_GPIO_MODE_ALTERNATE;
  gpio.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio.Pull = LL_GPIO_PULL_NO;
  gpio.Alternate = LL_GPIO_AF_14;

  gpio.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_5 |
             LL_GPIO_PIN_7 | LL_GPIO_PIN_8 | LL_GPIO_PIN_9 | LL_GPIO_PIN_10 |
             LL_GPIO_PIN_11 | LL_GPIO_PIN_12 | LL_GPIO_PIN_15;
  LL_GPIO_Init(GPIOA, &gpio);

  gpio.Pin = LL_GPIO_PIN_1 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_10 |
             LL_GPIO_PIN_11 | LL_GPIO_PIN_12 | LL_GPIO_PIN_15;
  LL_GPIO_Init(GPIOB, &gpio);

  gpio.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
  LL_GPIO_Init(GPIOF, &gpio);

  gpio.Pin = LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_9 | LL_GPIO_PIN_12 |
             LL_GPIO_PIN_13 | LL_GPIO_PIN_15;
  LL_GPIO_Init(GPIOG, &gpio);

  gpio.Alternate = LL_GPIO_AF_10;
  gpio.Pin = LL_GPIO_PIN_0;
  LL_GPIO_Init(GPIOG, &gpio);

  gpio.Alternate = LL_GPIO_AF_14;
  gpio.Pin = LL_GPIO_PIN_4;
  LL_GPIO_Init(GPIOH, &gpio);

  gpio.Mode = LL_GPIO_MODE_OUTPUT;
  gpio.Speed = LL_GPIO_SPEED_FREQ_LOW;
  gpio.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio.Pull = LL_GPIO_PULL_NO;
  gpio.Pin = LL_GPIO_PIN_3;
  LL_GPIO_Init(GPIOA, &gpio);
}

static void RGB_LCD_Clock_Init(void)
{
  uint32_t divider = RGB_LCD_PLL1_CLOCK_HZ / RGB_LCD_LTDC_CLOCK_HZ;

  if (divider == 0U)
  {
    divider = 1U;
  }

  LL_RCC_IC16_Disable();
  LL_RCC_IC16_SetSource(LL_RCC_ICCLKSOURCE_PLL1);
  LL_RCC_IC16_SetDivider(divider);
  LL_RCC_IC16_Enable();
  LL_RCC_SetLTDCClockSource(LL_RCC_LTDC_CLKSOURCE_IC16);

  LL_APB5_GRP1_ForceReset(LL_APB5_GRP1_PERIPH_LTDC);
  LL_APB5_GRP1_ReleaseReset(LL_APB5_GRP1_PERIPH_LTDC);
  LL_APB5_GRP1_EnableClock(LL_APB5_GRP1_PERIPH_LTDC);
}

static void RGB_LCD_LTDC_Init(void)
{
  uint32_t ahbp = RGB_LCD_HSW + RGB_LCD_HBP - 1U;
  uint32_t avbp = RGB_LCD_VSW + RGB_LCD_VBP - 1U;
  uint32_t active_w = RGB_LCD_HSW + RGB_LCD_HBP + RGB_LCD_WIDTH - 1U;
  uint32_t active_h = RGB_LCD_VSW + RGB_LCD_VBP + RGB_LCD_HEIGHT - 1U;
  uint32_t total_w = RGB_LCD_HSW + RGB_LCD_HBP + RGB_LCD_WIDTH + RGB_LCD_HFP - 1U;
  uint32_t total_h = RGB_LCD_VSW + RGB_LCD_VBP + RGB_LCD_HEIGHT + RGB_LCD_VFP - 1U;
  uint32_t pitch = RGB_LCD_WIDTH * RGB_LCD_BYTES_PER_PIXEL;
  LTDC_Layer_TypeDef *layer = LTDC_Layer1;

  LTDC->GCR = 0U;
  LTDC->SSCR = ((RGB_LCD_HSW - 1U) << LTDC_SSCR_HSW_Pos) |
               ((RGB_LCD_VSW - 1U) << LTDC_SSCR_VSH_Pos);
  LTDC->BPCR = (ahbp << LTDC_BPCR_AHBP_Pos) | (avbp << LTDC_BPCR_AVBP_Pos);
  LTDC->AWCR = (active_w << LTDC_AWCR_AAW_Pos) | (active_h << LTDC_AWCR_AAH_Pos);
  LTDC->TWCR = (total_w << LTDC_TWCR_TOTALW_Pos) | (total_h << LTDC_TWCR_TOTALH_Pos);
  LTDC->BCCR = 0U;

  layer->CR = 0U;
  layer->C0R = LTDC_LxC0R_P565;
  layer->PFCR = RGB_LCD_LAYER_PF_RGB565;
  layer->CACR = 255U;
  layer->DCCR = 0U;
  layer->BFCR = RGB_LCD_BF1_CA | RGB_LCD_BF2_CA;
  layer->WHPCR = ((RGB_LCD_WIDTH + ahbp) << LTDC_LxWHPCR_WHSPPOS_Pos) |
                 ((ahbp + 1U) << LTDC_LxWHPCR_WHSTPOS_Pos);
  layer->WVPCR = ((RGB_LCD_HEIGHT + avbp) << LTDC_LxWVPCR_WVSPPOS_Pos) |
                 ((avbp + 1U) << LTDC_LxWVPCR_WVSTPOS_Pos);
  layer->CFBAR = (uint32_t)s_lcd_framebuffer;
  layer->CFBLR = (pitch << LTDC_LxCFBLR_CFBP_Pos) |
                 ((pitch + 7U) << LTDC_LxCFBLR_CFBLL_Pos);
  layer->CFBLNR = RGB_LCD_HEIGHT;
  layer->RCR = LTDC_LxRCR_IMR | LTDC_LxRCR_GRMSK;
  layer->CR = LTDC_LxCR_LEN;

  LTDC_Layer2->CR = 0U;
  LTDC_Layer2->RCR = LTDC_LxRCR_GRMSK;

  LTDC->SRCR = LTDC_SRCR_IMR;
  LTDC->GCR |= LTDC_GCR_LTDCEN;
}

void RGB_LCD_Fill(uint16_t color)
{
  for (uint32_t i = 0; i < RGB_LCD_FRAMEBUFFER_PIXELS; i++)
  {
    s_lcd_framebuffer[i] = color;
  }

  SCB_CleanDCache_by_Addr((uint32_t *)s_lcd_framebuffer, sizeof(s_lcd_framebuffer));
}

void RGB_LCD_Init(void)
{
  RGB_LCD_GPIO_Init();
  RGB_LCD_Clock_Init();
  RGB_LCD_Fill(0x0000U);
  RGB_LCD_LTDC_Init();
  LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_3);
}
