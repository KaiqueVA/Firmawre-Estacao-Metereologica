#include "hw_adc.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <assert.h>


adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t         adc1_cali_handle = NULL;
static bool adc1_calibrated = false;

void init_hw_adc(void)
{
    

    adc_oneshot_unit_init_cfg_t adc1_unit_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc1_unit_cfg, &adc1_handle));

    adc_oneshot_chan_cfg_t adc1_chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };


    adc_channel_t ch32, ch33;
    adc_unit_t    unit_tmp;

    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(ADC_32, &unit_tmp, &ch32));  
    assert(unit_tmp == ADC_UNIT_1);
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(ADC_33, &unit_tmp, &ch33));
    assert(unit_tmp == ADC_UNIT_1);

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ch32, &adc1_chan_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ch33, &adc1_chan_cfg));

    adc_cali_line_fitting_config_t adc1_cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .default_vref = 1100
    };

    bool do_calibration = adc_cali_create_scheme_line_fitting(&adc1_cali_cfg, &adc1_cali_handle) == ESP_OK;
    if(do_calibration) {
        ESP_LOGI("HW_ADC", "ADC1 calibration created successfully");
        adc1_calibrated = true;
    } else {
        ESP_LOGW("HW_ADC", "ADC1 calibration failed to create");
    }

}

esp_err_t hw_adc_read(adc_channel_t ch, uint32_t *raw){
    if(!adc1_handle || !raw) return ESP_ERR_INVALID_STATE;
    return adc_oneshot_read(adc1_handle, ch, (int*)raw);
}

esp_err_t hw_adc_read_mv(adc_channel_t ch, int *mv_out)
{
    if(!adc1_handle || !mv_out) 
        return ESP_ERR_INVALID_STATE;

    uint32_t raw = 0;
    esp_err_t err = hw_adc_read(ch, &raw);

    if(err != ESP_OK) 
        return err;
    if(adc1_calibrated && adc1_cali_handle)
        return adc_cali_raw_to_voltage(adc1_cali_handle, raw, mv_out);
    else{
        *mv_out = -1;
        return ESP_OK;
    }
}