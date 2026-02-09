#include "soil.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"

#define ADC_PIN 28
#define ADC_CHANNEL 2
#define SOIL_PWR_PIN 8

#define SOIL_ADC_DRY 2000
#define SOIL_ADC_WET 2540

void soil_hw_init(void) {
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_CHANNEL);

    gpio_init(SOIL_PWR_PIN);
    gpio_set_dir(SOIL_PWR_PIN, GPIO_OUT);
    gpio_put(SOIL_PWR_PIN, 0);
}

uint16_t read_soil_adc(void) {
    gpio_put(SOIL_PWR_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(20));

    uint32_t sum = 0;
    for (int i = 0; i < 8; i++) {
        sum += adc_read();
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    gpio_put(SOIL_PWR_PIN, 0);
    return sum / 8;
}

float map_percent(uint16_t adc) {
    if (adc <= SOIL_ADC_DRY) return 0.0f;
    if (adc >= SOIL_ADC_WET) return 100.0f;

    return (adc - SOIL_ADC_DRY) * 100.0f /
           (SOIL_ADC_WET - SOIL_ADC_DRY);
}
