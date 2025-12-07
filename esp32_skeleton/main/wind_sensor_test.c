#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wind_sensor.h"

static const char *TAG = "WIND_TEST";

void app_main(void)
{
    ESP_LOGI(TAG, "=== SN-3000-FSA-N01 Wind Sensor Test ===");
    
    // Initialize wind sensor
    esp_err_t ret = wind_sensor_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize wind sensor: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Check wiring and power supply!");
        return;
    }
    
    ESP_LOGI(TAG, "Wind sensor initialized successfully");
    ESP_LOGI(TAG, "Expected wiring:");
    ESP_LOGI(TAG, "  - Wind sensor Brown  -> +12V external power");
    ESP_LOGI(TAG, "  - Wind sensor Black  -> GND (common)");
    ESP_LOGI(TAG, "  - Wind sensor Yellow -> MAX485 A");
    ESP_LOGI(TAG, "  - Wind sensor Blue   -> MAX485 B");
    ESP_LOGI(TAG, "  - MAX485 VCC -> ESP32 3.3V");
    ESP_LOGI(TAG, "  - MAX485 GND -> ESP32 GND");
    ESP_LOGI(TAG, "  - MAX485 RO  -> ESP32 GPIO16");
    ESP_LOGI(TAG, "  - MAX485 DI  -> ESP32 GPIO17");
    ESP_LOGI(TAG, "  - MAX485 DE+RE -> ESP32 GPIO18");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Starting continuous wind speed monitoring...");
    
    // Main loop - read wind speed every 2 seconds
    while (1) {
        float wind_speed;
        
        // Read wind speed
        // Get raw value first
        int raw_value = wind_sensor_get_speed_raw();
        
        ret = wind_sensor_read_speed(&wind_speed);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Wind: %.1f m/s (%.1f mph) | Raw: %d", 
                     wind_speed, wind_speed * 2.237, raw_value);
        } else {
            ESP_LOGE(TAG, "Read failed - check connections");
        }
        
        // Wait 2 seconds before next reading
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}