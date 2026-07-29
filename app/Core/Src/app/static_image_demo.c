#include "app_tasks.h"

#include "FreeRTOS.h"
#include "task.h"

#include "ai_instance_segmentation.h"
#include "app_config.h"
#include "console_log.h"
#include "rgb_lcd.h"

#include <stdint.h>

#define STATIC_IMAGE_FLASH_BASE  ((uintptr_t)0x71800000UL)
#define STATIC_IMAGE_BMP_WIDTH   872U
#define STATIC_IMAGE_BMP_HEIGHT  586U
#define STATIC_IMAGE_BPP         3U
#define STATIC_IMAGE_BOX_COUNT   10U
#define STATIC_IMAGE_BOX_COLOR   0xFFE0U /* yellow, RGB565 */
#define STATIC_IMAGE_BOX_BORDER  3U

typedef struct
{
  const uint8_t *base;
  uint32_t pixel_offset;
  uint32_t row_stride;
  uint32_t width;
  uint32_t height;
  uint8_t top_down;
} StaticBmp;

static uint16_t StaticImage_ReadLE16(const uint8_t *p)
{
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t StaticImage_ReadLE32(const uint8_t *p)
{
  return (uint32_t)p[0] |
         ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

static int32_t StaticImage_ReadLE32S(const uint8_t *p)
{
  return (int32_t)StaticImage_ReadLE32(p);
}

static uint8_t StaticImage_ParseBmp(StaticBmp *bmp)
{
  const uint8_t *base = (const uint8_t *)STATIC_IMAGE_FLASH_BASE;
  uint32_t file_size;
  uint32_t dib_size;
  int32_t signed_height;
  uint32_t height;
  uint32_t row_stride;
  uint32_t payload_size;

  if ((base[0] != 'B') || (base[1] != 'M'))
  {
    return 0U;
  }

  file_size = StaticImage_ReadLE32(&base[2]);
  if (file_size < 54U)
  {
    return 0U;
  }

  bmp->pixel_offset = StaticImage_ReadLE32(&base[10]);
  dib_size = StaticImage_ReadLE32(&base[14]);
  signed_height = StaticImage_ReadLE32S(&base[22]);
  if ((dib_size < 40U) || (StaticImage_ReadLE32S(&base[18]) <= 0) ||
      (signed_height == 0) || (StaticImage_ReadLE16(&base[26]) != 1U) ||
      (StaticImage_ReadLE16(&base[28]) != 24U) ||
      (StaticImage_ReadLE32(&base[30]) != 0U))
  {
    return 0U;
  }

  bmp->width = (uint32_t)StaticImage_ReadLE32S(&base[18]);
  bmp->top_down = (signed_height < 0) ? 1U : 0U;
  height = (signed_height < 0) ? (uint32_t)(-signed_height) : (uint32_t)signed_height;
  row_stride = ((bmp->width * STATIC_IMAGE_BPP) + 3U) & ~3U;
  payload_size = row_stride * height;

  if ((bmp->width != STATIC_IMAGE_BMP_WIDTH) ||
      (height != STATIC_IMAGE_BMP_HEIGHT) ||
      (bmp->pixel_offset < 54U) ||
      (bmp->pixel_offset > file_size) ||
      (payload_size > (file_size - bmp->pixel_offset)))
  {
    return 0U;
  }

  bmp->base = base;
  bmp->height = height;
  bmp->row_stride = row_stride;
  return 1U;
}

static void StaticImage_ReadRGB(const StaticBmp *bmp,
                                uint32_t x,
                                uint32_t y,
                                uint8_t *r,
                                uint8_t *g,
                                uint8_t *b)
{
  uint32_t row = (bmp->top_down != 0U) ? y : (bmp->height - 1U - y);
  const uint8_t *pixel = &bmp->base[bmp->pixel_offset +
                                    (row * bmp->row_stride) +
                                    (x * STATIC_IMAGE_BPP)];

  /* BMP stores pixels as B,G,R. */
  *b = pixel[0];
  *g = pixel[1];
  *r = pixel[2];
}

static uint16_t StaticImage_RGB565(uint8_t r, uint8_t g, uint8_t b)
{
  return (uint16_t)(((uint16_t)(r & 0xF8U) << 8) |
                    ((uint16_t)(g & 0xFCU) << 3) |
                    ((uint16_t)b >> 3));
}

static void StaticImage_Render(const StaticBmp *bmp,
                               uint32_t *draw_x,
                               uint32_t *draw_y,
                               uint32_t *draw_width,
                               uint32_t *draw_height)
{
  uint16_t *framebuffer = RGB_LCD_GetFrameBuffer();
  uint32_t width = RGB_LCD_WIDTH;
  uint32_t height = RGB_LCD_HEIGHT;
  uint32_t image_width = (bmp->width * height) / bmp->height;
  uint32_t image_height = height;

  if (image_width > width)
  {
    image_width = width;
    image_height = (bmp->height * width) / bmp->width;
  }

  *draw_x = (width - image_width) / 2U;
  *draw_y = (height - image_height) / 2U;
  *draw_width = image_width;
  *draw_height = image_height;

  RGB_LCD_Fill(0x0000U);
  for (uint32_t y = 0U; y < image_height; ++y)
  {
    uint32_t source_y = (y * bmp->height) / image_height;
    for (uint32_t x = 0U; x < image_width; ++x)
    {
      uint32_t source_x = (x * bmp->width) / image_width;
      uint8_t r;
      uint8_t g;
      uint8_t b;

      StaticImage_ReadRGB(bmp, source_x, source_y, &r, &g, &b);
      framebuffer[(y + *draw_y) * RGB_LCD_WIDTH + x + *draw_x] =
        StaticImage_RGB565(r, g, b);
    }
  }
  RGB_LCD_Flush();
}

static void StaticImage_PrepareInput(const StaticBmp *bmp, uint8_t *input)
{
  uint32_t crop_size = (bmp->width < bmp->height) ? bmp->width : bmp->height;
  uint32_t crop_x = (bmp->width - crop_size) / 2U;
  uint32_t crop_y = (bmp->height - crop_size) / 2U;

  for (uint32_t y = 0U; y < NN_HEIGHT; ++y)
  {
    uint32_t source_y = crop_y + ((y * (crop_size - 1U)) / (NN_HEIGHT - 1U));
    for (uint32_t x = 0U; x < NN_WIDTH; ++x)
    {
      uint32_t source_x = crop_x + ((x * (crop_size - 1U)) / (NN_WIDTH - 1U));
      uint8_t r;
      uint8_t g;
      uint8_t b;
      uint32_t offset = (y * NN_WIDTH + x) * NN_BPP;

      StaticImage_ReadRGB(bmp, source_x, source_y, &r, &g, &b);
      input[offset + 0U] = r;
      input[offset + 1U] = g;
      input[offset + 2U] = b;
    }
  }
}

static int32_t StaticImage_ClampCoordinate(float value, uint32_t limit)
{
  if (value <= 0.0f)
  {
    return 0;
  }
  if (value >= (float)(limit - 1U))
  {
    return (int32_t)(limit - 1U);
  }
  return (int32_t)(value + 0.5f);
}

static void StaticImage_HLine(uint16_t *framebuffer,
                              int32_t x0,
                              int32_t x1,
                              int32_t y,
                              uint16_t color)
{
  if ((y < 0) || (y >= (int32_t)RGB_LCD_HEIGHT))
  {
    return;
  }
  if (x0 > x1)
  {
    int32_t tmp = x0;
    x0 = x1;
    x1 = tmp;
  }
  if (x0 < 0) x0 = 0;
  if (x1 >= (int32_t)RGB_LCD_WIDTH) x1 = (int32_t)RGB_LCD_WIDTH - 1;
  for (int32_t x = x0; x <= x1; ++x)
  {
    framebuffer[y * RGB_LCD_WIDTH + x] = color;
  }
}

static void StaticImage_VLine(uint16_t *framebuffer,
                              int32_t x,
                              int32_t y0,
                              int32_t y1,
                              uint16_t color)
{
  if ((x < 0) || (x >= (int32_t)RGB_LCD_WIDTH))
  {
    return;
  }
  if (y0 > y1)
  {
    int32_t tmp = y0;
    y0 = y1;
    y1 = tmp;
  }
  if (y0 < 0) y0 = 0;
  if (y1 >= (int32_t)RGB_LCD_HEIGHT) y1 = (int32_t)RGB_LCD_HEIGHT - 1;
  for (int32_t y = y0; y <= y1; ++y)
  {
    framebuffer[y * RGB_LCD_WIDTH + x] = color;
  }
}

static void StaticImage_DrawBox(uint16_t *framebuffer,
                                const StaticBmp *bmp,
                                uint32_t draw_x,
                                uint32_t draw_y,
                                uint32_t draw_width,
                                uint32_t draw_height,
                                const AIInstanceSegmentationBox *box)
{
  float crop_size = (float)((bmp->width < bmp->height) ? bmp->width : bmp->height);
  float crop_x = ((float)bmp->width - crop_size) * 0.5f;
  float x0_source = crop_x + (box->x_center - box->width * 0.5f) * crop_size;
  float x1_source = crop_x + (box->x_center + box->width * 0.5f) * crop_size;
  float y0_source = (box->y_center - box->height * 0.5f) * crop_size;
  float y1_source = (box->y_center + box->height * 0.5f) * crop_size;
  int32_t x0 = StaticImage_ClampCoordinate((float)draw_x +
                                            x0_source * (float)draw_width / (float)bmp->width,
                                            RGB_LCD_WIDTH);
  int32_t x1 = StaticImage_ClampCoordinate((float)draw_x +
                                            x1_source * (float)draw_width / (float)bmp->width,
                                            RGB_LCD_WIDTH);
  int32_t y0 = StaticImage_ClampCoordinate((float)draw_y +
                                            y0_source * (float)draw_height / (float)bmp->height,
                                            RGB_LCD_HEIGHT);
  int32_t y1 = StaticImage_ClampCoordinate((float)draw_y +
                                            y1_source * (float)draw_height / (float)bmp->height,
                                            RGB_LCD_HEIGHT);

  for (uint32_t border = 0U; border < STATIC_IMAGE_BOX_BORDER; ++border)
  {
    StaticImage_HLine(framebuffer, x0, x1, y0 + (int32_t)border, STATIC_IMAGE_BOX_COLOR);
    StaticImage_HLine(framebuffer, x0, x1, y1 - (int32_t)border, STATIC_IMAGE_BOX_COLOR);
    StaticImage_VLine(framebuffer, x0 + (int32_t)border, y0, y1, STATIC_IMAGE_BOX_COLOR);
    StaticImage_VLine(framebuffer, x1 - (int32_t)border, y0, y1, STATIC_IMAGE_BOX_COLOR);
  }
}

void StaticImageTask(void *argument)
{
  StaticBmp bmp;
  AIInstanceSegmentationBox boxes[STATIC_IMAGE_BOX_COUNT];
  uint32_t draw_x;
  uint32_t draw_y;
  uint32_t draw_width;
  uint32_t draw_height;
  uint32_t sequence;
  uint8_t *input;
  uint32_t box_count;

  (void)argument;
  LOG_INFO("static: task start\r\n");
  RGB_LCD_Init();
  LOG_INFO("static: LTDC initialized\r\n");

  if (StaticImage_ParseBmp(&bmp) == 0U)
  {
    LOG_ERROR("static: invalid BMP at 0x%08lx\r\n",
              (unsigned long)STATIC_IMAGE_FLASH_BASE);
    for (;;) vTaskDelay(pdMS_TO_TICKS(1000U));
  }

  LOG_INFO("static: BMP %lux%lu from Flash 0x%08lx\r\n",
           (unsigned long)bmp.width, (unsigned long)bmp.height,
           (unsigned long)STATIC_IMAGE_FLASH_BASE);
  StaticImage_Render(&bmp, &draw_x, &draw_y, &draw_width, &draw_height);

  while (AIInstanceSegmentation_IsReady() == 0U)
  {
    vTaskDelay(pdMS_TO_TICKS(10U));
  }

  input = AIInstanceSegmentation_GetInputBuffer();
  if (input == NULL)
  {
    LOG_ERROR("static: NPU input buffer unavailable\r\n");
    for (;;) vTaskDelay(pdMS_TO_TICKS(1000U));
  }

  StaticImage_PrepareInput(&bmp, input);
  sequence = AIInstanceSegmentation_GetResultSequence();
  AIInstanceSegmentation_SubmitFrame();
  if (AIInstanceSegmentation_WaitForResult(sequence, 10000U) == 0U)
  {
    LOG_ERROR("static: NPU inference timeout\r\n");
    for (;;) vTaskDelay(pdMS_TO_TICKS(1000U));
  }

  box_count = AIInstanceSegmentation_GetBoxes(boxes, STATIC_IMAGE_BOX_COUNT);
  LOG_INFO("static: detections=%lu, drawing on LTDC framebuffer\r\n",
           (unsigned long)box_count);
  uint16_t *framebuffer = RGB_LCD_GetFrameBuffer();
  for (uint32_t i = 0U; i < box_count; ++i)
  {
    StaticImage_DrawBox(framebuffer, &bmp, draw_x, draw_y,
                        draw_width, draw_height, &boxes[i]);
  }
  RGB_LCD_Flush();

  for (;;) vTaskDelay(pdMS_TO_TICKS(1000U));
}
