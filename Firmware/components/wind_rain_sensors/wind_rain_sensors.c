#include <stdio.h>
#include "wind_rain_sensors.h"
#include "hw_gpio.h"
#include "stdint.h"
#include "esp_log.h"
#include "hw_timer.h"
#include "stdbool.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "hw_adc.h"

#define WIND_SPEED_BIT (1<<0)
#define WIND_DIRECTION_BIT (1<<1)
#define RAIN_GAUGE_BIT (1<<2)

#define DONE_ALL (WIND_SPEED_BIT | WIND_DIRECTION_BIT | RAIN_GAUGE_BIT)

TaskHandle_t task_wind_direction_handle = NULL;
TaskHandle_t task_wind_speed_handle = NULL;
TaskHandle_t task_rain_gauge_handle = NULL;

QueueHandle_t q_wind_data;
QueueHandle_t q_wind_speed;
QueueHandle_t q_wind_direction;
QueueHandle_t q_rain_gauge;

EventGroupHandle_t eg_sync;

uint8_t volatile flag_interrupt_34 = 0;
uint8_t volatile flag_interrupt_35 = 0;
uint8_t volatile flag_timer0 = 0;
uint8_t volatile flag_timer1 = 0;

bool interrupt_34_rising_edge = false;
bool interrupt_34_falling_edge = false;
bool interrupt_35_falling_edge = false;
bool interrupt_35_rising_edge = false;


uint32_t count_34 = 0;
uint32_t count_35 = 0;


void task_data_processing(void *pvArgs);
void task_wind_direction(void *pvArgs);
void task_wind_speed(void *pvArgs);
void task_rain_gauge(void *pvArgs);



void init_wind_rain_sensors(void)
{
    hw_timer_init(0, false, 1000*10); //10mS wind speed
    hw_timer_init(1, false, 1000 * 20); //20mS Rain gauge
    hw_timer_init(2, true, 1000 * 1000); //1S
    hw_timer_start(2);


    eg_sync = xEventGroupCreate();
    q_wind_data = xQueueCreate(1, sizeof(wind_rain_sensor_data_t));
    q_wind_direction = xQueueCreate(1, sizeof(float));
    q_wind_speed = xQueueCreate(1, sizeof(float));
    q_rain_gauge = xQueueCreate(1, sizeof(float));


    xTaskCreate(task_wind_speed, "task_wind_direction", 4096, NULL, 2, &task_wind_speed_handle);
    xTaskCreate(task_wind_direction, "task_wind_direction", 4096, NULL, 4, &task_wind_direction_handle);
    xTaskCreate(task_rain_gauge, "task_rain_gauge", 4096, NULL, 5, &task_rain_gauge_handle);
    xTaskCreate(task_data_processing, "task_data", 4096, NULL, 5, NULL);

    
}

void task_data_processing(void *pvArgs)
{
    wind_rain_sensor_data_t wind_rain_sensor;
    while(1)
    {
        xEventGroupWaitBits(eg_sync, DONE_ALL, pdTRUE, pdTRUE, portMAX_DELAY);
        xQueueReceive(q_wind_direction, &(wind_rain_sensor.wind_direction), 0);
        xQueueReceive(q_wind_speed, &(wind_rain_sensor.wind_speed_kmh), 0);
        xQueueReceive(q_rain_gauge, &(wind_rain_sensor.rain_gauge_mm), 0);
        ESP_LOGI("WIND_SENSOR", "Wind Speed: %.2f km/h | %.1f degree | Rain Gauge: %.4fmm", wind_rain_sensor.wind_speed_kmh, wind_rain_sensor.wind_direction, wind_rain_sensor.rain_gauge_mm);
        vTaskDelay(100);
    }
}

void task_wind_direction(void *pvArgs)
{
    
    int adc_value, adc_oversampling = 0;
    uint16_t count_sampling = 0;
    float degree = 0;

    while(1)
    {
        if(hw_adc_read_mv(ADC_CHANNEL_5, &adc_value) == ESP_OK)
        {
            if(adc_value != -1)
            {
                adc_oversampling += adc_value;
                count_sampling++;
            }
        }
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if(count_sampling == 0)
            count_sampling = 1;
        adc_value = adc_oversampling/count_sampling;
        if      (adc_value >= 200  && adc_value <= 300)  degree = 270.0f;
        else if (adc_value >= 400  && adc_value <= 500)  degree = 315.0f;
        else if (adc_value >= 600  && adc_value <= 700)  degree = 292.5f;
        else if (adc_value >= 700  && adc_value <= 850)  degree = 0.0f;
        else if (adc_value >= 900  && adc_value <= 1150) degree = 337.5f;
        else if (adc_value >= 1200 && adc_value <= 1350) degree = 225.0f;
        else if (adc_value >= 1350 && adc_value <= 1500) degree = 247.5f;
        else if (adc_value >= 1750 && adc_value <= 1900) degree = 45.0f;
        else if (adc_value >= 1900 && adc_value <= 2100) degree = 22.5f;
        else if (adc_value >= 2300 && adc_value <= 2450) degree = 180.0f;
        else if (adc_value >= 2450 && adc_value <= 2600) degree = 202.5f;
        else if (adc_value >= 2650 && adc_value <= 2800) degree = 135.0f;
        else if (adc_value >= 2850 && adc_value <= 2950) degree = 157.5f;
        else if (adc_value >= 2950 && adc_value <= 3020) degree = 90.0f;
        else if (adc_value >= 3020 && adc_value <= 3060) degree = 67.5f;
        else if (adc_value >= 3070 && adc_value <= 3120) degree = 112.5f;
        xQueueSend(q_wind_direction, &degree, 0);
        adc_oversampling = count_sampling = 0;
        xEventGroupSetBits(eg_sync, WIND_DIRECTION_BIT);
        vTaskDelay(10);
    }
        
    
}

void task_wind_speed(void *pvArgs)
{
    float wind_speed = 0;
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        wind_speed = count_35 * 2.4;
        xQueueSend(q_wind_speed, &wind_speed, 0);
        count_35 = 0;
        xEventGroupSetBits(eg_sync, WIND_SPEED_BIT);
    }
}

void task_rain_gauge(void *pvArgs)
{
    float rain_gauge;
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        rain_gauge = count_34 * 0.2794;
        xQueueSend(q_rain_gauge, &rain_gauge, 0);
        count_34 = 0;
        xEventGroupSetBits(eg_sync, RAIN_GAUGE_BIT);
    }
}



void hw_timer0_callback(void) { //timer 10ms
    flag_timer0 = 1;
}

void hw_timer1_callback(void){
    flag_timer1 = 1;
}

void hw_timer2_callback(void) //timer 1s
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(task_wind_direction_handle, &xHigherPriorityTaskWoken);
    vTaskNotifyGiveFromISR(task_wind_speed_handle, &xHigherPriorityTaskWoken);
    vTaskNotifyGiveFromISR(task_rain_gauge_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void IRAM_ATTR isr_gpio_isr_35(void *arg)
{
    uint8_t level = gpio_get_level((uint32_t)arg);

    if(level == 1)
        interrupt_35_rising_edge = true;
    else{
        interrupt_35_falling_edge = true;
        hw_timer_start(0);
    }
    if(interrupt_35_rising_edge && interrupt_35_falling_edge)
    {
        if(flag_timer0){
            count_35++;
        }
        hw_timer_stop(0);
        flag_timer0 = 0;
        interrupt_35_rising_edge = false;
        interrupt_35_falling_edge = false;
    }
    
}

void IRAM_ATTR isr_gpio_isr_34(void *arg)
{
    uint8_t level = gpio_get_level((uint32_t)arg);

    if(level)
    {
        interrupt_34_rising_edge = true;
    }else{
        interrupt_34_falling_edge = true;
        hw_timer_start(1);
    }
    if(interrupt_34_rising_edge && interrupt_34_falling_edge)
    {
        if(flag_timer1){
            count_34++;
        }
        hw_timer_stop(1);
        flag_timer1 = 0;
        interrupt_34_rising_edge = false;
        interrupt_34_falling_edge = false;
    }
}
