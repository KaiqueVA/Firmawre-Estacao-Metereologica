#include "LDR.h"
#include "data_processing.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hw_adc.h"
#include "esp_log.h"
#include <math.h>

#define TAG "LDR"

#define RESISTOR_FIXED 5000
#define K              75000
#define GAMMA           0.7
#define VCC            3.16

void LDR_task(void *pvArgs);

void LDR_init(void)
{
    xTaskCreate(LDR_task, "LDR_Task", 4096, NULL, 5, &task_ldr_handle);
}

void LDR_task(void *pvArgs)
{
    int ldr_value = 0;
    float ldr_resistence = 0, lux = 0;
    ESP_LOGI("LDR", "LDR Task started");
    while(1)
{
        hw_adc_read_mv(ADC_CHANNEL_4, &ldr_value);
        if(ldr_value != -1)
        {
            ldr_resistence = RESISTOR_FIXED * ((VCC / (ldr_value / 1000.0)) - 1.0);
            lux = powf(K/ldr_resistence, 1.0/GAMMA);
        }
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        xQueueSend(q_ldr_data, &lux, pdMS_TO_TICKS(100));
        xEventGroupSetBits(eg_sync, LDR_BIT);
        vTaskDelay(pdMS_TO_TICKS(10));

    }
}
