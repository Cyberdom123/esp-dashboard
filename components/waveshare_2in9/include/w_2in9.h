#ifndef W_2IN9
#define W_2IN9

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"

#include "freertos/semphr.h"

#include "w_2in9_defs.h"
#include "w_2in9_paint.h"

esp_err_t w_2in9_init(w_2in9 *dev);

void w_2in9_clear_frame_memory(w_2in9 *dev, uint8_t color);

void w_2in9_display_frame_update(w_2in9 *dev);

void w_2in9_display_frame_update_partial(w_2in9 *dev);

void w_2in9_reinit(w_2in9 *dev);

void w_2in9_set_frame_memory(w_2in9_paint *paint, w_2in9 *dev, int x,
                             int y, int image_width, int image_height);

void w_2in9_set_frame_partial(w_2in9_paint *paint, w_2in9 *dev, int x,
                             int y, int image_width, int image_height);

void w_2in9_set_frame_memory_base(w_2in9_paint *paint, w_2in9 *dev, int x,
                             int y, int image_width, int image_height);

#endif