//plot
#include "math.h"
//e-ink display
#include "w_2in9.h"
#include "w_2in9_paint.h"
#include "fonts.h"
#include "freertos/semphr.h"

#include "ui.h"

static const char *TAG = "ui";

static float fmax_element(const float data[]){
    float max = data[0];
    for (size_t i = 0; i < PLOT_DATA_LENGTH; i++)
        if (max < data[i])
            max = data[i];

    return max;
}

static float fcomp(const void * a, const void * b) {
   return ( *(float*)a - *(float*)b );
}



void ui_init(w_2in9 *e_paper, w_2in9_paint *paint, ui_weather *ui){
    ui->ui_mtx = xSemaphoreCreateRecursiveMutex();
    
    xSemaphoreTakeRecursive(ui->ui_mtx, portMAX_DELAY);

    e_paper->reset_pin = 12;
    e_paper->dc_pin = 2; 
    e_paper->busy_pin = 0;

    e_paper->height = 296;
    e_paper->width = 128;
    int ret = w_2in9_init(e_paper);
    if(ret == ESP_OK){
        ESP_LOGI(TAG, "e_paper initialization succed");
        w_2in9_clear_frame_memory(e_paper, 0xFF);
        w_2in9_display_frame_update(e_paper);

        vTaskDelay(2000 / portTICK_RATE_MS);

        paint->rotate = E_PAPER_ROTATE_90;
        paint->height = 296;
        paint->width = 128;
        paint->color_inv = 0;
        w_2in9_paint_init(paint);

        w_2in9_clean_paint(paint, 0);
        w_2in9_draw_vertical_line(paint, 90, 0, 128, 1);
        w_2in9_draw_string(paint, 140, 1, ui->date, &epaper_font_8, 1);
        w_2in9_draw_string(paint, 118, 114, ui->pm10, &epaper_font_12, 1);
        w_2in9_draw_string(paint, 198, 114, ui->pm25, &epaper_font_12, 1);

        /* PLOT */
        ui_draw_plot(e_paper, paint, &ui->plot);

        w_2in9_set_frame_memory_base(paint, e_paper, 0, 0, paint->width, paint->height);
        w_2in9_display_frame_update(e_paper);

        vTaskDelay(2000 / portTICK_RATE_MS);

        ESP_LOGI(TAG, "END OF DISPLAY INIT");
    }

    xSemaphoreGiveRecursive(ui->ui_mtx);
} 

void static_ui_data_update(w_2in9 *e_paper, w_2in9_paint *paint, ui_weather *ui){
    xSemaphoreTakeRecursive(ui->ui_mtx, portMAX_DELAY);

    w_2in9_reinit(e_paper);

    paint->rotate = E_PAPER_ROTATE_90;
    paint->height = 296;
    paint->width = 128;
    paint->color_inv = 0;

    //Refresh entire display 
    w_2in9_clean_paint(paint, 0);
    w_2in9_set_frame_memory_base(paint, e_paper, 0, 0, paint->width, paint->height);
    w_2in9_display_frame_update(e_paper);    
    vTaskDelay(2000 / portTICK_RATE_MS);
    
    //initialize new data
    w_2in9_draw_vertical_line(paint, 90, 0, 128, 1);
    w_2in9_draw_string(paint, 140, 1, ui->date, &epaper_font_8, 1);
    w_2in9_draw_string(paint, 118, 114, ui->pm10, &epaper_font_12, 1);
    w_2in9_draw_string(paint, 198, 114, ui->pm25, &epaper_font_12, 1);


    /* PLOT */
    ui_draw_plot(e_paper, paint, &ui->plot);
    //end

    w_2in9_set_frame_memory_base(paint, e_paper, 0, 0, paint->width, paint->height);
    w_2in9_display_frame_update(e_paper);  
    vTaskDelay(2000 / portTICK_RATE_MS);

    xSemaphoreGiveRecursive(ui->ui_mtx);
}


void ui_weather_data_update(w_2in9 *e_paper, w_2in9_paint *paint, ui_weather *ui){
    xSemaphoreTakeRecursive(ui->ui_mtx, portMAX_DELAY);
    
    paint->rotate = E_PAPER_ROTATE_90;
    paint->height = 90;
    paint->width = 120;

    w_2in9_clean_paint(paint, 0);

    /* DRAW INSIDE WEATHER DATA */
    w_2in9_draw_string(paint, 10, 0, "INSIDE", &epaper_font_16, 1);
    w_2in9_draw_string(paint, 13, 15, ui->in_temp, &epaper_font_12, 1);
    w_2in9_draw_string(paint, 13, 30, ui->in_press, &epaper_font_12, 1);
    /* DRAW OUTSIDE WEATHER DATA */
    w_2in9_draw_string(paint, 6, 50, "OUTSIDE", &epaper_font_16, 1);
    w_2in9_draw_string(paint, 13, 65, ui->out_temp, &epaper_font_12, 1);
    w_2in9_draw_string(paint, 13, 80, ui->out_press, &epaper_font_12, 1);
    w_2in9_draw_string(paint, 13, 95, ui->out_hum, &epaper_font_12, 1);
    w_2in9_draw_string(paint, 13, 109, ui->description, &epaper_font_12, 1);
    w_2in9_set_frame_partial(paint, e_paper, 0, 0, paint->width, paint->height);
    w_2in9_display_frame_update_partial(e_paper);

    xSemaphoreGiveRecursive(ui->ui_mtx);
}

void ui_draw_plot(w_2in9 *e_paper, w_2in9_paint *paint, ui_plot *plot){
        // xSemaphoreTakeRecursive(ui->ui_mtx, portMAX_DELAY);

        //Axis
        w_2in9_draw_horizontal_line(paint, PLOT_CORD_X, PLOT_CORD_Y, PLOT_LENGTH, 1);
        w_2in9_draw_vertical_line(paint, PLOT_CORD_X, PLOT_CORD_Y - PLOT_WIDTH, PLOT_WIDTH, 1);
        
        //Arrows 
        w_2in9_draw_line(paint, PLOT_CORD_X, PLOT_CORD_Y - PLOT_WIDTH, PLOT_CORD_X + 3, PLOT_CORD_Y - PLOT_WIDTH + 5, 1);
        w_2in9_draw_line(paint, PLOT_CORD_X, PLOT_CORD_Y - PLOT_WIDTH, PLOT_CORD_X - 3, PLOT_CORD_Y - PLOT_WIDTH + 5, 1);
        
        w_2in9_draw_line(paint, PLOT_CORD_X + PLOT_LENGTH, PLOT_CORD_Y, PLOT_CORD_X + PLOT_LENGTH - 5, PLOT_CORD_Y + 3, 1);
        w_2in9_draw_line(paint, PLOT_CORD_X + PLOT_LENGTH, PLOT_CORD_Y, PLOT_CORD_X + PLOT_LENGTH - 5, PLOT_CORD_Y - 3, 1);

        //X-axis
        int x[PLOT_DATA_LENGTH];
        int y[PLOT_DATA_LENGTH];
        int y_fill = 0;
        char val[10]; 
 
        //calculate spacing between pixels
        float x_max = fmax_element(plot->x_data);
        float y_max = fmax_element(plot->y_data);
        for (size_t i = 0; i < PLOT_DATA_LENGTH; i++)
        {
            x[i] = floor((plot->x_data[i]/x_max) * (PLOT_LENGTH - PLOT_END_OFFSET));

            y[i] = floor((plot->y_data[i]/y_max) * (PLOT_WIDTH - PLOT_END_OFFSET));

            //draw x-scale
            if(i > 0)
                w_2in9_draw_vertical_line(paint, PLOT_CORD_X + x[i], PLOT_CORD_Y - 1, 3, 1);

            //draw x-values
            if(i%PLOT_XDATA_SPACING  == 0){
                sprintf(val, "%1.f", plot->x_data[i]);
                w_2in9_draw_string(paint, PLOT_CORD_X + x[i] - 2, PLOT_CORD_Y + 3, val, &epaper_font_8, 1);
            }

            //draw scatter plot
            if(y[i] != 0){
                y_fill++;
                // ESP_LOGI(TAG, "PLOT: %d, %d, %d", i, y[i], y_fill);
                // w_2in9_draw_filled_circle(paint, PLOT_CORD_X + x[i], PLOT_CORD_Y - y[i], 2, 1); // for this project only
            }
        }

        for (size_t i = 0; i < y_fill-1; i++)
        {   
                w_2in9_draw_filled_circle(paint, PLOT_CORD_X + x[i], PLOT_CORD_Y - y[i], 2, 1); // for this project only
        }
        w_2in9_draw_circle(paint, PLOT_CORD_X + x[y_fill-1], PLOT_CORD_Y - y[y_fill-1], 2, 1); // for this project only
        

        //line plot
        for (size_t i = 0; i < PLOT_DATA_LENGTH-1; i++)
        {
            
            if(y[i] == y[i+1])
                w_2in9_draw_horizontal_line(paint, PLOT_CORD_X + x[i], PLOT_CORD_Y - y[i], x[i+1] - x[i], 1);
            if(y[i+1] != 0) //for this projectonly 
                w_2in9_draw_line(paint, PLOT_CORD_X + x[i], PLOT_CORD_Y - y[i], PLOT_CORD_X + x[i+1], PLOT_CORD_Y - y[i+1], 1);
        }

        float *y_temp = plot->y_data;
        qsort(y_temp, PLOT_DATA_LENGTH, sizeof(float), fcomp);

        for (size_t i = 0; i < PLOT_DATA_LENGTH; i++)
        {
            y[i] = floor((y_temp[i]/y_max) * (PLOT_WIDTH - PLOT_END_OFFSET));
            //draw y-values and y-scale
            if((i%PLOT_YDATA_SPACING == 0 && y[i] != 0) || y_fill < PLOT_YDATA_SPACING){
                //draw y-scale
                w_2in9_draw_horizontal_line(paint, PLOT_CORD_X - 1,  PLOT_CORD_Y - y[i], 3, 1);
                sprintf(val, "%1.f", y_temp[i]);

                if(y_temp[i] >= 10)
                    w_2in9_draw_string(paint, PLOT_CORD_X - 11, PLOT_CORD_Y - y[i] - 3, val, &epaper_font_8, 1);
                else
                    w_2in9_draw_string(paint, PLOT_CORD_X - 8, PLOT_CORD_Y  - y[i] - 3, val, &epaper_font_8, 1);
            }

        }

        // xSemaphoreGiveRecursive(ui->ui_mtx);
}
