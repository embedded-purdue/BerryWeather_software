/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "soil_moisture.h"

static const char *TAG = "MAIN";

void app_main(void) {
    soil_moisture_init();

    while (1) {
        float temp, hum, pres, light, soil_temp, soil_moisture, rain_level;
        
        soil_moisture_read(&soil_moisture);
        ESP_LOGI(TAG, "Soil Moisture -> Value: %.2f", soil_moisture);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}