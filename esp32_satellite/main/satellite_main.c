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
    const int SEND_INTERVAL_MINUTES = 1;
    const uint64_t SEND_INTERVAL_US = (uint64_t)SEND_INTERVAL_MINUTES * 60 * 1000 * 1000;
    
    printf("Starting periodic sensor task. Sending every %d minutes.\n", SEND_INTERVAL_MINUTES);

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
    
    lora_send_message(MM_ADDR, json_payload);
    // uart_flush_input(LORA_UART_PORT);
    printf("Waiting for data ACK from MiddleMan...\n");
    
    char rx_buf[128];
    bool ack_received = false;
    
    for (int attempt = 0; attempt < 25; attempt++) {
        if (lora_wait_for_message(rx_buf, sizeof(rx_buf), 6000)) {
            if (strstr(rx_buf, "MM_ACK_DATA")) {
                printf("ACK received from MiddleMan!\n");
                ack_received = true;
                break;
            }
        }
        printf("No ACK, retrying send (%d/3)...\n", attempt + 1);
        lora_send_message(MM_ADDR, json_payload);
    }
    
    // 4. Sleep if ACK or after max retries
    if (!ack_received)
        printf("No ACK from MiddleMan, going to sleep anyway.\n");
    
    uart_wait_tx_done(LORA_UART_PORT, pdMS_TO_TICKS(1000));
    vTaskDelay(pdMS_TO_TICKS(500));
    
    esp_sleep_enable_timer_wakeup(SEND_INTERVAL_US);
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

void app_main(void)
{
    printf("--- Satellite Device Booting ---\n");

    // 1. Initialize LoRa UART and Reset Module
    lora_uart_config();
    lora_reset();

    vTaskDelay(pdMS_TO_TICKS(500));
    // uart_flush_input(LORA_UART_PORT);

    // 2. Perform common LoRa setup
    printf("Setting up LoRa module...\n");
    lora_common_setup(SAT_ADDR); 

    printf("Performing LoRa boot handshake...\n");
    if (lora_boot_handshake(false, MM_ADDR)) {
        printf("Handshake successful! Starting periodic task.\n");
    } else {
        printf("Handshake failed. Continuing anyway.\n");
    }
    xTaskCreate(periodic_sensor_task, "periodic_sensor_task", 4096, NULL, 5, NULL);

    // If using deep sleep, the code in periodic_sensor_task will run,
    // and then the device will sleep and reboot.
}