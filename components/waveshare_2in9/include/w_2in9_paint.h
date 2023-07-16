#ifndef W_2IN9_PAINT
#define W_2IN9_PAINT

#include "fonts.h"

typedef enum{
    E_PAPER_ROTATE_0,
    E_PAPER_ROTATE_90,
    E_PAPER_ROTATE_180,
    E_PAPER_ROTATE_270,
} w_2in9_rotate;

typedef struct 
{   
    w_2in9_rotate rotate;
    unsigned char *image;
    uint16_t width;
    uint16_t height;
    uint8_t color_inv;
    xSemaphoreHandle paint_mutx;
}w_2in9_paint;

esp_err_t w_2in9_paint_init(w_2in9_paint *paint);
void w_2in9_draw_string(w_2in9_paint *paint, int x, int y, const char* text, epaper_font_t *font, int colored);
void w_2in9_draw_char(w_2in9_paint *paint, int x, int y, char ascii_char, epaper_font_t *font, uint8_t color);
void w_2in9_draw_pixel(w_2in9_paint *paint, int x, int y, int colored);
void w_2in9_clean_paint(w_2in9_paint *paint, int colored);
void w_2in9_draw_line(w_2in9_paint *paint, int x0, int y0, int x1, int y1, int colored);
void w_2in9_draw_horizontal_line(w_2in9_paint *paint, int x, int y, int line_width, int colored);
void w_2in9_draw_vertical_line(w_2in9_paint *paint , int x, int y, int line_height, int colored);
void w_2in9_draw_rectangle(w_2in9_paint *paint, int x0, int y0, int x1, int y1, int colored);
void w_2in9_draw_circle(w_2in9_paint *paint, int x, int y, int radius, int colored);
void w_2in9_draw_filled_circle(w_2in9_paint *paint, int x, int y, int radius, int colored);
#endif