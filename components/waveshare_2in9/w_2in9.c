#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"

#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/spi.h"

#include "w_2in9.h"
#include "w_2in9_paint.h"

static const char *TAG = "driver";

/* LUT MEMORY */
unsigned char WS_20_30[159] =
{											
0x80,	0x66,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x40,	0x0,	0x0,	0x0,
0x10,	0x66,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x20,	0x0,	0x0,	0x0,
0x80,	0x66,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x40,	0x0,	0x0,	0x0,
0x10,	0x66,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x20,	0x0,	0x0,	0x0,
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,
0x14,	0x8,	0x0,	0x0,	0x0,	0x0,	0x1,					
0xA,	0xA,	0x0,	0xA,	0xA,	0x0,	0x1,					
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,					
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,					
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,					
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,					
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,					
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,					
0x14,	0x8,	0x0,	0x1,	0x0,	0x0,	0x1,					
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x1,					
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,					
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,					
0x44,	0x44,	0x44,	0x44,	0x44,	0x44,	0x0,	0x0,	0x0,			
0x22,	0x17,	0x41,	0x0,	0x32,	0x36
};

/* PARTIAL LUT MEMORY */
unsigned char _WF_PARTIAL_2IN9[159] =
{
0x0,0x40,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x80,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x40,0x40,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0A,0x0,0x0,0x0,0x0,0x0,0x2,  
0x1,0x0,0x0,0x0,0x0,0x0,0x0,
0x1,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x22,0x22,0x22,0x22,0x22,0x22,0x0,0x0,0x0,
0x22,0x17,0x41,0xB0,0x32,0x36,
};


esp_err_t gpio_init(w_2in9 *dev){
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << dev->reset_pin | 1ULL << dev->dc_pin);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << dev->busy_pin);
    io_conf.pull_down_en = 1;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    return ESP_OK;
}

esp_err_t spi_master_init(w_2in9 *dev){
    spi_config_t spi_config;

    spi_config.interface.cs_en = 1;
    spi_config.interface.miso_en = 0; //w_2in9 doesn't use miso
    spi_config.interface.mosi_en = 1;
    spi_config.interface.byte_rx_order = 0;
    spi_config.interface.byte_tx_order = 0;
    spi_config.interface.cpha = 0;
    spi_config.interface.cpol = 0;


    spi_config.mode = SPI_MASTER_MODE;
    spi_config.intr_enable.val = 0;
    spi_config.clk_div = SPI_2MHz_DIV; //4 MHz clk signal
    spi_config.event_cb = NULL;

    ESP_ERROR_CHECK(spi_init(HSPI_HOST, &spi_config));

    return ESP_OK;
}

esp_err_t w_2in9_idle(w_2in9 *dev){
    int cnt = 0; 
    while (gpio_get_level(dev->busy_pin) == 1)
    {
        cnt++;
        vTaskDelay(10/ portTICK_RATE_MS);
        if(cnt == 100){
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

esp_err_t w_2in9_reset(w_2in9 *dev){
    int ret;

    xSemaphoreTakeRecursive(dev->spi_mutx, portMAX_DELAY); 
    gpio_set_level(dev->reset_pin, 1);
    ets_delay_us(200);
    gpio_set_level(dev->reset_pin, 0);
    ets_delay_us(200);
    gpio_set_level(dev->reset_pin, 1);
    ret = w_2in9_idle(dev);
    if(ret != ESP_OK){
        return ESP_FAIL;
    }
    xSemaphoreGiveRecursive(dev->spi_mutx);

    return ESP_OK;
}

esp_err_t spi_master_send(w_2in9 *dev, uint8_t *data, int len){
    spi_trans_t spi_transaction;
    spi_transaction.bits.val = 0;
    spi_transaction.bits.mosi = len * 8;
    spi_transaction.mosi = (uint32_t*) data;

    return spi_trans(HSPI_HOST, &spi_transaction);
}

esp_err_t w_2in9_send_command(w_2in9 *dev, uint8_t command){
    gpio_set_level(dev->dc_pin, 0);
    return spi_master_send(dev, &command, 1);
}

esp_err_t w_2in9_send_byte(w_2in9 *dev, uint8_t byte){
    gpio_set_level(dev->dc_pin, 1);
    return spi_master_send(dev, &byte, 1);
}

esp_err_t w_2in9_send_data(w_2in9 *dev, uint8_t *data, size_t len){
    gpio_set_level(dev->dc_pin, 1);
    // if(len > 32){
    //     uint32_t length_32_bytes = len/32;
    //     uint32_t residue = len % 32;
    //     uint8_t data_packet[32]; 

    //     for (size_t i = 0; i < length_32_bytes; i++)
    //     {
    //         for (size_t j = 0; j < 32; j++)
    //         {
    //             data_packet[j] = data[j + i*32];
    //         }
    //         spi_master_send(dev, data_packet, 32);
    //     }
    //     if (residue > 0)
    //     {
    //         for (size_t j = 0; j < 32; j++)
    //         {
    //             data_packet[j] = data[j + length_32_bytes*32];
    //         }
    //         spi_master_send(dev, data_packet, residue);
    //     }
    // }else{
    //     return spi_master_send(dev, data, len);
    // }

    for (size_t i = 0; i < len; i++)
    {
        w_2in9_send_byte(dev, data[i]);
    }
    

    return ESP_OK;
}

esp_err_t w_2in9_set_memory_area(w_2in9 *dev, uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end){
    xSemaphoreTakeRecursive(dev->spi_mutx, portMAX_DELAY);
    w_2in9_send_command(dev, 0x44);
    w_2in9_send_byte(dev, (uint8_t) ((x_start >> 3) & 0xFF));
    w_2in9_send_byte(dev, (uint8_t) (x_end >> 3) & 0xFF);
    w_2in9_send_command(dev, 0x45);
    w_2in9_send_byte(dev, (uint8_t) y_start & 0xFF);
    w_2in9_send_byte(dev, (uint8_t) (y_start >> 8) & 0xFF);
    w_2in9_send_byte(dev, (uint8_t) y_end & 0xFF);
    w_2in9_send_byte(dev, (uint8_t) (y_end >> 8) & 0xFF);
    xSemaphoreGiveRecursive(dev->spi_mutx);

    return ESP_OK;
}

esp_err_t w_2in9_set_memory_pointer(w_2in9 *dev, int x, int y){
    xSemaphoreTakeRecursive(dev->spi_mutx, portMAX_DELAY);
    w_2in9_send_command(dev, 0x4E);
    w_2in9_send_byte(dev, (x >> 3) & 0xFF);
    w_2in9_send_command(dev, 0x4F);
    w_2in9_send_byte(dev, y & 0xFF);
    w_2in9_send_byte(dev, (y >> 8) & 0xFF);
    w_2in9_idle(dev);
    xSemaphoreGiveRecursive(dev->spi_mutx);
    
    return ESP_OK;
}

esp_err_t w_2in9_set_lut(w_2in9 *dev, uint8_t* lut){
    xSemaphoreTakeRecursive(dev->spi_mutx, portMAX_DELAY);
    w_2in9_send_command(dev, E_PAPER_LUT_REGISTER);
    w_2in9_send_data(dev, (uint8_t*) lut, 153);
    xSemaphoreGiveRecursive(dev->spi_mutx);
    return ESP_OK;
}

esp_err_t w_2in9_set_lut_by_host(w_2in9  *dev){
    w_2in9_set_lut(dev, WS_20_30);
    w_2in9_send_command(dev, 0x3f);
	w_2in9_send_byte(dev, *(WS_20_30+153));
	w_2in9_send_command(dev, 0x03);	// gate voltage
	w_2in9_send_byte(dev,*(WS_20_30+154));
	w_2in9_send_command(dev, 0x04);	// source voltage
	w_2in9_send_byte(dev, *(WS_20_30+155));	// VSH
	w_2in9_send_byte(dev, *(WS_20_30+156));	// VSH2
	w_2in9_send_byte(dev, *(WS_20_30+157));	// VSL
	w_2in9_send_command(dev, E_PAPER_VCOM_REG);	// VCOM
	w_2in9_send_byte(dev, *(WS_20_30+158));
    return ESP_OK;
}


esp_err_t w_2in9_init(w_2in9 *dev){ //in build
    int ret;
    dev->spi_mutx = xSemaphoreCreateRecursiveMutex();

    ESP_ERROR_CHECK(gpio_init(dev));
    ESP_ERROR_CHECK(spi_master_init(dev));

    ret = w_2in9_reset(dev);
    if(ret != ESP_OK){
        return ESP_FAIL;
    }

    //xSemaphoreTakeRecursive(dev->spi_mutx, portMAX_DELAY);
    
    w_2in9_send_command(dev, E_PAPER_SOFTWARE_RESET);
    w_2in9_idle(dev);

    /* ----------------- INIT CODE ------------------- */
    w_2in9_send_command(dev, 0x01); //Deiver output control
    w_2in9_send_byte(dev, 0x27);
    w_2in9_send_byte(dev, 0x01);
    w_2in9_send_byte(dev, 0x00);
    /* ----------------------------------------------- */

    w_2in9_send_command(dev, 0x11); //Dispaly update control
    w_2in9_send_byte(dev, 0x03);

    w_2in9_set_memory_area(dev, 0, dev->width, 0, dev->height);

    w_2in9_send_command(dev, 0x21); //Display update control
    w_2in9_send_byte(dev, 0x00);
    w_2in9_send_byte(dev, 0x80);


    w_2in9_set_memory_pointer(dev, 0, 0);

    w_2in9_set_lut_by_host(dev);

    //xSemaphoreGiveRecursive(dev->spi_mutx);

    return ESP_OK;
}


void w_2in9_reinit(w_2in9 *dev){
    xSemaphoreTakeRecursive(dev->spi_mutx, portMAX_DELAY);
    int ret = w_2in9_reset(dev);
    if(ret != ESP_OK){
        return ESP_FAIL;
    }
    
    w_2in9_send_command(dev, E_PAPER_SOFTWARE_RESET);
    w_2in9_idle(dev);

    /* ----------------- INIT CODE ------------------- */
    w_2in9_send_command(dev, 0x01); //Deiver output control
    w_2in9_send_byte(dev, 0x27);
    w_2in9_send_byte(dev, 0x01);
    w_2in9_send_byte(dev, 0x00);
    /* ----------------------------------------------- */

    w_2in9_send_command(dev, 0x11); //Dispaly update control
    w_2in9_send_byte(dev, 0x03);

    w_2in9_set_memory_area(dev, 0, dev->width, 0, dev->height);

    w_2in9_send_command(dev, 0x21); //Display update control
    w_2in9_send_byte(dev, 0x00);
    w_2in9_send_byte(dev, 0x80);


    w_2in9_set_memory_pointer(dev, 0, 0);

    w_2in9_set_lut_by_host(dev);
    xSemaphoreGiveRecursive(dev->spi_mutx);
}

/* @brief: Bit set - white, Bit reset - black */
/* To Do: send data as uint32_t for better speed */
void w_2in9_clear_frame_memory(w_2in9 *dev, uint8_t color){
    xSemaphoreTakeRecursive(dev->spi_mutx, portMAX_DELAY);
    w_2in9_set_memory_area(dev, 0, dev->width-1, 0, dev->height-1);
    w_2in9_set_memory_pointer(dev, 0, 0);
    w_2in9_send_command(dev, E_PAPER_RAM_WRITE);

    for (int i = 0; i < dev->width / 8 * dev->height ; i++)
    {
        w_2in9_send_byte(dev, color);
    }
    xSemaphoreGiveRecursive(dev->spi_mutx);
}

void w_2in9_display_frame_update(w_2in9 *dev){
    xSemaphoreTakeRecursive(dev->spi_mutx, portMAX_DELAY);
    w_2in9_send_command(dev, E_PAPER_START_DISP_REG);
    w_2in9_send_byte(dev, E_PAPER_START_DISP);
    w_2in9_send_command(dev, E_PAPER_ACTIVATE);
    w_2in9_idle(dev);
    xSemaphoreGiveRecursive(dev->spi_mutx);
}


void w_2in9_set_frame_memory(w_2in9_paint *paint, w_2in9 *dev, int x,
                             int y, int image_width, int image_height)
{
    unsigned char* image_buffer = paint->image;
    int x_end;
    int y_end;
    
    if (
        image_buffer == NULL ||
        x < 0 || image_width < 0 ||
        y < 0 || image_height < 0
    ) {
        ESP_LOGI(TAG, "INAGE BUFFER EMPTY");
        return;
    }

    x &= 0xF8;
    image_width &= 0xF8;
    if (x + image_width >= dev->width) {
        x_end = dev->width - 1;
    } else {
        x_end = x + image_width - 1;
    }
    if (y + image_height >= dev->height) {
        y_end = dev->height - 1;
    } else {
        y_end = y + image_height - 1;
    }

    w_2in9_set_memory_area(dev, x, x_end, y, y_end);
    w_2in9_set_memory_pointer(dev, x, y);
    w_2in9_send_command(dev, E_PAPER_RAM_WRITE);
    
    /* send the image data */
    for (int j = 0; j < y_end - y + 1; j++) {
        for (int i = 0; i < (x_end - x + 1) / 8; i++) {
            w_2in9_send_byte(dev, image_buffer[i + j * (image_width / 8)]);
        }
    }

}

void w_2in9_set_frame_memory_base(w_2in9_paint *paint, w_2in9 *dev, int x,
                             int y, int image_width, int image_height)
{
    unsigned char* image_buffer = paint->image;
    int x_end;
    int y_end;
    
    if (
        image_buffer == NULL ||
        x < 0 || image_width < 0 ||
        y < 0 || image_height < 0
    ) {
        ESP_LOGI(TAG, "INAGE BUFFER EMPTY");
        return;
    }

    x &= 0xF8;
    image_width &= 0xF8;
    if (x + image_width >= dev->width) {
        x_end = dev->width - 1;
    } else {
        x_end = x + image_width - 1;
    }
    if (y + image_height >= dev->height) {
        y_end = dev->height - 1;
    } else {
        y_end = y + image_height - 1;
    }

    w_2in9_set_memory_area(dev, x, x_end, y, y_end);
    w_2in9_set_memory_pointer(dev, x, y);
    w_2in9_send_command(dev, E_PAPER_RAM_WRITE);

    /* send the image data */
    for (int j = 0; j < y_end - y + 1; j++) {
        for (int i = 0; i < (x_end - x + 1) / 8; i++) {
            w_2in9_send_byte(dev, image_buffer[i + j * (image_width / 8)]);
        }
    }

    w_2in9_send_command(dev, E_PAPER_SECOND_RAM_WRITE);
    /* send the image data to second ram */
    for (int j = 0; j < y_end - y + 1; j++) {
        for (int i = 0; i < (x_end - x + 1) / 8; i++) {
            w_2in9_send_byte(dev, image_buffer[i + j * (image_width / 8)]);
        }
    }

}


void w_2in9_set_frame_partial(w_2in9_paint *paint, w_2in9 *dev, int x,
                             int y, int image_width, int image_height)
{
    unsigned char* image_buffer = paint->image;
    int x_end;
    int y_end;
    
    xSemaphoreTakeRecursive(dev->spi_mutx, portMAX_DELAY);
    if (
        image_buffer == NULL ||
        x < 0 || image_width < 0 ||
        y < 0 || image_height < 0
    ) {
        ESP_LOGI(TAG, "INAGE BUFFER EMPTY");
        return;
    }

    x &= 0xF8;
    image_width &= 0xF8;
    if (x + image_width >= dev->width) {
        x_end = dev->width - 1;
    } else {
        x_end = x + image_width - 1;
    }
    if (y + image_height >= dev->height) {
        y_end = dev->height - 1;
    } else {
        y_end = y + image_height - 1;
    }

    gpio_set_level(dev->reset_pin, 0);
    ets_delay_us(600);
    gpio_set_level(dev->reset_pin, 1);
    ets_delay_us(600);

    w_2in9_set_lut(dev, _WF_PARTIAL_2IN9);
    w_2in9_send_command(dev, E_PAPER_VCOM_OTP_SELECTION);
	w_2in9_send_byte(dev, 0x00);  
	w_2in9_send_byte(dev, 0x00);  
	w_2in9_send_byte(dev, 0x00);  
	w_2in9_send_byte(dev, 0x00); 
	w_2in9_send_byte(dev, 0x00);  	
	w_2in9_send_byte(dev, 0x40);  
	w_2in9_send_byte(dev, 0x00);  
	w_2in9_send_byte(dev, 0x00);   
	w_2in9_send_byte(dev, 0x00);  
	w_2in9_send_byte(dev, 0x00);

	w_2in9_send_command(dev, 0x3C); //BorderWavefrom
	w_2in9_send_byte(dev, 0x80);	

	w_2in9_send_command(dev, E_PAPER_START_DISP_REG); 
	w_2in9_send_byte(dev, 0xC0);   
	w_2in9_send_command(dev, E_PAPER_ACTIVATE); 
	w_2in9_idle(dev);  
	
    w_2in9_set_memory_area(dev, x, x_end, y, y_end);
    w_2in9_set_memory_pointer(dev, x, y);
    w_2in9_send_command(dev, E_PAPER_RAM_WRITE);

    for (int j = 0; j < y_end - y + 1; j++) {
        for (int i = 0; i < (x_end - x + 1) / 8; i++) {
            w_2in9_send_byte(dev, image_buffer[i + j * (image_width / 8)]);
        }
    }
    xSemaphoreGiveRecursive(dev->spi_mutx);
}

void w_2in9_display_frame_update_partial(w_2in9 *dev){
    xSemaphoreTakeRecursive(dev->spi_mutx, portMAX_DELAY);
    w_2in9_send_command(dev, E_PAPER_START_DISP_REG);
    w_2in9_send_byte(dev, 0x0F);
    w_2in9_send_command(dev, E_PAPER_ACTIVATE);
    w_2in9_idle(dev);
    xSemaphoreGiveRecursive(dev->spi_mutx);
}
