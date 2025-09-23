#ifndef HW_GPIO_H
#define HW_GPIO_H

//===========Inclusões de arquivos===========

#include "driver/gpio.h"

//===========================================

//===========Funções modificaveis===========

void isr_gpio_isr_34(void *arg);
void isr_gpio_isr_35(void *arg);

//==========================================

//===========Protótipos de funções===========

void hw_gpio_init(void);

//======================================
#endif // HW_GPIO_H
