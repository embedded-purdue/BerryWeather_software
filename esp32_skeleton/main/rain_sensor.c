// rain_sensor.c - Rain Water Level sensor driver source (stub)
#include "rain_sensor.h"
#include <stdio.h>
#include "driver/adc.h" // Uncomment if using ADC
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


// Goals for myself
// 1. Learn how people measure precipitation
// 2. How to connect sensor to ESP32
// 3. Take measurements
// 4. How to send measurements

#define RAIN_SENSOR_ADC_CHAN ADC1_CHANNEL_0 // Replace with your ADC channel
#define RAIN_SENSOR_POWER_PIN GPIO_NUM_4 

void rain_sensor_init(void) {
    // 1. (Optional) Initialize ADC here if not done elsewhere

    // 2. Configure ADC width, attenuation, etc. (see ESP-IDF ADC docs)
    // Example: adc1_config_width(ADC_WIDTH_BIT_12);
    // adc1_config_channel_atten(RAIN_SENSOR_ADC_CHAN, ADC_ATTEN_DB_11);
    

    // Add more configuration as needed
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RAIN_SENSOR_POWER_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Start with sensor powered OFF
    gpio_set_level(RAIN_SENSOR_POWER_PIN, 0);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(RAIN_SENSOR_ADC_CHAN, ADC_ATTEN_DB_12);

    printf("Rain Water Level sensor initialized (real hardware)!\n");
}

// INCLUDE THE REST OF THE FUNCTIONS BELOW
int rain_sensor_get_raw(void) {
    // Turn sensor ON
    gpio_set_level(RAIN_SENSOR_POWER_PIN, 1);
    
    // Wait for voltage to stabilize (10ns)
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Read the sensor
    int raw = adc1_get_raw(RAIN_SENSOR_ADC_CHAN);
    
    // Turn sensor OFF to prevent corrosion
    gpio_set_level(RAIN_SENSOR_POWER_PIN, 0);
    return raw;
}

float rain_sensor_get_voltage(void) {
    int raw_value = rain_sensor_get_raw();
    // Convert raw ADC value to voltage (0-3.3V)
    float voltage = (raw_value / 4095.0) * 3.3;
    return voltage;
}


void app_main(void) {
    rain_sensor_init();

    while(1) {
        float rain_level = rain_sensor_get_voltage();
        printf("Rain level: %.2f\n", rain_level);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


// #include "rain_sensor.h"
// #include <stdio.h>
// #include "esp_adc/adc_oneshot.h"  // New header

// // ADC handle (replaces channel number)
// static adc_oneshot_unit_handle_t adc1_handle;

// #define RAIN_SENSOR_ADC_CHANNEL ADC_CHANNEL_0  // GPIO36
// #define RAIN_SENSOR_ATTEN ADC_ATTEN_DB_12      // 0-3.3V range

// void rain_sensor_init(void) {
//     // Configure ADC unit
//     adc_oneshot_unit_init_cfg_t init_config = {
//         .unit_id = ADC_UNIT_1,
//         .ulp_mode = ADC_ULP_MODE_DISABLE,
//     };
    
//     // Initialize ADC1
//     ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
    
//     // Configure the channel
//     adc_oneshot_chan_cfg_t config = {
//         .bitwidth = ADC_BITWIDTH_12,           // 12-bit resolution (0-4095)
//         .atten = RAIN_SENSOR_ATTEN,            // 0-3.3V range
//     };
    
//     ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, RAIN_SENSOR_ADC_CHANNEL, &config));
    
//     printf("Rain Water Level sensor initialized (real hardware)!\n");
// }

// int rain_sensor_get_raw(void) {
//     int raw_value = 0;
//     adc_oneshot_read(adc1_handle, RAIN_SENSOR_ADC_CHANNEL, &raw_value);
//     return raw_value;
// }

// float rain_sensor_get_voltage(void) {
//     int raw_value = rain_sensor_get_raw();
//     // Convert raw ADC value to voltage (0-3.3V)
//     float voltage = (raw_value / 4095.0) * 3.3;
//     return voltage;
// }

// void rain_sensor_read(void) {
//     int raw = rain_sensor_get_raw();
//     float voltage = rain_sensor_get_voltage();
    
//     printf("Raw ADC: %d, Voltage: %.2f V\n", raw, voltage);
    
//     // Interpret the reading (calibrate these thresholds with real sensor)
//     if (raw < 500) {
//         printf("Status: DRY\n");
//     } else if (raw < 2000) {
//         printf("Status: DAMP\n");
//     } else {
//         printf("Status: WET\n");
//     }
// }
