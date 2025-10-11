#ifndef DATA_PROCESSING_H
#define DATA_PROCESSING_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"


//========================= Event Group =================================
#define WIND_SPEED_BIT      (1<<0)
#define WIND_DIRECTION_BIT  (1<<1)
#define RAIN_GAUGE_BIT      (1<<2)
#define BMP280_BIT          (1<<3)

#define DONE_ALL (WIND_SPEED_BIT | WIND_DIRECTION_BIT | RAIN_GAUGE_BIT)

extern EventGroupHandle_t eg_sync;

//=======================================================================

//========================= Queues ======================================

extern QueueHandle_t q_wind_speed;
extern QueueHandle_t q_wind_direction;
extern QueueHandle_t q_rain_gauge;

//=======================================================================

//========================= Tasks Handles ===============================

extern TaskHandle_t task_wind_direction_handle;
extern TaskHandle_t task_wind_speed_handle;
extern TaskHandle_t task_rain_gauge_handle;

//=======================================================================

esp_err_t init_data_processing(void);

#endif // DATA_PROCESSING_H