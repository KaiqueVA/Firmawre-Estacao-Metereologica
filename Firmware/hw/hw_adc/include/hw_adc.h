#ifndef HW_ADC_H
#define HW_ADC_H

#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"

#define ADC_32 GPIO_NUM_32 //LDR
#define ADC_33 GPIO_NUM_33 //Direcao do vento

void init_hw_adc(void);
esp_err_t hw_adc_read(adc_channel_t ch, uint32_t *raw);
esp_err_t hw_adc_read_mv(adc_channel_t ch, int *mv_out);


#endif //HW_ADC_H