

/*
Created on: 2024-06-26
Author: Kaique Vecchia Alves
Description: Header file for wind_sensors.c
*/

#ifndef WIND_RAIN_SENSORS_H_
#define WIND_RAIN_SENSORS_H_

typedef struct{
    float wind_speed_ms;
    float wind_speed_kmh;
    float wind_direction;
    float rain_gauge_mm;
}wind_rain_sensor_data_t;


void init_wind_rain_sensors(void);


#endif /* WIND_RAIN_SENSORS_H_ */
