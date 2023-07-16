#ifndef FONTS
#define FONTS

#include <stdint.h>

typedef struct
{
    uint16_t width;
    uint16_t height;
    const uint8_t *font_table;
} epaper_font_t;

extern epaper_font_t epaper_font_24;
extern epaper_font_t epaper_font_20;
extern epaper_font_t epaper_font_16;
extern epaper_font_t epaper_font_12;
extern epaper_font_t epaper_font_8;

#endif