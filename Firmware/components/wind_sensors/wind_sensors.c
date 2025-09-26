#include <stdio.h>
#include "wind_sensors.h"
#include "hw_gpio.h"
#include "stdint.h"
#include "esp_log.h"
#include "hw_timer.h"
#include "stdbool.h"

uint8_t volatile flag_interrupt_34 = 0;
uint8_t volatile flag_interrupt_35 = 0;
uint8_t volatile flag_timer0 = 0;
bool interrupt_34_falling_edge = false;
bool interrupt_34_rising_edge = false;


uint32_t count_34 = 0;
uint32_t count_35 = 0;

void init_wind_sensors(void)
{
    hw_gpio_init();
    hw_timer_init(0, false);

}

void get_count_34(void)
{
    static uint32_t last_count_34 = 0;
    if(count_34 != last_count_34)
    {
        last_count_34 = count_34;
        ESP_LOGI("WIND_SENSOR", "Count 34: %d", count_34);
    }
}

void hw_timer0_callback(void) {
    flag_timer0 = 1;
}


void IRAM_ATTR isr_gpio_isr_34(void *arg)
{
    uint8_t level = gpio_get_level((uint32_t)arg);

    if(level == 1)
        interrupt_34_rising_edge = true;
    else{
        interrupt_34_falling_edge = true;
        hw_timer_start(0);
    }
    if(interrupt_34_rising_edge && interrupt_34_falling_edge)
    {
        if(flag_timer0){
            count_34++;
        }
        hw_timer_stop(0);
        flag_timer0 = 0;
        interrupt_34_rising_edge = false;
        interrupt_34_falling_edge = false;
    }
    
}

void IRAM_ATTR isr_gpio_isr_35(void *arg)
{
    count_35 = (uint32_t)arg;
    flag_interrupt_35 = 1;
}
