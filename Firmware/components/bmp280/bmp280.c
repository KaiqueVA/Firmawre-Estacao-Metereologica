#include <stdio.h>
#include "bmp280.h"
#include "hw_i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "string.h"

#define ADDR_BMP280 0x76
// Identificação e Reset
#define BMP280_REG_ID              0xD0  // ID do chip (deve retornar 0x58)
#define BMP280_REG_RESET           0xE0  // Reset do dispositivo (valor 0xB6)
// Status e controle
#define BMP280_REG_STATUS          0xF3  // Bit 3: measuring, Bit 0: im_update
#define BMP280_REG_CTRL_MEAS       0xF4  // Controle de modo, oversampling pressão/temp
#define BMP280_REG_CONFIG          0xF5  // Configuração de filtro, standby e SPI 3-wire
// Dados de pressão (3 bytes)
#define BMP280_REG_PRESS_MSB       0xF7  // Pressão [19:12]
#define BMP280_REG_PRESS_LSB       0xF8  // Pressão [11:4]
#define BMP280_REG_PRESS_XLSB      0xF9  // Pressão [3:0]
// Dados de temperatura (3 bytes)
#define BMP280_REG_TEMP_MSB        0xFA  // Temperatura [19:12]
#define BMP280_REG_TEMP_LSB        0xFB  // Temperatura [11:4]
#define BMP280_REG_TEMP_XLSB       0xFC  // Temperatura [3:0]

// =========================================
// Registradores de calibração (armazenam coeficientes NVM)
// =========================================
// (Cada parâmetro ocupa 2 bytes, little-endian)

#define BMP280_REG_CALIB00         0x88  // T1 (LSB)
#define BMP280_REG_CALIB25         0xA1  // Último coeficiente de calibração (H7 ou dig_P9)

typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} bmp280_calib_t;

typedef struct{
    uint8_t *data;
    uint32_t length;
    bmp280_calib_t calibration_data;
}bmp280_data_i2c_t;


//Variavel gloval
bmp280_data_i2c_t bmp280_i2c;

esp_err_t bmp280_get_id(void);
esp_err_t bmp280_soft_reset(void);
esp_err_t bmp280_get_calib_data(void);


esp_err_t bmp280_init(void)
{
    hw_i2c_init();
    bmp280_i2c.data = NULL;
    bmp280_i2c.length = 0;
    esp_err_t ret;
    ret = bmp280_get_id();
    if(ret != ESP_OK)
    {
        ESP_LOGE("BMP280", "BMP280 not found!");
        return ret;
    }
    ESP_LOGI("BMP280", "BMP280 detected.");
    ret = bmp280_soft_reset();
    if(ret != ESP_OK){
        ESP_LOGE("BMP280", "Failed to reset BMP280");
        return ret;
    }
    ESP_LOGI("BMP280", "BMP280 reseted.");
    vTaskDelay(pdMS_TO_TICKS(100));
    ret = bmp280_get_calib_data();
    if(ret != ESP_OK)
    {
        ESP_LOGE("BMP280", "Failed to get calibration data");
        return ret;
    }
    ESP_LOGI("BMP280", "Calibration data loaded.");
    return ESP_OK;

}



esp_err_t bmp280_get_id(void)
{
    uint8_t id = 0;
    esp_err_t ret;
    ret = hw_i2c_read(ADDR_BMP280, BMP280_REG_ID, &id, 1);
    if(ret != ESP_OK)
        return ret;

    if(id == 0x58)
    {
        return ESP_OK;
    }
    return ESP_ERR_INVALID_RESPONSE;
}

esp_err_t bmp280_soft_reset(void)
{
    esp_err_t ret;
    uint8_t reset_cmd = 0xB6;
    ret = hw_i2c_write(ADDR_BMP280, BMP280_REG_RESET, &reset_cmd, 1);
    return ret;
}

esp_err_t bmp280_get_calib_data(void)
{
    esp_err_t ret;
    uint8_t calib_data[26];
    ret = hw_i2c_read(ADDR_BMP280, BMP280_REG_CALIB00, calib_data, 26);
    if(ret != ESP_OK)
        return ret;
    bmp280_i2c.calibration_data.dig_T1 = (uint16_t)((calib_data[1] << 8) | calib_data[0]);
    bmp280_i2c.calibration_data.dig_T2 = (int16_t)((calib_data[3] << 8) | calib_data[2]);
    bmp280_i2c.calibration_data.dig_T3 = (int16_t)((calib_data[5] << 8) | calib_data[4]);
    bmp280_i2c.calibration_data.dig_P1 = (uint16_t)((calib_data[7] << 8) | calib_data[6]);
    bmp280_i2c.calibration_data.dig_P2 = (int16_t)((calib_data[9] << 8) | calib_data[8]);
    bmp280_i2c.calibration_data.dig_P3 = (int16_t)((calib_data[11] << 8) | calib_data[10]);
    bmp280_i2c.calibration_data.dig_P4 = (int16_t)((calib_data[13] << 8) | calib_data[12]);
    bmp280_i2c.calibration_data.dig_P5 = (int16_t)((calib_data[15] << 8) | calib_data[14]);
    bmp280_i2c.calibration_data.dig_P6 = (int16_t)((calib_data[17] << 8) | calib_data[16]);
    bmp280_i2c.calibration_data.dig_P7 = (int16_t)((calib_data[19] << 8) | calib_data[18]);
    bmp280_i2c.calibration_data.dig_P8 = (int16_t)((calib_data[21] << 8) | calib_data[20]);
    bmp280_i2c.calibration_data.dig_P9 = (int16_t)((calib_data[23] << 8) | calib_data[22]);

    return ESP_OK;
    
    
}
