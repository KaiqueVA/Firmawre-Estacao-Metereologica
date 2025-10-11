#include <stdio.h>
#include "hw_common.h"
#include "hw_i2c.h"
#include "hw_gpio.h"
#include "hw_adc.h"
#include "esp_log.h"

void hw_common_init(void)
{
    ESP_LOGI("HW_COMMON", "Initializing common hardware...");
    hw_gpio_init();
    init_hw_adc();
    hw_i2c_init();
}