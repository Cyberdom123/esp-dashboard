#ifndef I2C_BMP280
#define I2C_BMP280

#include "bmp280_defs.h"

esp_err_t i2c_master_init();

esp_err_t i2c_bmp280_write(i2c_port_t i2c_num, uint8_t reg_addr, uint8_t data);

esp_err_t i2c_bmp280_read(i2c_port_t i2c_num, uint8_t reg_addr, uint8_t *data, size_t data_len);

esp_err_t i2c_bmp280_init(bmp280_dev *dev);

esp_err_t i2c_bmp280_burst_read(bmp280_dev *dev);

esp_err_t i2c_bmp280_test();

#endif