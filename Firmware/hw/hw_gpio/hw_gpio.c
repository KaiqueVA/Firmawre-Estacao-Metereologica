#include <stdio.h>
#include "hw_gpio.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"



#define GPIO_34_INPUT_PIN   34
#define GPIO_35_INPUT_PIN   35
#define GPIO_17_INPUT_PIN   17

uint32_t count = 0;
uint8_t flag_interrupt = 0;


__attribute__((weak)) void IRAM_ATTR isr_gpio_isr_34(void *arg) { (void)arg; }
__attribute__((weak)) void IRAM_ATTR isr_gpio_isr_35(void *arg) { (void)arg; }
__attribute__((weak)) void IRAM_ATTR isr_gpio_isr_17(void *arg) { (void)arg; }

void hw_gpio_init(void)
{
    ESP_LOGI("HW_GPIO", "Initializing GPIO...");
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << GPIO_34_INPUT_PIN) | (1ULL << GPIO_35_INPUT_PIN) | (1ULL << GPIO_17_INPUT_PIN),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };

    assert(gpio_config(&io_conf) == ESP_OK);

    assert(gpio_install_isr_service(0) == ESP_OK);

    assert(gpio_isr_handler_add(GPIO_34_INPUT_PIN, isr_gpio_isr_34, (void*)GPIO_34_INPUT_PIN) == ESP_OK);
    assert(gpio_isr_handler_add(GPIO_35_INPUT_PIN, isr_gpio_isr_35, (void*)GPIO_35_INPUT_PIN) == ESP_OK);
    assert(gpio_isr_handler_add(GPIO_17_INPUT_PIN, isr_gpio_isr_17, (void*)GPIO_17_INPUT_PIN) == ESP_OK);
    
    ESP_LOGI("HW_GPIO", "GPIO initialized.");
}

void hw_configure_gpio(gpio_num_t pin, gpio_mode_t mode, gpio_int_type_t intr_type)
{
    gpio_config_t io_conf = {
        .intr_type = intr_type,
        .mode = mode,
        .pin_bit_mask = (1ULL << pin),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };

    assert(gpio_config(&io_conf) == ESP_OK);
}

void hw_gpio_level_set(gpio_num_t pin, uint32_t level)
{
    gpio_set_level(pin, level);
}