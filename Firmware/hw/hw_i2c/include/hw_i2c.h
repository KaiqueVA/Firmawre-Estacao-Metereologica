#ifndef HW_I2C_H
#define HW_I2C_H

#include "esp_err.h"

void hw_i2c_init(void);
esp_err_t hw_i2c_write(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, size_t data_len);
esp_err_t hw_i2c_read(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, size_t data_len);

#endif //HW_I2C_H