#include <stdio.h>
#include "as7331.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app main(void) {
    AS7331 sensor;
    AS7331_Light light;

    if (as7331_init(&sensor) != ESP_OK) {
        printf("Failed to initialize AS7331!\n")
        return;
    }

    while(1){
        if (as7331_read_light(&sensor, &light) == ESP_OK) {
            printf("UVA: %.2f µW/cm²\tUVB: %.2f µW/cm²\tUVC: %.2f µW/cm²\n", light.uva, light.uvb, light.uvc);
        }
        else {
            printf("Error reading light values!\n")
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}