/*
Created on: 2024-06-26
Author: Kaique Vecchia Alves
Description: Header file for wind_sensors.c
*/

#ifndef HW_TIMER_H
#define HW_TIMER_H

#include "stdbool.h"
#include "driver/gptimer.h"
#include "esp_attr.h"
#include "esp_err.h"


void IRAM_ATTR hw_timer0_callback(void);
void IRAM_ATTR hw_timer1_callback(void);
void IRAM_ATTR hw_timer2_callback(void);
void IRAM_ATTR hw_timer3_callback(void);


void hw_timer_init(uint8_t timer_num, bool auto_reload, uint64_t timer_window_us);
void hw_timer_start(uint8_t timer_num);
void hw_timer_stop(uint8_t timer_num);
void hw_timer_disarm(uint8_t timer_num);
void hw_timer_del(uint8_t timer_num);
uint64_t hw_timer_get_time_us(void);



#endif // HW_TIMER_H