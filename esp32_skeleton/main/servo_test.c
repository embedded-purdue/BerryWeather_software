#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rain_sensor.h"  // Contains servo functions

static const char *TAG = "SERVO_TEST";

void app_main(void) {
    ESP_LOGI(TAG, "=== ESP32 Servo Motor Test on GPIO14 ===");
    
    // Initialize servo on GPIO14 (D14)
    esp_err_t ret = servo_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize servo: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(TAG, "Servo initialized successfully on GPIO14");
    ESP_LOGI(TAG, "Starting servo movement test...");
    
    while (1) {
        // Move to 0 degrees (minimum)
        ESP_LOGI(TAG, "Moving to 0 degrees");
        servo_set_angle(0);
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        // Move to 45 degrees
        ESP_LOGI(TAG, "Moving to 45 degrees");
        servo_set_angle(45);
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        // Move to 90 degrees (center)
        ESP_LOGI(TAG, "Moving to 90 degrees");
        servo_set_angle(90);
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        // Move to 135 degrees  
        ESP_LOGI(TAG, "Moving to 135 degrees");
        servo_set_angle(135);
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        // Move to 180 degrees (maximum)
        ESP_LOGI(TAG, "Moving to 180 degrees");
        servo_set_angle(180);
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        // Return to center
        ESP_LOGI(TAG, "Returning to center (90 degrees)");
        servo_set_angle(90);
        vTaskDelay(pdMS_TO_TICKS(3000));
        
        ESP_LOGI(TAG, "--- Test cycle complete ---");
    }
}