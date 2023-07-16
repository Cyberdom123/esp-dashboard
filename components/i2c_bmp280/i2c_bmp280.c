#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"

#include "driver/i2c.h"

#include "i2c_bmp280.h"

//i2c esp8266 driver configuration
#define I2C_MASTER_PORT_NUM                 I2C_NUM_0
#define I2C_MASTER_PORT_SDA_IO 4
#define I2C_MASTER_PORT_SCL_IO 5
#define WRITE_BIT                           I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                            I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                        0x1              /*!< I2C master will check ack from slave*/
#define LAST_NACK_VAL                       0x2              /*!< I2C last_nack value */

static const char *TAG = "bmp280";

void bmp280_init_default_params(bmp280_dev *dev) {
	dev->conf.mode = BMP280_MODE_NORMAL;
	dev->conf.filter = BMP280_FILTER_OFF;
	dev->conf.oversampling_pressure = BMP280_STANDARD;
	dev->conf.oversampling_temperature = BMP280_STANDARD;
	dev->conf.standby_time = BMP280_STANDBY_250;
}

esp_err_t i2c_master_init(){
    int ret;
    int i2c_master_port = I2C_MASTER_PORT_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_PORT_SDA_IO;
    conf.sda_pullup_en = 0;
    conf.scl_io_num = I2C_MASTER_PORT_SCL_IO;
    conf.scl_pullup_en = 0;
    conf.clk_stretch_tick = 300; // 300 ticks, Clock stretch is about 210us, you can make changes according to the actual situation.
    ret = i2c_driver_install(i2c_master_port, conf.mode);
    if(ret != ESP_OK){
        return ESP_FAIL;
    }
    ret = i2c_param_config(i2c_master_port, &conf);
    if(ret != ESP_OK){
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t i2c_bmp280_write(i2c_port_t i2c_num, uint8_t reg_addr, uint8_t data){
    int ret;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    //Queue command for I2C master
    i2c_master_start(cmd); 
    i2c_master_write_byte(cmd, BMP280_I2C_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    //Send all queued commands
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

esp_err_t i2c_bmp280_read(i2c_port_t i2c_num, uint8_t reg_addr, uint8_t *data, size_t data_len){
    int ret;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    //Queue command for I2C master
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, BMP280_I2C_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
    i2c_master_stop(cmd);    
    //Send all queued commands
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        return ret;
    }

    cmd = i2c_cmd_link_create();
    //Queue command for I2C master
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, BMP280_I2C_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd, data, data_len, LAST_NACK_VAL); //check nack for last byte
    i2c_master_stop(cmd);    
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

void i2c_bmp280_read_calibration(bmp280_dev *dev){
    uint8_t data1, data0;

    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_T1_LSB_ADDR, &data1, 1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_T1_MSB_ADDR, &data0, 1));
    dev->calib_param.dig_t1 = (uint16_t) data0 << 8 | data1;

    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_T2_LSB_ADDR, &data1, 1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_T2_MSB_ADDR, &data0, 1));
    dev->calib_param.dig_t2 = (uint16_t) data0 << 8 | data1;

    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_T3_LSB_ADDR, &data1, 1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_T3_MSB_ADDR, &data0, 1));
    dev->calib_param.dig_t3 = (uint16_t) data0 << 8 | data1;

    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P1_LSB_ADDR, &data1, 1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P1_MSB_ADDR, &data0, 1));
    dev->calib_param.dig_p1 = (uint16_t) data0 << 8 | data1;

    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P2_LSB_ADDR, &data1, 1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P2_MSB_ADDR, &data0, 1));
    dev->calib_param.dig_p2 = (uint16_t) data0 << 8 | data1;

    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P3_LSB_ADDR, &data1, 1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P3_MSB_ADDR, &data0, 1));
    dev->calib_param.dig_p3 = (uint16_t) data0 << 8 | data1;

    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P4_LSB_ADDR, &data1, 1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P4_MSB_ADDR, &data0, 1));
    dev->calib_param.dig_p4 = (uint16_t) data0 << 8 | data1;

    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P5_LSB_ADDR, &data1, 1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P5_MSB_ADDR, &data0, 1));
    dev->calib_param.dig_p5 = (uint16_t) data0 << 8 | data1;

    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P6_LSB_ADDR, &data1, 1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P6_MSB_ADDR, &data0, 1));
    dev->calib_param.dig_p6 = (uint16_t) data0 << 8 | data1;

    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P7_LSB_ADDR, &data1, 1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P7_MSB_ADDR, &data0, 1));
    dev->calib_param.dig_p7 = (uint16_t) data0 << 8 | data1;
    
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P8_LSB_ADDR, &data1, 1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P8_MSB_ADDR, &data0, 1));
    dev->calib_param.dig_p8 = (uint16_t) data0 << 8 | data1;

    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P9_LSB_ADDR, &data1, 1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_DIG_P9_MSB_ADDR, &data0, 1));
    dev->calib_param.dig_p9 = (uint16_t) data0 << 8 | data1;

}


//Return value is in Pa, 24 integer bits and 8 fractional bits.
static inline uint32_t compensate_pressure(bmp280_dev *dev, int32_t adc_press,
		                                   int32_t fine_temp) 
{
	int64_t var1, var2, p;

	var1 = (int64_t) fine_temp - 128000;
	var2 = var1 * var1 * (int64_t) dev->calib_param.dig_p6;
	var2 = var2 + ((var1 * (int64_t) dev->calib_param.dig_p5) << 17);
	var2 = var2 + (((int64_t) dev->calib_param.dig_p4) << 35);
	var1 = ((var1 * var1 * (int64_t) dev->calib_param.dig_p3) >> 8)
			+ ((var1 * (int64_t) dev->calib_param.dig_p2) << 12);
	var1 = (((int64_t) 1 << 47) + var1) * ((int64_t) dev->calib_param.dig_p1) >> 33;

	if (var1 == 0) {
		return 0;  // avoid exception caused by division by zero
	}

	p = 1048576 - adc_press;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = ((int64_t) dev->calib_param.dig_p9 * (p >> 13) * (p >> 13)) >> 25;
	var2 = ((int64_t) dev->calib_param.dig_p8 * p) >> 19;

	p = ((p + var1 + var2) >> 8) + ((int64_t) dev->calib_param.dig_p7 << 4);
	return p;
}

//Get fine_temp for pressure compensation
//Return value in degrees Celsius.
static inline int32_t compensate_temperature(bmp280_dev *dev, int32_t adc_temp,
		int32_t *fine_temp) {
	int32_t var1, var2;

	var1 = ((((adc_temp >> 3) - ((int32_t) dev->calib_param.dig_t1 << 1)))
			* (int32_t) dev->calib_param.dig_t2) >> 11;
	var2 = (((((adc_temp >> 4) - (int32_t) dev->calib_param.dig_t1)
			* ((adc_temp >> 4) - (int32_t) dev->calib_param.dig_t1)) >> 12)
			* (int32_t) dev->calib_param.dig_t3) >> 14;

	*fine_temp = var1 + var2;
	return (*fine_temp * 5 + 128) >> 8;
}

esp_err_t i2c_bmp280_init(bmp280_dev *dev){
    uint8_t data;
    int ret;
    vTaskDelay(100 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(i2c_master_init());
    
    i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_REG_ID, &data, 1);
    if(!(data == BMP280_CHIP_ID_1 || data == BMP280_CHIP_ID_2 || data == BMP280_CHIP_ID_3)){
        ESP_LOGI(TAG, "BMP280 not found!, device id is %x", data);
        ESP_LOGI(TAG, "BMP280 id's are %x, %x, %x", BMP280_CHIP_ID_1,
                BMP280_CHIP_ID_2, BMP280_CHIP_ID_3);
        return ESP_FAIL;
    }

    //Reset device
    ret = i2c_bmp280_write(I2C_MASTER_PORT_NUM, BMP280_REG_RESET, BMP280_RESET_VALUE);
    if(ret != ESP_OK){
        return ESP_FAIL;
    }

    //Wait for device reboot
    i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_REG_STATUS, &data, 1);
    if(data && 1 == 0){
        return ESP_FAIL;
    }

    //Read calibration register
    i2c_bmp280_read_calibration(dev);

    //note: set the default parameters if they are not set
    if(dev->defoult_conf == true){
        bmp280_init_default_params(dev);
    }

    //Set config register
    uint8_t conf = dev->conf.filter << 2 | dev->conf.standby_time << 5;
    ret = i2c_bmp280_write(I2C_MASTER_PORT_NUM, BMP280_REG_CONFIG, conf);
    if(ret != ESP_OK){
        return ESP_FAIL;
    }

    //Set control register
    uint8_t ctrl = dev->conf.mode | dev->conf.oversampling_pressure << 2
                   | dev->conf.oversampling_temperature << 5;
    ret = i2c_bmp280_write(I2C_MASTER_PORT_NUM, BMP280_REG_CTRL, ctrl);
    if(ret != ESP_OK){
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t i2c_bmp280_burst_read_raw(bmp280_dev *dev, int32_t *temp, uint32_t *press){
    //The BMP280 does not have a humidity reading,
    //so we need to read 6 bytes of data.
    int ret;
    uint8_t data[6];
    ret = i2c_bmp280_read(I2C_MASTER_PORT_NUM, BMP280_REG_PRESS_MSB, data, 6);
    if(ret != ESP_OK){
        return ESP_FAIL;
    }

    *press = data[0] << 12 | data[1] << 4 | data[2] >> 4;
	*temp = data[3] << 12 | data[4] << 4 | data[5] >> 4;

    int32_t fine_temp;
	*temp = compensate_temperature(dev, *temp, &fine_temp);
	*press = compensate_pressure(dev, *press, fine_temp);

    return ESP_OK;
}

esp_err_t i2c_bmp280_burst_read(bmp280_dev *dev){
    int32_t temp;
    uint32_t press;
    int ret = i2c_bmp280_burst_read_raw(dev, &temp, &press);
    dev->temp = (float) temp / 100;
    dev->press = (float) press / 256;
    return ret;
}

//Note:
// - Find way to check if its struct is unset