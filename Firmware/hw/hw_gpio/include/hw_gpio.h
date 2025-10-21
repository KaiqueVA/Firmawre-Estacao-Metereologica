#ifndef HW_GPIO_H
#define HW_GPIO_H

//===========Inclusões de arquivos===========

#include "driver/gpio.h"

//===========================================

//===========Funções modificaveis===========

void isr_gpio_isr_34(void *arg);
void isr_gpio_isr_35(void *arg);
void isr_gpio_isr_17(void *arg);

//==========================================

//===========Protótipos de funções===========

void hw_gpio_init(void);
void hw_configure_gpio(gpio_num_t pin, gpio_mode_t mode, gpio_int_type_t intr_type);
void hw_gpio_level_set(gpio_num_t pin, uint32_t level);

//======================================
#endif // HW_GPIO_H
