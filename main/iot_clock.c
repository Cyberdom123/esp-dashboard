// requirements
#include <stdio.h>
#include <string.h>
// system
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "ringbuf.h"
#include "esp8266/spi_struct.h"
#include "esp8266/gpio_struct.h"
#include "esp_system.h"
#include "esp_log.h"
// bmp280
#include "driver/gpio.h"
#include "driver/i2c.h"
// http_req
#include <netdb.h>
#include <sys/socket.h>
// edp 
#include "i2c_bmp280.h"
#include "w_2in9.h"
#include "w_2in9_paint.h"
#include "fonts.h"
#include "dashboard_api.h"
#include "ui.h"
// log
#include "esp_vfs.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "json_logger.h"
#include "frozen.h"
#include <time.h>
// forecast
#include "simple_forecast.h"

static const char *TAG = "iot_dashboard";

xSemaphoreHandle edp_sem, ui_sem;

/* EDP */
ui_weather ui;

char in_temp_now[10] = "0.0";
char in_press_now[10] = "0.0";
char out_temp[6] = "0.0";
char out_press[5] = "0";
char out_hum[3] = "0";
char desc[9] = "none";

char pm_10[6] = "0";
char pm_2_5[6] = "0.0";
char aqi[4] = "0.0";

int hour = 0;
int next_hour = -1, next_hour_log = -1; //init -1 val, set on the begining of json read

static httpd_handle_t server = NULL;

/* http req */
#define WEB_SERVER "api.openweathermap.org"
#define WEB_URL "http://api.openweathermap.org/data/2.5/weather"

#define WEB_URL_AIR "http://api.openweathermap.org/data/2.5/air_pollution"
// Location ID to get the weather data for
#define LOCATION_ID "3094802"
#define LAT "50.0693"
#define LON "19.911"

static const char *REQUEST = "GET "
    WEB_URL"?id="LOCATION_ID"&appid=f654aab5728f541a667b8d63ec764246&units=metric HTTP/1.1\n"
    "Host: "WEB_SERVER"\n"
    "Connection: close\n"
    "User-Agent: esp-idf/1.0 esp32\n"
    "\n";

static const char *REQUEST_AIR = "GET "
    WEB_URL_AIR"?lat="LAT"&lon="LON"&appid=f654aab5728f541a667b8d63ec764246&units=metric HTTP/1.1\n"
    "Host: "WEB_SERVER"\n"
    "Connection: close\n"
    "User-Agent: esp-idf/1.0 esp32\n"
    "\n";


void e_paper_task(){
    w_2in9 e_paper;
    w_2in9_paint paint;
    //xSemaphoreTakeRecursive(edp_sem, portMAX_DELAY);
    
    //EDP
    vTaskDelay(20000/portTICK_RATE_MS);
    ui_init(&e_paper, &paint, &ui);

    /* PART OF CODE TO TEST PARTIAL UPDATES */
    xSemaphoreTakeRecursive(ui_sem, portMAX_DELAY);
    strcpy(ui.in_temp, "00.00C");
    strcpy(ui.in_press, "0000.0hpa");
    xSemaphoreGiveRecursive(ui_sem);

    if(ui.out_temp == NULL){
        strcpy(ui.out_temp, "00.00C");
        strcpy(ui.out_press, "0000.0hpa");
        strcpy(ui.out_hum, "00% hum");
    }

    ui_weather_data_update(&e_paper, &paint, &ui);
    //xSemaphoreGiveRecursive(edp_sem);
    while (1)
    {
        xSemaphoreTakeRecursive(edp_sem, portMAX_DELAY);
        if(hour != next_hour){
            ui_weather_data_update(&e_paper, &paint, &ui);
        }else{
            ESP_LOGI(TAG, "STATIC UPDATE TEST");
            //get_data_array("/spiffs/weather_log.json", ".pm10_hour", &ui.plot.y_data);
            
            for (size_t i = 0; i < 24; i++)
            {
                ESP_LOGI(TAG, "%f", ui.plot.y_data[i]);
            }
            

            static_ui_data_update(&e_paper, &paint, &ui);
            next_hour = hour + 1;
            if(next_hour == 24){
                next_hour = 0;
            }
            vTaskDelay(1000/portTICK_RATE_MS);
        }

        xSemaphoreGiveRecursive(edp_sem);

        vTaskDelay(2000 / portTICK_RATE_MS);
    }
   
}

void i2c_bmp280_task(){
   xSemaphoreTakeRecursive(edp_sem, portMAX_DELAY);
   ESP_LOGI(TAG, "TASK 2");
   bmp280_dev dev;
   dev.defoult_conf = true;
   int ret = i2c_bmp280_init(&dev);
   if(ret == ESP_OK){
      ESP_LOGI(TAG,"bmp280 initialization succeed");
   }
   xSemaphoreGiveRecursive(edp_sem);

    while (1)
    {
        i2c_bmp280_burst_read(&dev);

        xSemaphoreTakeRecursive(ui_sem, portMAX_DELAY);
        sprintf(ui.in_temp, "%0.01fC", dev.temp);
        sprintf(ui.in_press, "%0.1fhpa", dev.press/100); //add strcat C and hpa
        sprintf(in_temp_now, "%0.01f", dev.temp);
        sprintf(in_press_now, "%0.1f", dev.press/100);
        xSemaphoreGiveRecursive(ui_sem);

        vTaskDelay(600/portTICK_RATE_MS);
    }
}


void loger_task(){
    while(1){

        xSemaphoreTakeRecursive(edp_sem, portMAX_DELAY);
        ESP_LOGI(TAG, "Current temp is: %s and pressure is: %s", in_temp_now, in_press_now);
        log_data("/spiffs/weather_log.json", ".in_temp_now", in_temp_now, 24);
        log_data("/spiffs/weather_log.json", ".in_press_now", in_press_now, 24);
        log_data("/spiffs/weather_log.json", ".out_temp", out_temp, 24);
        log_data("/spiffs/weather_log.json", ".out_press", out_press, 24);
        log_data("/spiffs/weather_log.json", ".out_hum", out_hum, 24);

        ESP_LOGI(TAG, " Logging data");


        int fill = 0;
        float pm10_predicted, plot_data[24] = {0};
        char value[10];
        if(hour == next_hour_log){
            ESP_LOGI(TAG, "Time for log, %d", next_hour_log);
            log_data_array("/spiffs/weather_log.json", ".aqi_hour", aqi, 24);
            //remove predicted data and insert accual pm_10
            //log_data_array("/spiffs/weather_log.json", ".pm10_hour", pm_10, 24);             
            log_data_array("/spiffs/weather_log.json", ".pm25_hour", pm_2_5, 24);

            /* Forecast */
            get_data_array("/spiffs/weather_log.json", ".pm10_hour", &plot_data);
            for (size_t i = 0; i < 24; i++)
            {  

               if(plot_data[i] != 0){
                fill++;
               }
            }
            set_data_array("/spiffs/weather_log.json", ".pm10_hour", pm_10, 24, fill-1);
            ESP_LOGI(TAG, "Prediction parameters: %s, %f, %f", pm_10, plot_data[fill-2], plot_data[fill-3]);

            pm10_predicted = predict_pm10(atof(out_temp), atof(out_hum), atof(in_press_now), atof(pm_10), plot_data[fill-2], plot_data[fill-3]);
            ESP_LOGI(TAG, "pm10_predicted: %0.01f", pm10_predicted);
            sprintf(value, "%0.01f", pm10_predicted);
            log_data_array("/spiffs/weather_log.json", ".pm10_hour", value, 24); 
            fill = 0;
            /* end Forecast */

            // log_data("/spiffs/weather_log.json", ".refresh_cnt", hour, 24);
            next_hour_log = hour + 1;
        }
        
        xSemaphoreTakeRecursive(ui_sem, portMAX_DELAY);        
        for (size_t i = 0; i < 24; i++)
        {
            ui.plot.y_data[i] = 0;
        }

        get_data_array("/spiffs/weather_log.json", ".pm10_hour", &ui.plot.y_data);
        xSemaphoreGiveRecursive(ui_sem);
        
        xSemaphoreGiveRecursive(edp_sem);
        vTaskDelay(10000 / portTICK_RATE_MS);

    }
}

void debug_task(){
    while(1){
        ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
        vTaskDelay(10000 / portTICK_RATE_MS);
    }  
}

static void http_get_task(void *pvParameters)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    //struct in_addr *addr;
    int s, r;
    char recv_buf[4000];
    char *ptr;
    struct json_token t;

    struct tm  ts;
    char buf[80];

    while(1) {
        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... set socket receiving timeout success");

        /* Read HTTP response */
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf)-1);

        
        /* Get weather data from JSON */        
        if(r>0){
            ptr = strchr(recv_buf, '{');
            if(ptr != NULL){
                char *value;
                ESP_LOGI(TAG,"%s", ptr);
                json_scanf(ptr, strlen(ptr), "{main:{temp:%Q}}", &value);
                memcpy(out_temp, value, strlen(value));
                free(value);

                json_scanf(ptr, strlen(ptr), "{main:{pressure:%Q}}", &value);
                memcpy(out_press, value, strlen(value));
                free(value);
                
                json_scanf(ptr, strlen(ptr), "{main:{humidity:%Q}}", &value);
                memcpy(out_hum, value, strlen(value));
                free(value);

                for (int i = 0; json_scanf_array_elem(ptr, strlen(ptr), ".weather", i, &t) > 0; i++) {
                    // t.type == JSON_TYPE_OBJECT
                    json_scanf(t.ptr, t.len, "{main:%Q}", &value);  // value is 123, then 345
                }
                memcpy(desc, value, strlen(value));
                free(value);

                ESP_LOGI(TAG, "The out_temp is: %s and pres: %s", out_temp, out_press);
                xSemaphoreTakeRecursive(ui_sem, portMAX_DELAY);
                sprintf(ui.out_temp, "%sC", out_temp);
                sprintf(ui.out_press, "%shpa", out_press);
                sprintf(ui.out_hum, "%s%% hum", out_hum);
                sprintf(ui.description, "%s", desc);
                xSemaphoreGiveRecursive(ui_sem);
            }
        }
        
        close(s);
        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);

        vTaskDelay(2000/portTICK_RATE_MS);

        err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);


        if (write(s, REQUEST_AIR, strlen(REQUEST_AIR)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");

        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... set socket receiving timeout success");

        /* Read HTTP response */
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf)-1);


        // /* Get data from JSON */
        if(r>0){
            char *pm_10_tmp, *pm_2_5_tmp, *aqi_tmp;
            int rawtime = 0;
            ptr = strchr(recv_buf, '{');
            if(ptr != NULL){
                ESP_LOGI(TAG,"%s", ptr);
                for (int i = 0; json_scanf_array_elem(ptr, strlen(ptr), ".list", i, &t) > 0; i++) {
                    // t.type == JSON_TYPE_OBJECT
                    json_scanf(t.ptr, t.len, "{dt:%d}",  &rawtime); 
                    json_scanf(t.ptr, t.len, "{components:{pm10:%Q}}",  &pm_10_tmp);
                    json_scanf(t.ptr, t.len, "{components:{pm2_5:%Q}}",  &pm_2_5_tmp); 
                    json_scanf(t.ptr, t.len, "{main:{aqi:%Q}}",  &aqi_tmp); 
                }
                memcpy(pm_10, pm_10_tmp, strlen(pm_10_tmp));
                memcpy(pm_2_5, pm_2_5_tmp, strlen(pm_2_5_tmp));
                memcpy(aqi, aqi_tmp, strlen(aqi_tmp));
                free(pm_10_tmp);
                free(pm_2_5_tmp);
                free(aqi_tmp);
                ESP_LOGI(TAG, "PM10:%s PM2_5:%s AQI:%s", pm_10, pm_2_5, aqi);

                rawtime = rawtime + 7200; //time zone correction
                ts = *localtime(&rawtime);

                strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ts);
                hour = ts.tm_hour;

                if(next_hour == -1){ //initalize next_hour
                    next_hour = hour + 1;
                    next_hour_log = hour;
                    if(next_hour == 24){
                        next_hour = 0;
                    }
                }

                free(&rawtime);
                ESP_LOGI(TAG, "%s, next update: %d", buf, next_hour);

                xSemaphoreTakeRecursive(ui_sem, portMAX_DELAY);
                sprintf(ui.pm10, "PM10:%s", pm_10);
                sprintf(ui.pm25, "PM2_5:%s", pm_2_5);
                sprintf(ui.date, "PM2.5 %s", buf);
                xSemaphoreGiveRecursive(ui_sem);
            }
        }

        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);

        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(30000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");

    }
}

void app_main(void)
{
   edp_sem = xSemaphoreCreateRecursiveMutex();
   ui_sem = xSemaphoreCreateRecursiveMutex();

   //Web
   ESP_ERROR_CHECK(nvs_flash_init());
   ESP_ERROR_CHECK(esp_netif_init());
   ESP_ERROR_CHECK(esp_event_loop_create_default());
   ESP_ERROR_CHECK(example_connect());

   //Server
   ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
   ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler,
                                              &server));

   server = start_webserver();
   
   //SPIFFS
   spiffs_init();



    create_json("/spiffs/weather_log.json");

    //initialize data
    char value[10]; char cnt[3]; int y_data;
    for (size_t i = 0; i < 12; i++)
    {
        y_data =  rand() % 9 + 13;
        //ui.plot.y_data[i] = y_data;
        //ui.plot.x_data[i] = i-1;
        sprintf(value, "%d", y_data);
        sprintf(cnt, "%d", i);
        log_data_array("/spiffs/weather_log.json", ".pm10_hour", value, 24);
        log_data_array("/spiffs/weather_log.json", ".pm25_hour", value, 24);
        log_data_array("/spiffs/weather_log.json", ".aqi_hour", "3", 24);
        log_data("/spiffs/weather_log.json", ".refresh_cnt", cnt, 24);
    }

    for (size_t i = 0; i < 24; i++)
    {
        ui.plot.x_data[i] = i;
    }
    // sprintf(value, "%d", 24);
    set_data_array("/spiffs/weather_log.json", ".pm10_hour", value, 24, 11);
    get_data_array("/spiffs/weather_log.json", ".pm10_hour", &ui.plot.y_data);

   xTaskCreate(e_paper_task, "e_paper_task", 4096, NULL, 10, NULL);
   xTaskCreate(i2c_bmp280_task, "i2c_master_bmp280_init", 3048, NULL, 1, NULL);
   xTaskCreate(&http_get_task, "http_get_task", 16384, NULL, 5, NULL);
   xTaskCreate(loger_task, "loger_task", 6096, NULL, 2, NULL);
   xTaskCreate(debug_task, "test_task", 2048, NULL, 3, NULL);


}