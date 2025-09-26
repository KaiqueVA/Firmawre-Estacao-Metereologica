#include <stdio.h>
#include "hw_timer.h"
/*
==============================================================================
 Private Macros
==============================================================================
 */
#define TIMER_RES_HZ    1000000
#define T_WINDOW_US     10000
#define ALARM_TICKS(us) (us)
/*
==============================================================================
==============================================================================
==============================================================================
*/
/*
=============================================================================
 Private Variables
=============================================================================
*/

typedef struct{
    gptimer_handle_t timer;
    bool flag_started;
}timer_data_t;

timer_data_t timer_data[4];
/*
=============================================================================
=============================================================================
=============================================================================
*/

void __attribute__((weak)) hw_timer0_callback(void) { return; } //callback para complementar o timer 0

bool IRAM_ATTR hw_timer0_isr_callback(gptimer_handle_t t, const gptimer_alarm_event_data_t *edata, void *user_ctx) { 
    timer_data[0].flag_started = false;
    hw_timer0_callback();
    return false; 
}


void hw_timer_init(uint8_t timer_num, bool auto_reload) {
    timer_data[timer_num].flag_started = false;
    gptimer_config_t cfg = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_RES_HZ,
        .intr_priority = 1,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&cfg, &timer_data[timer_num].timer));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = ALARM_TICKS(T_WINDOW_US),
        .flags.auto_reload_on_alarm = auto_reload,
    };

    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer_data[timer_num].timer, &alarm_config));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = hw_timer0_isr_callback,
    };

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer_data[timer_num].timer, &cbs, NULL));

    ESP_ERROR_CHECK(gptimer_enable(timer_data[timer_num].timer));
}

void hw_timer_start(uint8_t timer_num)
{
    if(!timer_data[timer_num].flag_started) {
        ESP_ERROR_CHECK(gptimer_start(timer_data[timer_num].timer));
        timer_data[timer_num].flag_started = true;
    }
}

void hw_timer_stop(uint8_t timer_num)
{
    if(timer_data[timer_num].flag_started){
        ESP_ERROR_CHECK(gptimer_stop(timer_data[timer_num].timer));
        timer_data[timer_num].flag_started = false;
    }
}

void hw_timer_disarm(uint8_t timer_num)
{
    ESP_ERROR_CHECK(gptimer_disable(timer_data[timer_num].timer));
}

void hw_timer_del(uint8_t timer_num)
{
    ESP_ERROR_CHECK(gptimer_del_timer(timer_data[timer_num].timer));
    timer_data[timer_num].flag_started = false;
}

