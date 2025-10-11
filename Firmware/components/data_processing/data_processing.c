#include "data_processing.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "hw_timer.h"
#include "bmp280.h"

//Variaveis externas glboais
EventGroupHandle_t eg_sync = NULL;

QueueHandle_t q_wind_speed;
QueueHandle_t q_wind_direction;
QueueHandle_t q_rain_gauge;
QueueHandle_t q_bmp280_data;

TaskHandle_t task_wind_direction_handle = NULL;
TaskHandle_t task_wind_speed_handle = NULL;
TaskHandle_t task_rain_gauge_handle = NULL;
TaskHandle_t task_bmp280_handle = NULL;

void task_data_processing(void *pvArgs);

esp_err_t init_data_processing(void)
{
    hw_timer_init(2, true, 1000 * 1000); //1S
    hw_timer_start(2);

    eg_sync = xEventGroupCreate();
    
    q_wind_speed = xQueueCreate(1, sizeof(float));
    q_wind_direction = xQueueCreate(1, sizeof(float));
    q_rain_gauge = xQueueCreate(1, sizeof(float));  
    q_bmp280_data = xQueueCreate(1, sizeof(bmp280_data_t));

    if (!q_wind_speed || !q_wind_direction || !q_rain_gauge || !q_bmp280_data) {
        ESP_LOGE("DATA_PROCESSING", "Failed to create queues");
        return ESP_FAIL;
    }

    xTaskCreate(task_data_processing, "task_data_processing", 4096, NULL, 5, NULL);
    
    return ESP_OK;
}

void task_data_processing(void *pvArgs)
{
    float wind_speed = 0, wind_direction = 0, rain_gauge = 0;
    BaseType_t status;
    bmp280_data_t sensor_data;
    ESP_LOGI("DATA_PROCESSING", "Task data processing started");
    
    while(1)
    {
        EventBits_t bits = xEventGroupWaitBits(eg_sync, 
                                             DONE_ALL,
                                             pdTRUE,
                                             pdTRUE,
                                             portMAX_DELAY);
        
        ESP_LOGD("DATA_PROCESSING", "Event bits received: 0x%x", (unsigned int)bits);
        
        status = xQueueReceive(q_wind_direction, &wind_direction, pdMS_TO_TICKS(100));
        if (status != pdTRUE) {
            ESP_LOGW("DATA_PROCESSING", "Failed to receive wind direction");
        }
        
        status = xQueueReceive(q_wind_speed, &wind_speed, pdMS_TO_TICKS(100));
        if (status != pdTRUE) {
            ESP_LOGW("DATA_PROCESSING", "Failed to receive wind speed");
        }
        
        status = xQueueReceive(q_rain_gauge, &rain_gauge, pdMS_TO_TICKS(100));
        if (status != pdTRUE) {
            ESP_LOGW("DATA_PROCESSING", "Failed to receive rain gauge");
        }

        status = xQueueReceive(q_bmp280_data, &sensor_data, pdMS_TO_TICKS(100));
        if (status != pdTRUE) {
            ESP_LOGW("DATA_PROCESSING", "Failed to receive BMP280 data");
        }

        ESP_LOGI("DATA_PROCESSING", "Wind Speed: %.2f km/h | %.1f degree | Rain Gauge: %.4fmm | BMP280: %.2f hPa",
                wind_speed, wind_direction, rain_gauge, sensor_data.pressure);
                
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void hw_timer2_callback(void) //timer 1s
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (task_wind_direction_handle != NULL) {
        
        vTaskNotifyGiveFromISR(task_wind_direction_handle, &xHigherPriorityTaskWoken);
    }
    
    if (task_wind_speed_handle != NULL) {
        vTaskNotifyGiveFromISR(task_wind_speed_handle, &xHigherPriorityTaskWoken);
    }
    
    if (task_rain_gauge_handle != NULL) {
        vTaskNotifyGiveFromISR(task_rain_gauge_handle, &xHigherPriorityTaskWoken);
    }

    if (task_bmp280_handle != NULL) {
        vTaskNotifyGiveFromISR(task_bmp280_handle, &xHigherPriorityTaskWoken);
    }
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    
}