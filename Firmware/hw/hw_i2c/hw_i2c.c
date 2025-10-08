#include "hw_i2c.h"
#include "driver/i2c.h"
#include "freertos/semphr.h"

SemaphoreHandle_t i2c_mutex;

void hw_i2c_init(void)
{
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_21,
        .scl_io_num = GPIO_NUM_22,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 100000
    };

    i2c_param_config(I2C_NUM_0, &i2c_config);

    i2c_driver_install(I2C_NUM_0, i2c_config.mode, 0, 0, 0);
    i2c_mutex = xSemaphoreCreateMutex();
}

esp_err_t hw_i2c_write(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, size_t data_len)
{
    esp_err_t ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write(cmd, data, data_len, true);
    i2c_master_stop(cmd);
    xSemaphoreTake(i2c_mutex, portMAX_DELAY);
    ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
    xSemaphoreGive(i2c_mutex);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t hw_i2c_read(uint8_t device_addr, uint8_t reg_addr, uint8_t *data, size_t data_len)
{
    esp_err_t ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_READ, true);
    if (data_len > 1) {
        i2c_master_read(cmd, data, data_len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + data_len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    xSemaphoreTake(i2c_mutex, portMAX_DELAY);
    ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
    xSemaphoreGive(i2c_mutex);
    i2c_cmd_link_delete(cmd);
    return ret;
}

