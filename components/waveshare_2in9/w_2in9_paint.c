#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"

#include "freertos/semphr.h"
#include  "w_2in9_paint.h"

static const char *TAG = "paint";


esp_err_t w_2in9_paint_init(w_2in9_paint *paint){
    uint8_t* frame_buf = (unsigned char*) heap_caps_malloc(
            (paint->width * paint->height / 8), MALLOC_CAP_8BIT);
    if (frame_buf == NULL) {
        ESP_LOGE(TAG, "frame_buffer malloc fail");
        return ESP_FAIL;
    }
    paint->image = frame_buf;

    paint->paint_mutx = xSemaphoreCreateRecursiveMutex();

    return ESP_OK;
}

/**
 *  @brief: this draws a pixel by absolute coordinates.
 *          this function won't be affected by the rotate parameter.
 */
void w_2in9_draw_absolute_pixel(w_2in9_paint *paint, int x, int y, int colored) {
    xSemaphoreTakeRecursive(paint->paint_mutx, portMAX_DELAY);
    if (x < 0 || x >= paint->width || y < 0 || y >= paint->height) {
        return;
    }
    if (paint->color_inv) {
        if (colored) {
            paint->image[(x + y * paint->width) / 8] |= 0x80 >> (x % 8);
        } else {
            paint->image[(x + y * paint->width) / 8] &= ~(0x80 >> (x % 8));
        }
    } else {
        if (colored) {
            paint->image[(x + y * paint->width) / 8] &= ~(0x80 >> (x % 8));
        } else {
            paint->image[(x + y * paint->width) / 8] |= 0x80 >> (x % 8);
        }
    }
    xSemaphoreGiveRecursive(paint->paint_mutx);
}


/**
 *  @brief: this draws a pixel by the coordinates
 */
void w_2in9_draw_pixel(w_2in9_paint *paint, int x, int y, int colored){
    int point_temp;
    xSemaphoreTakeRecursive(paint->paint_mutx, portMAX_DELAY);
    if (paint->rotate == E_PAPER_ROTATE_0) {
        if(x < 0 || x >= paint->width || y < 0 || y >= paint->height) {
            return;
        }
        w_2in9_draw_absolute_pixel(paint, x, y, colored);
    } else if (paint->rotate == E_PAPER_ROTATE_90) {
        if(x < 0 || x >= paint->height || y < 0 || y >= paint->width) {
          return;
        }
        point_temp = x;
        x = paint->width - y;
        y = point_temp;
        w_2in9_draw_absolute_pixel(paint, x, y, colored);
    } else if (paint->rotate == E_PAPER_ROTATE_180) {
        if(x < 0 || x >= paint->width || y < 0 || y >= paint->height) {
          return;
        }
        x = paint->width - x;
        y = paint->height - y;
        w_2in9_draw_absolute_pixel(paint, x, y, colored);
    } else if (paint->rotate == E_PAPER_ROTATE_270) {
        if(x < 0 || x >= paint->height || y < 0 || y >= paint->width) {
          return;
        }
        point_temp = x;
        x = y;
        y = paint->height - point_temp;
        w_2in9_draw_absolute_pixel(paint, x, y, colored);
    }
    xSemaphoreGiveRecursive(paint->paint_mutx);
}

void w_2in9_draw_char(w_2in9_paint *paint, int x, int y, char ascii_char, epaper_font_t *font, uint8_t color) {
    int i, j;
    unsigned int char_offset = (ascii_char - ' ') * font->height * (font->width / 8 + (font->width % 8 ? 1 : 0));
    const unsigned char* ptr = &font->font_table[char_offset];
    xSemaphoreTakeRecursive(paint->paint_mutx, portMAX_DELAY);
    for (j = 0; j < font->height; j++) {
        for (i = 0; i < font->width; i++) {
            if (*ptr & (0x80 >> (i % 8))) {
                w_2in9_draw_pixel(paint, x + i, y + j, color);
            }
            if (i % 8 == 7) {
                ptr++;
            }
        }
        if (font->width % 8 != 0) {
            ptr++;
        }
    }
    xSemaphoreGiveRecursive(paint->paint_mutx);
  
}

void w_2in9_draw_string(w_2in9_paint *paint, int x, int y, const char* text, epaper_font_t *font, int colored) {
    const char* p_text = text;
    unsigned int counter = 0;
    int refcolumn = x;
    
    xSemaphoreTakeRecursive(paint->paint_mutx, portMAX_DELAY);
    /* Send the string character by character on EPD */
    while (*p_text != 0) {
        /* Display one character on EPD */
        w_2in9_draw_char(paint, refcolumn, y, *p_text, font, colored);
        /* Decrement the column position by 16 */
        refcolumn += font->width;
        /* Point on the next character */
        p_text++;
        counter++;
    }
    xSemaphoreGiveRecursive(paint->paint_mutx);
}

void w_2in9_clean_paint(w_2in9_paint *paint, int colored)
{
    xSemaphoreTakeRecursive(paint->paint_mutx, portMAX_DELAY);
    for (int x = 0; x < paint->width; x++) {
        for (int y = 0; y < paint->height; y++) {
            w_2in9_draw_absolute_pixel(paint, x, y, colored);
        }
    }
    xSemaphoreGiveRecursive(paint->paint_mutx);
}


/**
*  @brief: this draws a line on the frame buffer
*/
void w_2in9_draw_line(w_2in9_paint *paint, int x0, int y0, int x1, int y1, int colored) {
    /* Bresenham algorithm */
    int dx = x1 - x0 >= 0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 - y0 <= 0 ? y1 - y0 : y0 - y1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    xSemaphoreTakeRecursive(paint->paint_mutx, portMAX_DELAY);
    while((x0 != x1) && (y0 != y1)) {
        w_2in9_draw_pixel(paint, x0, y0 , colored);
        if (2 * err >= dy) {     
            err += dy;
            x0 += sx;
        }
        if (2 * err <= dx) {
            err += dx; 
            y0 += sy;
        }
    }
    xSemaphoreGiveRecursive(paint->paint_mutx);
}

/**
*  @brief: this draws a horizontal line on the frame buffer
*/
void w_2in9_draw_horizontal_line(w_2in9_paint *paint, int x, int y, int line_width, int colored) {
    int i;
    xSemaphoreTakeRecursive(paint->paint_mutx, portMAX_DELAY);
    for (i = x; i < x + line_width; i++) {
        w_2in9_draw_pixel(paint, i, y, colored);
    }
    xSemaphoreGiveRecursive(paint->paint_mutx);
}

/**
*  @brief: this draws a vertical line on the frame buffer
*/
void w_2in9_draw_vertical_line(w_2in9_paint *paint , int x, int y, int line_height, int colored) {
    int i;
    xSemaphoreTakeRecursive(paint->paint_mutx, portMAX_DELAY);
    for (i = y; i < y + line_height; i++) {
        w_2in9_draw_pixel(paint, x, i, colored);
    }
    xSemaphoreGiveRecursive(paint->paint_mutx);
}

/**
*  @brief: this draws a rectangle
*/
void w_2in9_draw_rectangle(w_2in9_paint *paint, int x0, int y0, int x1, int y1, int colored) {
    xSemaphoreTakeRecursive(paint->paint_mutx, portMAX_DELAY);
    int min_x, min_y, max_x, max_y;
    min_x = x1 > x0 ? x0 : x1;
    max_x = x1 > x0 ? x1 : x0;
    min_y = y1 > y0 ? y0 : y1;
    max_y = y1 > y0 ? y1 : y0;
    
    w_2in9_draw_horizontal_line(paint, min_x, min_y, max_x - min_x + 1, colored);
    w_2in9_draw_horizontal_line(paint, min_x, max_y, max_x - min_x + 1, colored);
    w_2in9_draw_vertical_line(paint, min_x, min_y, max_y - min_y + 1, colored);
    w_2in9_draw_vertical_line(paint, max_x, min_y, max_y - min_y + 1, colored);
    xSemaphoreGiveRecursive(paint->paint_mutx);
}


/**
*  @brief: this draws a circle
*/
void w_2in9_draw_circle(w_2in9_paint *paint, int x, int y, int radius, int colored) {
    xSemaphoreTakeRecursive(paint->paint_mutx, portMAX_DELAY);
    /* Bresenham algorithm */
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    do {
        w_2in9_draw_pixel(paint, x - x_pos, y + y_pos, colored);
        w_2in9_draw_pixel(paint, x + x_pos, y + y_pos, colored);
        w_2in9_draw_pixel(paint, x + x_pos, y - y_pos, colored);
        w_2in9_draw_pixel(paint, x - x_pos, y - y_pos, colored);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos) {
              e2 = 0;
            }
        }
        if (e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while (x_pos <= 0);
    xSemaphoreGiveRecursive(paint->paint_mutx);
}


/**
*  @brief: this draws a filled circle
*/
void w_2in9_draw_filled_circle(w_2in9_paint *paint, int x, int y, int radius, int colored) {
    /* Bresenham algorithm */
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    xSemaphoreTakeRecursive(paint->paint_mutx, portMAX_DELAY);
    do {
        w_2in9_draw_pixel(paint, x - x_pos, y + y_pos, colored);
        w_2in9_draw_pixel(paint, x + x_pos, y + y_pos, colored);
        w_2in9_draw_pixel(paint, x + x_pos, y - y_pos, colored);
        w_2in9_draw_pixel(paint, x - x_pos, y - y_pos, colored);
        w_2in9_draw_horizontal_line(paint, x + x_pos, y + y_pos, 2 * (-x_pos) + 1, colored);
        w_2in9_draw_horizontal_line(paint, x + x_pos, y - y_pos, 2 * (-x_pos) + 1, colored);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos) {
                e2 = 0;
            }
        }
        if(e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while(x_pos <= 0);
    xSemaphoreGiveRecursive(paint->paint_mutx);
}
