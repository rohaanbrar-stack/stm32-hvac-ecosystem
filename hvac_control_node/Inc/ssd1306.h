#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

void SSD1306_Init(void);
void SSD1306_SetPixel(uint8_t x, uint8_t y);
void SSD1306_Refresh();
void SSD1306_Clear();
void SSD1306_DrawChar(char c, uint8_t x, uint8_t y);
void SSD1306_DrawString(char *str, uint8_t x, uint8_t y);

#endif
