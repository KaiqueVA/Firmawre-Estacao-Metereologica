#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_err.h"
#include "wind_rain_sensors.h"
#include "hw_common.h"
#include "data_processing.h"
#include "bmp280.h" 
#include "DHT22.h"
#include "LDR.h"
#include "MAX17043.h"


void app_main(void)
{   
    hw_common_init();
    bmp280_init();
    init_MAX17043();
    init_wind_rain_sensors();
    dht_init();
    LDR_init();
    init_data_processing();
}
