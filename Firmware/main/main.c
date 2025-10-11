#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_err.h"
#include "wind_rain_sensors.h"
#include "hw_common.h"


void app_main(void)
{   
    hw_common_init();
    init_wind_rain_sensors();    
}
