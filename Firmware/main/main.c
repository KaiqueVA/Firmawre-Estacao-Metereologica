#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "wind_sensors.h"
#include "esp_log.h"


void app_main(void)
{   

    init_wind_sensors();
    
    while(1)
    {
        get_count_34();
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}
