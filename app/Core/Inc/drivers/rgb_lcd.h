#ifndef RGB_LCD_H
#define RGB_LCD_H

#include <stdint.h>

#define RGB_LCD_WIDTH   1024U
#define RGB_LCD_HEIGHT  600U

void RGB_LCD_Init(void);
void RGB_LCD_Fill(uint16_t color);

#endif /* RGB_LCD_H */
