

/*
Created on: 2024-06-26
Author: Kaique Vecchia Alves
Description: Header file for wind_sensors.c
*/

#ifndef WIND_SENSORS_H_
#define WIND_SENSORS_H_

typedef struct{
    float wind_speed_ms;
    float wind_speed_kmh;
    float wind_direction;
}wind_sensor_data_t;


void init_wind_sensors(void);


#endif /* WIND_SENSORS_H_ */
