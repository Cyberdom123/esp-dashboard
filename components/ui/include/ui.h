#ifndef UI
#define UI

#define PLOT_LENGTH 186
#define PLOT_WIDTH  88

#define PLOT_CORD_X 105
#define PLOT_CORD_Y 98
#define PLOT_DATA_LENGTH 24

//offset from the end of the x-axis
#define PLOT_END_OFFSET 7

#define PLOT_XDATA_SPACING 7
#define PLOT_YDATA_SPACING 4
typedef struct 
{
    float x_data[PLOT_DATA_LENGTH];    
    float y_data[PLOT_DATA_LENGTH];    
}ui_plot;

typedef struct 
{
    char in_temp[10];
    char in_press[11];

    char out_temp[10]; 
    char out_press[11]; 
    char out_hum[10];
    char pm25[13];
    char pm10[13];
    char date[100];
    char description[64];

    ui_plot plot;
    xSemaphoreHandle ui_mtx;
}ui_weather;

void ui_init(w_2in9 *e_paper, w_2in9_paint *paint, ui_weather *ui);
void ui_weather_data_update(w_2in9 *e_paper, w_2in9_paint *paint, ui_weather *ui);
void static_ui_data_update(w_2in9 *e_paper, w_2in9_paint *paint, ui_weather *ui);
void ui_draw_plot(w_2in9 *e_paper, w_2in9_paint *paint, ui_plot *plot);

#endif