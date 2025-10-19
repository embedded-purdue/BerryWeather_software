#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "bme688.h"         // Disabled
#include "as7331.h"         // Disabled
#include "soil_moisture.h"  // Disabled
#include "rain_sensor.h"    // Disabled
#include "ds18b20.h"        // Enabled

static const char *TAG = "MAIN";

void app_main(void) {
    bme688_init();
    as7331_init();
    soil_moisture_init();
    rain_sensor_init();
    ds18b20_init(); // Initialize only the DS18B20

    while (1) {
        float temp, hum, pres, light, soil_moisture, rain_level; // Old variables
        float soil_temp; // The only variable we need now

        bme688_read_temperature(&temp);
        bme688_read_humidity(&hum);
        bme688_read_pressure(&pres);
        as7331_read_light(&light);
        ds18b20_read_temperature(&soil_temp); // Read only from the DS18B20
        soil_moisture_read(&soil_moisture);
        rain_sensor_read(&rain_level);

        ESP_LOGI(TAG, "BME688 -> Temp: %.2f °C, Hum: %.2f %%, Pres: %.2f hPa", temp, hum, pres);
        ESP_LOGI(TAG, "AS7331 -> Light: %.2f uW/cm²", light);
        ESP_LOGI(TAG, "DS18B20 -> Soil Temp: %.2f °C", soil_temp); // Log only the DS18B20 data
        ESP_LOGI(TAG, "Soil Moisture -> Value: %.2f", soil_moisture);
        ESP_LOGI(TAG, "Rain Sensor -> Level: %.2f", rain_level);

        vTaskDelay(pdMS_TO_TICKS(400));
    }
}