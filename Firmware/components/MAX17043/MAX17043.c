#include <stdio.h>
#include "MAX17043.h"
#include "hw_i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "data_processing.h"

#define MAX17043_I2C_ADDR               0x36  // 7-bit I2C address

#define MAX17043_REG_VCELL              0x02  // 0x02–0x03: tensão da célula
#define MAX17043_REG_SOC                0x04  // 0x04–0x05: state of charge (%)
#define MAX17043_REG_MODE               0x06  // 0x06–0x07: modo/comandos especiais
#define MAX17043_REG_VERSION            0x08  // 0x08–0x09: versão (read-only)
#define MAX17043_REG_CONFIG             0x0A  // 0x0A–0x0B: config/alert
#define MAX17043_REG_COMMAND            0x0C  // 0x0C–0x0D: comandos (quick start/reset)

esp_err_t max17043_get_id(void);
esp_err_t max17043_get_voltage(float *voltage);
esp_err_t max17043_get_soc(float *soc);
void task_MAX17043(void *pvArgs);



void init_MAX17043(void)
{
    esp_err_t ret;
    ret = max17043_get_id();
    if(ret != ESP_OK)
    {
        ESP_LOGE("MAX17043", "MAX17043 not found!");
        return;
    }

    xTaskCreate(task_MAX17043, "task_MAX17043", 4096, NULL, 5, &task_max17043_handle);
}


void task_MAX17043(void *pvArgs)
{
    MAX17043_Data data;
    ESP_LOGI("MAX17043", "MAX17043 initialized.");
    while(1)
    {
        if(max17043_get_voltage(&data.voltage) == ESP_OK &&
           max17043_get_soc(&data.soc) == ESP_OK)
        {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            xQueueSend(q_max17043_data, &data, pdMS_TO_TICKS(100));
            xEventGroupSetBits(eg_sync, MAX17043_BIT);
        }
        else
        {
            ESP_LOGE("MAX17043", "Failed to read data from MAX17043");
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

}



esp_err_t max17043_get_id(void)
{
    esp_err_t ret;
    uint8_t data[2];
    ret = hw_i2c_read(MAX17043_I2C_ADDR, MAX17043_REG_VERSION, data, 2);
    if(ret != ESP_OK)
        return ret;
    uint16_t version = (data[0] << 8) | data[1];
    //ESP_LOGI("MAX17043", "MAX17043 Version: 0x%04X", version);
    return ESP_OK;

}

esp_err_t max17043_get_voltage(float *voltage)
{
    esp_err_t ret;
    uint8_t data[2];
    ret = hw_i2c_read(MAX17043_I2C_ADDR, MAX17043_REG_VCELL, data, 2);
    if(ret != ESP_OK)
        return ret;
    uint16_t vcell_raw = (data[0] << 8) | data[1];
    *voltage = (vcell_raw >> 4) * 0.00125; // Cada bit representa 1.25mV
    return ESP_OK;
}

esp_err_t max17043_get_soc(float *soc)
{
    esp_err_t ret;
    uint8_t data[2];
    ret = hw_i2c_read(MAX17043_I2C_ADDR, MAX17043_REG_SOC, data, 2);
    if(ret != ESP_OK)
        return ret;
    uint16_t soc_raw = (data[0] << 8) | data[1];
    *soc = (soc_raw >> 8) + ((soc_raw & 0xFF) / 256.0); // Parte inteira + parte fracionária
    return ESP_OK;
}