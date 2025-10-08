#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>
#include "esp_err.h"

typedef struct{
    float temperature;
    float pressure;
    float altitude;
} bmp280_data_t;

esp_err_t bmp280_init(void);
void bmp280_read_data(bmp280_data_t *data);
void bmp280_ctrl_meas(void);
void bmp280_config(void);
void bmp280_osrs_p(uint8_t oversampling);






#endif //BMP280_H
