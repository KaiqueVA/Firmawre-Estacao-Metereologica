#include <stdio.h>
#include "wind_sensors.h"
#include "hw_gpio.h"
#include "stdint.h"
#include "esp_log.h"
#include "hw_timer.h"
#include "stdbool.h"
#include "freertos/FreeRTOS.h"

TaskHandle_t task_data_handle = NULL;
uint8_t volatile flag_interrupt_34 = 0;
uint8_t volatile flag_interrupt_35 = 0;
uint8_t volatile flag_timer0 = 0;
bool interrupt_34_falling_edge = false;
bool interrupt_34_rising_edge = false;

uint32_t count_34 = 0;
uint32_t count_35 = 0;

uint32_t flag_timer_1min = 0;

void task_data_processing(void *pvArgs);



void init_wind_sensors(void)
{
    hw_gpio_init();
    hw_timer_init(0, false, 10000); //10mS
    hw_timer_init(2, true, 1000 * 1000); //1 S
    hw_timer_start(2);
    //xTaskCreate(get_count_34, "get_count_34", 4096, NULL, 5, NULL);
    xTaskCreate(task_data_processing, "task_data", 4096, NULL, 5, &task_data_handle);
    
}

void task_data_processing(void *pvArgs)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ESP_LOGI("WIND_SENSOR", "Wind Speed: %.2f km/h", (count_34 * 2.4));
        count_34 = 0;
    }
}

void get_count_34(void *pvArgs)
{
    static uint32_t last_count_34 = 0;

    while(1)
    {
        if(count_34 != last_count_34)
        {
            last_count_34 = count_34;
            ESP_LOGI("WIND_SENSOR", "Count 34: %d", count_34);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    
}

void hw_timer0_callback(void) {
    flag_timer0 = 1;
}

void hw_timer2_callback(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(task_data_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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
