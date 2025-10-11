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

#define BMP280_OSRS_T_X2   (0b010 << 5)
#define BMP280_OSRS_P_X16  (0b101 << 2)
#define BMP280_MODE_NORMAL (0b11)

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
    int64_t t_fine;
}bmp280_data_i2c_t;


//Variavel gloval
bmp280_data_i2c_t bmp280_i2c;

esp_err_t bmp280_get_id(void);
esp_err_t bmp280_soft_reset(void);
esp_err_t bmp280_get_calib_data(void);
esp_err_t bmp280_ctrl_meas(void);
esp_err_t bmp280_config(void);
int32_t bmp280_compensate_T(int32_t adc_T);
uint32_t bmp280_compensate_P(int32_t adc_P);



esp_err_t bmp280_init(void)
{
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

    ret = bmp280_ctrl_meas();
    if(ret != ESP_OK)
    {
        ESP_LOGE("BMP280", "Failed to set ctrl_meas");
        return ret;
    }

    ret = bmp280_config();
    if(ret != ESP_OK)
    {
        ESP_LOGE("BMP280", "Failed to set config");
        return ret;
    }

    ESP_LOGI("BMP280", "BMP280 initialized.");

    return ESP_OK;

}




esp_err_t bmp280_read_data(bmp280_data_t *data)
{
    uint8_t raw_data[6];
    esp_err_t ret;
    ret = hw_i2c_read(ADDR_BMP280, BMP280_REG_PRESS_MSB, raw_data, 6);
    if(ret != ESP_OK)
        return ret;
    int32_t adc_p = (int32_t)((raw_data[0] << 12) | (raw_data[1] << 4) | (raw_data[2] >> 4));
    int32_t adc_t = (int32_t)((raw_data[3] << 12) | (raw_data[4] << 4) | (raw_data[5] >> 4));

    data->temperature = bmp280_compensate_T(adc_t)/100.0;
    data->pressure = bmp280_compensate_P(adc_p)/25600.0;
    ESP_LOGI("BMP280", "Temperature: %.2f C, Pressure: %.2f hPa", data->temperature, data->pressure);
    return ret;

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

esp_err_t bmp280_ctrl_meas(void)
{
    uint8_t ctrl_meas = BMP280_OSRS_T_X2 | BMP280_OSRS_P_X16 | BMP280_MODE_NORMAL;
    esp_err_t ret;
    ret = hw_i2c_write(ADDR_BMP280, BMP280_REG_CTRL_MEAS, &ctrl_meas, 1);
    return ret;
}

esp_err_t bmp280_config(void)
{
    uint8_t config = (0b100 << 5) | (0b100 << 2);
    esp_err_t ret;
    ret = hw_i2c_write(ADDR_BMP280, BMP280_REG_CONFIG, &config, 1);
    return ret;
}

int32_t bmp280_compensate_T(int32_t adc_T)
{
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)bmp280_i2c.calibration_data.dig_T1 << 1))) * ((int32_t)bmp280_i2c.calibration_data.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)bmp280_i2c.calibration_data.dig_T1)) * ((adc_T >> 4) - ((int32_t)bmp280_i2c.calibration_data.dig_T1))) >> 12) * ((int32_t)bmp280_i2c.calibration_data.dig_T3)) >> 14;
    bmp280_i2c.t_fine = var1 + var2;
    float T = (bmp280_i2c.t_fine * 5 + 128) >> 8;
    return T;
}

uint32_t bmp280_compensate_P(int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)bmp280_i2c.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bmp280_i2c.calibration_data.dig_P6;
    var2 = var2 + ((var1 * (int64_t)bmp280_i2c.calibration_data.dig_P5) << 17);
    var2 = var2 + (((int64_t)bmp280_i2c.calibration_data.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)bmp280_i2c.calibration_data.dig_P3) >> 8) + ((var1 * (int64_t)bmp280_i2c.calibration_data.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)bmp280_i2c.calibration_data.dig_P1) >> 33;
    if (var1 == 0) {
        return 0; // evita divisão por zero
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)bmp280_i2c.calibration_data.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)bmp280_i2c.calibration_data.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)bmp280_i2c.calibration_data.dig_P7) << 4);
    return p;

}

