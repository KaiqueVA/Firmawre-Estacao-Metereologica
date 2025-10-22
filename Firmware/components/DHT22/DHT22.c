#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "hw_gpio.h"
#include "hw_timer.h"
#include "DHT22.h"
#include "data_processing.h"


#define TAG "DHT22"
#define EDGE_THRESHOLD_MICROSECONDS	110L

typedef struct{
	uint32_t isr_count;
	uint64_t edge_times[64];
	gpio_num_t gpio_num;
} DHT22_data_config_t;

static DHT22_data_config_t DHT22_data;

void task_dht22(void *pvArgs);

void IRAM_ATTR isr_gpio_isr_17(void *arg)
{
	if(DHT22_data.isr_count < 64) DHT22_data.edge_times[DHT22_data.isr_count]= hw_timer_get_time_us();
	DHT22_data.isr_count ++;
}



void dht_init(void)
{
	DHT22_data.gpio_num = GPIO_NUM_17;
	
	hw_configure_gpio(DHT22_data.gpio_num, GPIO_MODE_INPUT_OUTPUT_OD , GPIO_INTR_NEGEDGE);
	
	hw_gpio_level_set(DHT22_data.gpio_num , 1);

	xTaskCreate(task_dht22, "task_dht22", 4095, NULL, 5, &task_dht_handle);

}



int dht_read(DHT22_data_t *data_dht)
{
	data_dht->humidity = 0;
	data_dht->temperature = 0;

	hw_gpio_level_set( DHT22_data.gpio_num, 0 );

	vTaskDelay( 12 / portTICK_PERIOD_MS ); 
	hw_gpio_level_set( DHT22_data.gpio_num, 1 );

	DHT22_data.isr_count = 0;
	
	vTaskDelay( 10 / portTICK_PERIOD_MS );
	if(DHT22_data.isr_count >= 42 )
	{
		if(DHT22_data.isr_count != 42) ESP_LOGE(TAG, "WARNING pulse count %d != 42", DHT22_data.isr_count);
		uint8_t mask= 0x80;
		uint8_t data[6];
		int j=0;
		data[j] =0; 
		for (unsigned int i=(DHT22_data.isr_count - 40); i<DHT22_data.isr_count; i++)
		{
			int64_t pulse_width_us= DHT22_data.edge_times[i] - DHT22_data.edge_times[i-1];

			if(pulse_width_us > EDGE_THRESHOLD_MICROSECONDS) data[j]= data[j] | mask;
			mask = mask >> 1;
			if(mask==0) 		
			{
				mask= 0x80; 
				data[++j]=0;
			}
		}
		
		data_dht->humidity = ((uint16_t)data[0] << 8) + (uint16_t)data[1];
		data_dht->temperature = ((uint16_t)data[2] << 8) + (uint16_t)data[3];
	}
	else 
	{
		return -1;
	}
	fflush(stdout);
	return 0;
} 




void task_dht22(void *pvArgs)
{
	DHT22_data_t data, aux;

	uint16_t count = 0;
	ESP_LOGI(TAG, "Task DHT22 started");
	while(1)
	{
		if(dht_read(&aux) == 0)
		{
			data.humidity += aux.humidity;
			data.temperature += aux.temperature;
			count++;
			//ESP_LOGI("DHT22", "TEMP: %.2f | HUM: %d", aux.temperature/10.0, aux.humidity/10);

		}
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if(count == 0)
			count = 1;
		data.humidity /= count;
		data.temperature /= count;
		xQueueSend(q_dht22_data, &data, pdMS_TO_TICKS(100));
		count = 0;
		data.humidity = 0;
		data.temperature = 0;
		xEventGroupSetBits(eg_sync, DHT22_BIT);
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}
