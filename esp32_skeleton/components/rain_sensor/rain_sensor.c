// rain_sensor.c - Rain Water Level sensor driver source (stub)
#include "rain_sensor.h"
#include <stdio.h>
#include "driver/adc.h" 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define RAIN_SENSOR_ADC_CHAN ADC1_CHANNEL_0 
// #define RAIN_SENSOR_POWER_PIN GPIO_NUM_13 

void rain_sensor_init(void) {

    // gpio_config_t io_conf = {
    //     .pin_bit_mask = (1ULL << RAIN_SENSOR_POWER_PIN),
    //     .mode = GPIO_MODE_OUTPUT,
    //     .pull_up_en = GPIO_PULLUP_DISABLE,
    //     .pull_down_en = GPIO_PULLDOWN_DISABLE,
    //     .intr_type = GPIO_INTR_DISABLE
    // };
    // gpio_config(&io_conf);

    // Start with sensor powered OFF
    // gpio_set_level(RAIN_SENSOR_POWER_PIN, 0);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(RAIN_SENSOR_ADC_CHAN, ADC_ATTEN_DB_12);

    printf("Rain Water Level sensor initialized (real hardware)!\n");
}

int rain_sensor_get_raw(void) {
    // Turn sensor ON
    // gpio_set_level(RAIN_SENSOR_POWER_PIN, 1);
    
    // Wait for voltage to stabilize (10ns)
    // vTaskDelay(pdMS_TO_TICKS(10));
    
    // Read the sensor
    int raw = adc1_get_raw(RAIN_SENSOR_ADC_CHAN);
    
    // Turn sensor OFF to prevent corrosion
    // gpio_set_level(RAIN_SENSOR_POWER_PIN, 0);
    return raw;
}

float rain_sensor_get_normalized(void) {
    int raw_value = rain_sensor_get_raw();
    // Convert raw ADC value to normalized 0-1 scale
    // 0V -> 0.0, 3.3V -> 1.0
    float normalized = raw_value / 4095.0;
    return normalized;
}

// void app_main(void) {
//     rain_sensor_init();

//     while(1) {
//         float rain_level = rain_sensor_get_normalized();
//         printf("Rain level: %.2f in\n", rain_level);

//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

