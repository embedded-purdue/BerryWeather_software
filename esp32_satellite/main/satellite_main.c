#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_sleep.h" // For deep sleep
#include "lora_comm.h" 

// --- Satellite Specific Configuration ---
#define SAT_ADDR 10 // This satellite's address
#define MM_ADDR  1  // The MiddleMan's address

// --- Sensor Stubs ---
// Replace these with your actual sensor reading functions
float get_temp_data() { return 25.1 + (float)(esp_random() % 100) / 100.0; }
int get_humidity_data() { return 50 + (esp_random() % 10); }
float get_pressure_data() { return 1013.2 + (float)(esp_random() % 200) / 100.0; }
float get_uv_data() { return 1.5 + (float)(esp_random() % 50) / 100.0; }

/**
 * @brief Main task for the satellite.
 *
 * This task runs in a loop, reads sensor data, sends it via LoRa,
 * and then sleeps for a configured interval.
 */
void periodic_sensor_task(void *arg)
{
    // Define the send interval (e.g., 30 minutes)
    // For testing, you can set this to a shorter duration, like 30 seconds:
    // const int SEND_INTERVAL_SECONDS = 30;
    const int SEND_INTERVAL_MINUTES = 0.5;
    const uint64_t SEND_INTERVAL_US = (uint64_t)SEND_INTERVAL_MINUTES * 60 * 1000 * 1000;
    
    printf("Starting periodic sensor task. Sending every %d minutes.\n", SEND_INTERVAL_MINUTES);

    while(1)
    {
        // 1. Read data from sensors
        float temperature = get_temp_data();
        int humidity = get_humidity_data();
        float pressure = get_pressure_data();
        float uv_index = get_uv_data();

        // 2. Create the JSON payload
        char json_payload[128];
        snprintf(json_payload, sizeof(json_payload),
                "{\"t\":%.1f,\"h\":%d,\"p\":%.1f,\"uv\":%.1f}",
                temperature, humidity, pressure, uv_index);

        printf("----------------------------------\n");
        printf("Reading sensors and sending data...\n");
        printf("Payload: %s\n", json_payload);
        
        // 3. Send the JSON payload via LoRa
        lora_send_message(MM_ADDR, json_payload);

        // 4. Go to sleep
        printf("Data sent. Entering deep sleep for %d minutes.\n", SEND_INTERVAL_MINUTES);
        
        // Flush UART buffer before sleeping
        uart_wait_tx_done(LORA_UART_PORT, pdMS_TO_TICKS(1000));
        vTaskDelay(pdMS_TO_TICKS(500));

        // Configure deep sleep
        esp_sleep_enable_timer_wakeup(SEND_INTERVAL_US);
        
        // Enter deep sleep
        // The device will restart from app_main after waking up.
        esp_deep_sleep_start();

        /*
        // --- OR ---
        // If you don't want deep sleep, you can use vTaskDelay.
        // This keeps the ESP32 powered on but pauses the task.
        // It uses much more power than deep sleep.
        
        printf("Data sent. Delaying task for %d minutes.\n", SEND_INTERVAL_MINUTES);
        printf("----------------------------------\n");
        vTaskDelay(pdMS_TO_TICKS(SEND_INTERVAL_MINUTES * 60 * 1000));
        */
    }
}

void app_main(void)
{
    printf("--- Satellite Device Booting ---\n");

    // 1. Initialize LoRa UART and Reset Module
    lora_uart_config();
    lora_reset();

    vTaskDelay(pdMS_TO_TICKS(500));
    uart_flush_input(LORA_UART_PORT);

    // 2. Perform common LoRa setup
    printf("Setting up LoRa module...\n");
    lora_common_setup(SAT_ADDR); 

    // 3. Send a one-time "boot" test message
    printf("Sending boot-up test message...\n");
    lora_send_message(MM_ADDR, "SATELLITE_BOOT_OK");
    vTaskDelay(pdMS_TO_TICKS(2000)); // Give time for message to send

    // 4. Start the main periodic sensor task
    // Since we are using deep sleep, the task doesn't need to be in a loop
    // and we can just call the "do work" part.
    // If NOT using deep sleep, create a persistent task:
    xTaskCreate(periodic_sensor_task, "periodic_sensor_task", 4096, NULL, 5, NULL);

    // If using deep sleep, the code in periodic_sensor_task will run,
    // and then the device will sleep and reboot.
}